#include "main.h"

void setup() {
  pinMode(PRECHARGE_CONTACTOR, OUTPUT);
  pinMode(NEGATIVE_CONTACTOR, OUTPUT);
  pinMode(POSITIVE_CONTACTOR, OUTPUT);

  digitalWrite(PRECHARGE_CONTACTOR, HIGH);
  digitalWrite(NEGATIVE_CONTACTOR, HIGH);
  digitalWrite(POSITIVE_CONTACTOR, HIGH);

  shunt.begin(500);
  shunt.initialize();

  can1.begin(500);
  can1.watchFor(HV_COMMAND_ID);
  can1.attachCANInterrupt(canInputTask);

  os.begin();
  os.addTask(canOutputTask, os.convertMs(200));
  os.addTask(currentSensorTask, os.convertMs(10));

  shunt.START();
}

void loop() { }

void enablePrecharge(bool enable) {
  digitalWrite(PRECHARGE_CONTACTOR, enable);
}

void enableNegativeContactor(bool enable) {
  digitalWrite(NEGATIVE_CONTACTOR, enable);
}

void enablePositiveContactor(bool enable) {
  digitalWrite(POSITIVE_CONTACTOR, enable);
}

void canInputTask(CAN_FRAME *frame) {
  uint8_t command = frame->data.byte[0];

  switch (command) {
    case HighVoltageCommand::Enable:
      enableHighVoltage();
    break;
    case HighVoltageCommand::Disable:
      disableHighVoltage();
    break;
    case HighVoltageCommand::ResetCounters:
      resetCurrentSensor();
    break;
  }
}

void canOutputTask() {
  outputFrame.id = HV_STATUS_ID;
  outputFrame.data.bytes[0] = state;
  outputFrame.data.bytes[1] = faults;

  can1.sendFrame(outputFrame);
}

void currentSensorTask() {
  packVoltage = shunt.Voltage1;
  outputVoltage = shunt.Voltage2;
  current = shunt.Amperes;
  busTemperature = shunt.Temperature;

  if (packVoltage < minPackVoltage) {
    faults |= HighVoltageFault::UNDER_VOLTAGE;
  }
  else if (packVoltage > maxPackVoltage) {
    faults |= HighVoltageFault::OVER_VOLTAGE;
  }
  else {
    // clear voltage faults
    faults = faults & ~HighVoltageFault::UNDER_VOLTAGE;
    faults = faults & ~HighVoltageFault::OVER_VOLTAGE;
  }

  if (current < minCurrent) {
    faults |= HighVoltageFault::UNDER_CURRENT;
  }
  else if (current > maxCurrent) {
    faults |= HighVoltageFault::OVER_CURRENT;
  }
  else {
    // clear current faults
    faults &= ~HighVoltageFault::UNDER_CURRENT;
    faults &= ~HighVoltageFault::OVER_CURRENT; 
  } 

  if (busTemperature < minimumTemperature) {
    faults |= HighVoltageFault::UNDER_TEMPERATURE;
  } 
  else if (busTemperature > maximumTemperature) {
    faults |= HighVoltageFault::OVER_TEMPERATURE;
  }
  else {
    // clear temperature faults
    faults |= ~HighVoltageFault::UNDER_TEMPERATURE;
    faults |= ~HighVoltageFault::OVER_TEMPERATURE;
  }
}

void enableHighVoltage() {
  // clear pre-charge timeout if it exists
  faults &= ~HighVoltageFault::PRE_CHARGE_TIMEOUT;

  if (state != HighVoltageStatus::Disconnected) {
    return;
  }

  enableNegativeContactor(true);
  enablePrecharge(true);

  if (faults != 0)
    return;

  bool prechargeCompleted = false;
  int32_t timeLeft = prechargeTimeout;
  while (timeLeft > 0) {
    if (outputVoltage >= prechargeTimeout) {
      prechargeCompleted = true;
      break;
    }
    delay(PRECHARGE_LOOP_DELAY);
    timeLeft -= PRECHARGE_LOOP_DELAY;
  }

  // pre-charge timeout fault
  if (!prechargeCompleted) {
    faults |= HighVoltageFault::PRE_CHARGE_TIMEOUT; 
    enablePrecharge(false);
    return;
  }

  // enable positive contactor if everything is good
  enablePositiveContactor(true);

  // disable pre-charge contactor
  enablePrecharge(false);
}

void disableHighVoltage() {
  enablePositiveContactor(false);
  enableNegativeContactor(false);

  // wait some time and then check output voltage
  bool disconnectCompleted = false;
  int32_t timeLeft = disconnectTimeout;

  while (timeLeft > 0) {
    if (outputVoltage < minPackVoltage) {
      disconnectCompleted = true;
      return;
    }
  }

  // disconnect timeout fault (something is wrong with the contactors)
  if (!disconnectCompleted) {
    faults |= HighVoltageFault::DISCONNECT_TIMEOUT;
    return;
  }

  state = HighVoltageStatus::Disconnected;
}

void resetCurrentSensor() {
  shunt.KWH = 0;
  shunt.AH = 0;
  shunt.framecount = 0;
  shunt.RESTART();
  shunt.initCurrent();
}
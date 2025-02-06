#include <Arduino.h>
#include "can_common.h"
#include "MCP2515.h"
#include "SimpleISA.h"
#include "leOS2.h"
#include "models.h"

#define PRECHARGE_CONTACTOR 4
#define NEGATIVE_CONTACTOR 5
#define POSITIVE_CONTACTOR 6

#define CAN0_CS 10
#define CAN0_EN 11
#define CAN0_INT 12

#define CAN1_CS 13
#define CAN1_EN 14
#define CAN1_INT 15

#define HV_COMMAND_ID 0x600
#define HV_STATUS_ID 0x601
#define HV_DATA_ID 0x602

CANListener listener;

// can0 -> ISA
MCP2515 can0(CAN0_CS, CAN0_INT);
ISA shunt(&can0, CAN0_EN);

// can1 -> VCU
MCP2515 can1(CAN1_CS, CAN1_INT);

leOS2 os;
HighVoltageStatus state;
uint64_t faults;

CAN_FRAME outputFrame;

// validations
double prechargeVoltageThreshold;

int32_t prechargeTimeout, disconnectTimeout;
const uint64_t PRECHARGE_LOOP_DELAY = 10;

double minPackVoltage, maxPackVoltage;
double packVoltage, outputVoltage;

double minCurrent, maxCurrent;
double current;

double minimumTemperature, maximumTemperature;
double busTemperature;

void enablePrecharge(bool enable);
void enableNegativeContactor(bool enable);
void enablePositiveContactor(bool enable);

void canInputTask(CAN_FRAME *frame);
void canOutputTask();
void currentSensorTask();

void enableHighVoltage();
void disableHighVoltage();
void resetCurrentSensor();
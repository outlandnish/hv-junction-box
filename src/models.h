#pragma once
#include <stdint.h>

enum HighVoltageStatus : uint8_t {
  Disconnected,
  Precharge,
  Connected
};

enum HighVoltageFault : uint16_t {
  UNDER_VOLTAGE = 0x01,
  OVER_VOLTAGE = 0x02,
  UNDER_CURRENT = 0x04,
  OVER_CURRENT = 0x08,
  UNDER_TEMPERATURE = 0x10,
  OVER_TEMPERATURE = 0x20,
  PRE_CHARGE_TIMEOUT = 0x40,
  DISCONNECT_TIMEOUT = 0x80
};

enum HighVoltageCommand : uint8_t {
  Enable,
  Disable,
  ResetCounters
};


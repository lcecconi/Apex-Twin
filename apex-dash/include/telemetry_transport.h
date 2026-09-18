#pragma once

#include <stdint.h>
#include "telemetry_can.h"

typedef void (*CanFrameReceiveCallback)(const CanFdFrame &frame, void *user_arg);

/**
 * Abstract Telemetry Transport Interface.
 * Allows Apex-Dash and Apex-Track to communicate interchangeably over
 * ESP-NOW wireless or ESP32 TWAI (CAN 2.0B / CAN-FD) physical wiring.
 */
class ITelemetryTransport {
public:
  virtual ~ITelemetryTransport() {}
  virtual bool begin() = 0;
  virtual bool isConnected() const = 0;
  virtual int8_t getRssi() const = 0;
  virtual bool sendFrame(const CanFdFrame &frame) = 0;
  virtual void setReceiveCallback(CanFrameReceiveCallback cb, void *user_arg = nullptr) = 0;
  virtual void poll() {}
};

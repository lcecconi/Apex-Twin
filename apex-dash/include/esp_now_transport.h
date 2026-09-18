/**
 * @file esp_now_transport.h
 * Native ESP-IDF ESP-NOW Virtual CAN-FD Transport for Apex-Dash
 */

#pragma once

#include <cstdint>
#include "telemetry_can.h"
#include "telemetry_data.h"

namespace ApexTransport {

class EspNowTransport {
public:
    void init(ChassisTelemetry *target_chassis);
    void update();
    bool isConnected() const;

private:
    ChassisTelemetry *_chassis = nullptr;
    uint32_t _last_rx_ms = 0;
};

} // namespace ApexTransport

#pragma once

#include <vector>
#include <string>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

typedef enum {
    TIME_SYNC = 0x31,
    POWER_MANAGEMENT,
    COLOR_TABLE,
    ACK,
    POWEROFF_SCHEDULE,
} CommandType;

struct SuperfectFormat {
    CommandType command;
    uint32_t packet_length;
    std::string body;
};

extern const struct device *backend_uart;
extern const struct device *config_uart;
void superfectSend(const struct device *dev, CommandType cmd, uint8_t body[], uint32_t length);
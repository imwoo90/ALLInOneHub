#pragma once

#include <vector>
#include <string>

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


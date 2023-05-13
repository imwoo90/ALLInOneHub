#pragma once

#include <zephyr/kernel.h>

typedef enum {
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASED
} ButtonState;

typedef enum {
    BUTTON_TYPE_POWER,
    BUTTON_TYPE_RESERVE_UP,
    BUTTON_TYPE_RESERVE_CONFIRM,
    BUTTON_TYPE_NEOPIXEL_MODE,
    BUTTON_TYPE_MAX = 4,
} ButtonType;

typedef void (*button_event_handler_t)(ButtonType type, ButtonState state);

//Assume to use gpio0
int button_init(const uint8_t pins[BUTTON_TYPE_MAX], button_event_handler_t handler);
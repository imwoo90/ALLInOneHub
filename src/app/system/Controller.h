#pragma once

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <TM1637Display.h>

#define MAX_MSG_SIZE 256

typedef enum {
    MSG_INITIAL,
    MSG_HUB_ON,
    MSG_HUB_OFF,
    MSG_TIME_DISPLAY,
    MSG_RESERVE_CANCEL,
    MSG_BUTTON_POWER,
    MSG_BUTTON_RESERVE_UP,
    MSG_BUTTON_RESERVE_CONFIRM,
    MSG_BUTTON_NEOPIXEL_MODE,
    MSG_BACKEND_PING,
    MSG_TICK,
    MSG_BLINK_POWER_LED,
    MSG_TEST,
} MessageType;

struct Message {
    MessageType type;
    void* data;
};

class Controller {
private:
    k_msgq _q;
    const struct device* _port;
    const struct device* _uart2HID;
    char __aligned(4) _q_buffer[MAX_MSG_SIZE*sizeof(Message)];

    timeval _tv;

    bool _on_hub_power = false;
    bool _set_reserve = false;
    int _reserve_count = 0;

    TM1637Display* _fnd;

    void initialize();
    void eventHandler(Message &msg);
    void uart2HID(uint8_t key_code);
public:
    usb_dc_status_code _usb_status;
    bool _is_alive_backend = false;
    
    static Controller* getInstance() {
        static Controller singleton_instance;
        return &singleton_instance;
    }

    void putMessage(MessageType type, void* data, int delay_ms = 0);

    void setup();
    void loop();
};
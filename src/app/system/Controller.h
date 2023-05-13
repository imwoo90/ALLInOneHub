#pragma once

#include <zephyr/kernel.h>
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
    MSG_TIMESYNC,
    MSG_TIME_DISPLAY,
    MSG_BUTTON_POWER,
    MSG_BUTTON_RESERVE_UP,
    MSG_BUTTON_RESERVE_CONFIRM,
    MSG_BUTTON_NEOPIXEL_MODE,
    MSG_BACKEND_PING,
    MSG_TEST,
} MessageType;

struct Message {
    MessageType type;
    void* data;
};

class Controller {
private:
    k_msgq _q;
    char __aligned(4) _q_buffer[MAX_MSG_SIZE*sizeof(Message)];

    timeval _tv;
    bool _on_hub_power;

    
    TM1637Display* _fnd;

    void initialize();
    void eventHandler(Message &msg);
public:
    static Controller* getInstance() {
        static Controller singleton_instance;
        return &singleton_instance;
    }

    void putMessage(MessageType type, void* data, int delay_ms = 0);

    void setup();
    void loop();
};
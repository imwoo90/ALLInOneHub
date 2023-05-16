#include <Controller.h>
#include <button.h>

#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(CTRL);

static void button_event_handler(ButtonType type, ButtonState state) {
    Controller *p = Controller::getInstance();
    if (state == BUTTON_STATE_PRESSED) {
        switch (type) {
        case BUTTON_TYPE_POWER:
            p->putMessage(MSG_BUTTON_POWER, NULL);
            break;
        case BUTTON_TYPE_RESERVE_UP:
            p->putMessage(MSG_BUTTON_RESERVE_UP, NULL);
            break;
        case BUTTON_TYPE_RESERVE_CONFIRM:
            p->putMessage(MSG_BUTTON_RESERVE_CONFIRM, NULL);
            break;
        case BUTTON_TYPE_NEOPIXEL_MODE:
            p->putMessage(MSG_BUTTON_NEOPIXEL_MODE, NULL);
            break;
        default:
            break;
        }
    }
}

struct MessageCallbackData{
    struct k_work_delayable work;
    struct k_msgq* q;
    Message msg;
};

static void putMessageCallback( struct k_work *work ) {
    MessageCallbackData* p = (MessageCallbackData*)work;
    k_msgq_put(p->q, &p->msg, K_NO_WAIT);
    delete p;
}

void Controller::putMessage(MessageType type, void* data, int delay_ms) {
    MessageCallbackData *p = new MessageCallbackData();
    p->work = Z_WORK_DELAYABLE_INITIALIZER(putMessageCallback);
    p->q = &_q; 
    p->msg.type = type; p->msg.data = data;
    k_work_schedule(&p->work, K_MSEC(delay_ms));
}

void Controller::setup() {
    button_init((const uint8_t[]){11, 12, 24, 25}, button_event_handler);
    _fnd = new TM1637Display(3, 4);
    _fnd->setBrightness(0x0f);

    k_msgq_init(&_q, _q_buffer, sizeof(Message), MAX_MSG_SIZE);
    putMessage(MSG_INITIAL, NULL);
}

void Controller::loop() {
    Message msg;
    k_msgq_get(&_q, &msg, K_FOREVER);
    eventHandler(msg);
}

void Controller::eventHandler(Message &msg) {
    tm* now;

    switch(msg.type) {
    case MSG_INITIAL:
        initialize();
        putMessage(MSG_TIME_DISPLAY, NULL);
        break;
    case MSG_HUB_ON:
        _on_hub_power = true;
        // power on hub
        // power led on
        // reschedule hub off work (10min)
        // send key data to the uart2HID
        break;
    case MSG_HUB_OFF:
        _on_hub_power = false;
        // power off hub
        // power led off
        break;
    case MSG_TIME_DISPLAY:
        gettimeofday(&_tv, NULL);
        now = localtime(&_tv.tv_sec);
        _fnd->showNumberDecEx(now->tm_hour * 100 + now->tm_min, 0b01000000, true);
        putMessage(MSG_TIME_DISPLAY, NULL, 1000);
        break;
    case MSG_BUTTON_POWER:
        if (_on_hub_power) {
            // send a off command to the PC program
        } else {
            putMessage(MSG_HUB_ON, NULL);
        }
        break;
    case MSG_BUTTON_RESERVE_UP:
        // if ! set power off reserve
            // count up reserve up button press
            // reschedule reserve cancle work (5sec)
        break;
    case MSG_BUTTON_RESERVE_CONFIRM:
        // if ! set power off reserve
            // send power off reserved time to pc (current time + 20*reserve_up_cnt)
            // cancel schedule reserve cancel work
            // count = 0
            // power off reserve = true
        // else
            // send cancel command to the PC program
            // power off reserve = false
        break;
    case MSG_BUTTON_NEOPIXEL_MODE:
        // rotate neopixel mode
        break;
    case MSG_BACKEND_PING:
        LOG_INF("ping");
        // reschedule hub off work
        break;
    case MSG_TEST:
        break;
    default:
        break;
    }
}

void Controller::initialize() {
    
}

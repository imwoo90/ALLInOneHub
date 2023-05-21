#include <Controller.h>
#include <button.h>
#include <superfect_protocol.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(CTRL);

static void power_off_work_handle(struct k_work *work) {
    Controller *p = Controller::getInstance();
    p->putMessage(MSG_HUB_OFF, NULL);
}
K_WORK_DELAYABLE_DEFINE(power_off_work, power_off_work_handle);

static void reserve_cancel_work_handle(struct k_work *work) {
    Controller *p = Controller::getInstance();
    p->putMessage(MSG_RESERVE_CANCEL, NULL);
}
K_WORK_DELAYABLE_DEFINE(reserve_cancel_work, reserve_cancel_work_handle);

static void backend_alive_work_handle(struct k_work *work) {
    Controller *p = Controller::getInstance();
    p->_is_alive_backend = false;
}
K_WORK_DELAYABLE_DEFINE(backend_alive_work, backend_alive_work_handle);

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
    const uint8_t _btn[BUTTON_TYPE_MAX] = {
        CONFIG_BTN_POWER_PIN,
        CONFIG_BTN_RESERVE_UP_PIN,
        CONFIG_BTN_RESERVE_CONFIRM_PIN,
        CONFIG_BTN_NEOPIXEL_MODE_PIN,
    };

    _port = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    _uart2HID = DEVICE_DT_GET(DT_NODELABEL(uart0));
    button_init(_btn, button_event_handler);
    _fnd = new TM1637Display(CONFIG_FND_CLK_PIN, CONFIG_FND_DIO_PIN);
    _fnd->setBrightness(0x0f);

    gpio_pin_configure(_port, CONFIG_LED_USB_HUB_SW_PIN, GPIO_OUTPUT);
    gpio_pin_configure(_port, CONFIG_LED_RESERVE_PIN, GPIO_OUTPUT);
    gpio_pin_configure(_port, CONFIG_LED_POWER_PIN, GPIO_OUTPUT);

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
        putMessage(MSG_TICK, NULL);
        break;
    case MSG_HUB_ON:
        _on_hub_power = true;
        gpio_pin_set(_port, CONFIG_LED_USB_HUB_SW_PIN, 1);
        gpio_pin_set(_port, CONFIG_LED_POWER_PIN, 1);
        k_work_reschedule(&power_off_work, K_MINUTES(10)); // reschedule hub off work (10min)
        uart2HID(0x04); // send 'A' key_code data to the uart2HID
        break;
    case MSG_HUB_OFF:
        _on_hub_power = false;
        gpio_pin_set(_port, CONFIG_LED_USB_HUB_SW_PIN, 0);
        gpio_pin_set(_port, CONFIG_LED_POWER_PIN, 0);
        break;
    case MSG_TIME_DISPLAY:
        gettimeofday(&_tv, NULL);
        _tv.tv_sec += 20*60*_reserve_count; //count per 20minutes. it is for reserved time display
        now = localtime(&_tv.tv_sec);
        _fnd->showNumberDecEx(now->tm_hour * 100 + now->tm_min, 0b01000000, true);
        break;
    case MSG_BUTTON_POWER:
        if (_on_hub_power) {
            if(_is_alive_backend) {
                gettimeofday(&_tv, NULL);
                now = localtime(&_tv.tv_sec);
                char str_time[70];
                int str_time_len = sprintf(str_time, "%4d%02d%02d%02d%02d%02d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
                LOG_INF("%s", str_time);
                superfectSend(backend_uart, POWEROFF_SCHEDULE, (uint8_t*)str_time, str_time_len);
            }
        } else {
            putMessage(MSG_HUB_ON, NULL);
        }
        break;
    case MSG_BUTTON_RESERVE_UP:
        if (!_set_reserve) {
            _reserve_count += 1;
            k_work_reschedule(&reserve_cancel_work, K_SECONDS(5));
            putMessage(MSG_TIME_DISPLAY, NULL);
        }
        break;
    case MSG_BUTTON_RESERVE_CONFIRM:
        if (_set_reserve) {
            if (_is_alive_backend) {
                uint8_t cancel = 0x01;
                superfectSend(backend_uart, POWER_MANAGEMENT, &cancel, 1);
            }
            // send cancel command to the PC program 이거 지금 정의 안됨
            _set_reserve = false;
            gpio_pin_set(_port, CONFIG_LED_RESERVE_PIN, 0);
        } else if (_reserve_count != 0 && _is_alive_backend) {
            gettimeofday(&_tv, NULL);
            _tv.tv_sec += 20*60*_reserve_count;
            now = localtime(&_tv.tv_sec);
            char str_time[70];
            int str_time_len = sprintf(str_time, "%4d%02d%02d%02d%02d%02d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
            LOG_INF("%s", str_time);
            superfectSend(backend_uart, POWEROFF_SCHEDULE, (uint8_t*)str_time, str_time_len);

            _reserve_count = 0;
            _set_reserve = true;
            k_work_cancel_delayable(&reserve_cancel_work);
            gpio_pin_set(_port, CONFIG_LED_RESERVE_PIN, 1);
            putMessage(MSG_TIME_DISPLAY, NULL);
        }
        break;
    case MSG_RESERVE_CANCEL:
        _reserve_count = 0;
        _set_reserve = false;
        putMessage(MSG_TIME_DISPLAY, NULL);
        break;
    case MSG_BUTTON_NEOPIXEL_MODE:
        // rotate neopixel mode
        break;
    case MSG_BACKEND_PING:
        if (!_on_hub_power) {
            putMessage(MSG_HUB_ON, NULL);
        }
        _is_alive_backend = true;
        k_work_reschedule(&power_off_work, K_MINUTES(10)); // reschedule hub off work (10min)
        k_work_reschedule(&backend_alive_work, K_SECONDS(3)); // reschedule backend alive work (1sec)
        break;
    case MSG_TICK:
        putMessage(MSG_TIME_DISPLAY, NULL);
        putMessage(MSG_TICK, NULL, 1000);
        break;
    case MSG_TEST:
        break;
    default:
        break;
    }
}

void Controller::uart2HID(uint8_t key_code) {
    uint8_t packet[8] = {0x00, 0x00, key_code, 0x00, 0x00, 0x00, 0x00, 0x00};
    // Key press
    for (uint32_t i = 0; i < sizeof(packet); i++) {
        uart_poll_out(_uart2HID, packet[i]);
    }
    for (uint32_t i = 0; i < sizeof(packet); i++) {
        uart_poll_in(_uart2HID, &packet[i]);
    }
    k_msleep(100);

    //key Release
    packet[2] = 0x00;
    for (uint32_t i = 0; i < sizeof(packet); i++) {
        uart_poll_out(_uart2HID, packet[i]);
    }
    for (uint32_t i = 0; i < sizeof(packet); i++) {
        uart_poll_in(_uart2HID, &packet[i]);
    }
}

void Controller::initialize() {
    
}

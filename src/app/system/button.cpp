#include <Controller.h>
#include <button.h>
#include <zephyr/drivers/gpio.h>

static const struct device *button_gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static button_event_handler_t user_cb;
static struct gpio_callback button_callback;
static uint8_t type_to_pin[BUTTON_TYPE_MAX];
k_work_delayable buttonWorks[BUTTON_TYPE_MAX];

static void cooldown_expired(struct k_work *work)
{
    ButtonType type = (ButtonType)(((int)work - (int)&buttonWorks[0])/sizeof(k_work_delayable));

    int val = gpio_pin_get(button_gpio, type_to_pin[type]);
    ButtonState st = val ? BUTTON_STATE_RELEASED : BUTTON_STATE_PRESSED;
    if (user_cb) {
        user_cb(type, st);
    }
}

static void button_interrupts(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
    for(int type = 0; type < BUTTON_TYPE_MAX; type++) {
        if (pins & BIT(type_to_pin[type])) {
            k_work_reschedule(&buttonWorks[type], K_MSEC(15));
        }
    }
}

int button_init(const uint8_t pins[BUTTON_TYPE_MAX], button_event_handler_t handler) {
    user_cb = handler;
    gpio_port_pins_t pin_mask = 0;
    for(int type = 0; type < BUTTON_TYPE_MAX; type++) {
        gpio_pin_configure(button_gpio, pins[type], GPIO_INPUT | GPIO_PULL_UP);
        gpio_pin_interrupt_configure(button_gpio, pins[type], GPIO_INT_EDGE_BOTH);
        pin_mask |= BIT(pins[type]);
        type_to_pin[type] = pins[type];
        buttonWorks[type] = Z_WORK_DELAYABLE_INITIALIZER(cooldown_expired);
    }

    gpio_init_callback(&button_callback, button_interrupts, pin_mask);
	gpio_add_callback(button_gpio, &button_callback);

    return 0;
}


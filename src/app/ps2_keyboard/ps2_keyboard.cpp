#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <ps2dev.h>


static void ps2_keyboard_task(void) {
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    PS2dev keyboard(gpio0, 16,17);
    gpio_pin_configure(gpio0, 18, GPIO_INPUT | GPIO_PULL_DOWN);

    while(true) {
        if (gpio_pin_get(gpio0, 18) == 0) {
            k_msleep(100);
            continue;
        }

        keyboard.keyboard_init();
        uint32_t timecount = 0;
        while(true) {
            if (gpio_pin_get(gpio0, 18) == 0) {
                break;
            }

            unsigned char leds;            
            if(keyboard.keyboard_handle(&leds)) {
                // LOG_INF("%x", leds);
            }

            if((k_uptime_get_32() - timecount) > 1000) {
                keyboard.keyboard_mkbrk(PS2dev::A);
                timecount = k_uptime_get_32();
                // LOG_INF(".");
            }
            k_msleep(10);
        }
    }
}
K_THREAD_DEFINE(ps2_keyboard, 2048, ps2_keyboard_task, NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO-1, 0, 0);
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <ps2dev.h>

K_MUTEX_DEFINE(ps2dev_mutex);
static uint8_t g_read_leds = false;
static PS2dev* g_keyboard = NULL;

void ps2_keyboard_mkbrk(void) {
    k_mutex_lock(&ps2dev_mutex, K_FOREVER);
    g_keyboard->keyboard_mkbrk(PS2dev::A);
    k_mutex_unlock(&ps2dev_mutex);
}

static void ps2_keyboard_task(void) {
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    g_keyboard = new PS2dev(gpio0, CONFIG_PS2_KEYBOARD_CLK, CONFIG_PS2_KEYBOARD_DIO);
    gpio_pin_configure(gpio0, CONFIG_PS2_KEYBOARD_PWR, GPIO_INPUT);

    while(true) {
        if (gpio_pin_get(gpio0, 18) == 0) {
            g_read_leds = false;
            k_msleep(100);
            continue;
        }
        k_msleep(10);
        if(gpio_pin_get(gpio0, 18) == 0) {
            continue;
        }

        k_mutex_lock(&ps2dev_mutex, K_FOREVER);
        if (g_keyboard->keyboard_handle(&g_read_leds)) {
            g_read_leds = true;
        }
        k_mutex_unlock(&ps2dev_mutex);
    }
}
K_THREAD_DEFINE(ps2_keyboard, 2048, ps2_keyboard_task, NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO-1, 0, 0);
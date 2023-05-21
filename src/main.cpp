/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/class/usb_hid.h>

#include <Controller.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

static const uint8_t hid_kbd_report_desc[] = HID_KEYBOARD_REPORT_DESC();

int main(void)
{
	usb_hid_register_device(device_get_binding("HID_0"), hid_kbd_report_desc,
				sizeof(hid_kbd_report_desc), NULL);
	usb_hid_init(device_get_binding("HID_0"));
	int ret = usb_enable(NULL);
	if (ret != 0) {
		return -1;
	}

    LOG_INF("Start All in one hub");

	Controller* ctrl = Controller::getInstance();
	ctrl->setup();

    while(true) {
        ctrl->loop();
    }

    return 0;
}


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

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);


#include <TM1637Display.h>

int main(void)
{
    int ret = usb_enable(NULL);
	if (ret != 0) {
		return -1;
	}

    LOG_INF("Start All in one hub");

    TM1637Display _7segmentsX4(7, 8);
    _7segmentsX4.setBrightness(0x0f);

    int cnt = 0;
    while(true) {
        LOG_INF("count %d", cnt);
        _7segmentsX4.showNumberDecEx(cnt%10000, 0b01000000, true);
        cnt += 1;
        k_msleep(1000);
    }

    return 0;
}

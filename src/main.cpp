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
int cnt = 0;
TM1637Display *_7segmentsX4;

#include <RTClib.h>
#include <zephyr/drivers/gpio.h>
RTC_DS3231 rtc;
static const device *gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
gpio_pin_t _onAlarm_pin = 2;
static struct gpio_callback _onAlarm;
void onAlarm(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    cnt += 1;
    LOG_INF("Alarm occured!");
}

void setup() {
    _7segmentsX4 = new TM1637Display(20, 19);
    _7segmentsX4->setBrightness(0x0f);
    _7segmentsX4->showNumberDecEx(cnt, 0b01000000, true);

    // initializing the rtc
    if(!rtc.begin(DEVICE_DT_GET(DT_NODELABEL(i2c0)))) {
        // while (1) {
        //     LOG_INF("Couldn't find RTC!");
        //     k_msleep(1000);
        // }
        return;
    }

    if(rtc.lostPower()) {
        // this will adjust to the date and time at compilation
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    //we don't need the 32K Pin, so disable it
    rtc.disable32K();

    // Making it so, that the alarm will trigger an interrupt
    // pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
    gpio_pin_interrupt_configure(gpio1, _onAlarm_pin, GPIO_INPUT | GPIO_PULL_UP | GPIO_INT_EDGE_FALLING);
    // attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING);
    gpio_init_callback(&_onAlarm, onAlarm, BIT(2));
    gpio_add_callback(gpio1, &_onAlarm);
    // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
    // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);

    // stop oscillating signals at SQW Pin
    // otherwise setAlarm1 will fail
    rtc.writeSqwPinMode(DS3231_OFF);

    // turn off alarm 2 (in case it isn't off already)
    // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
    rtc.disableAlarm(2);

    // schedule an alarm 10 seconds in the future
    if(!rtc.setAlarm1(
            rtc.now() + TimeSpan(10),
            DS3231_A1_Second // this mode triggers the alarm when the seconds match. See Doxygen for other options
    )) {
        LOG_INF("Error, alarm wasn't set!");
    }else {
        LOG_INF("Alarm will happen in 10 seconds!");
    }

}

void loop() {
    // print current time
    char date[10] = "hh:mm:ss";
    rtc.now().toString(date);
    // Serial.print(date);
    LOG_INF("%s", date);

    // the stored alarm value + mode
    DateTime alarm1 = rtc.getAlarm1();
    Ds3231Alarm1Mode alarm1mode = rtc.getAlarm1Mode();
    char alarm1Date[12] = "DD hh:mm:ss";
    alarm1.toString(alarm1Date);
    // Serial.print(" [Alarm1: ");
    // Serial.print(alarm1Date);
    // Serial.print(", Mode: ");
    const char *str_alarm1mode = "";
    switch (alarm1mode) {
      case DS3231_A1_PerSecond: str_alarm1mode = "PerSecond"; break;
      case DS3231_A1_Second: str_alarm1mode = "Second"; break;
      case DS3231_A1_Minute: str_alarm1mode = "Minute"; break;
      case DS3231_A1_Hour: str_alarm1mode = "Hour"; break;
      case DS3231_A1_Date: str_alarm1mode = "Date"; break;
      case DS3231_A1_Day: str_alarm1mode = "Day"; break;
    }

    // the value at SQW-Pin (because of pullup 1 means no alarm)
    // Serial.print("] SQW: ");
    // Serial.print(digitalRead(CLOCK_INTERRUPT_PIN));
    LOG_INF(" [Alarm1: %s, Mode: %s] SQW: %d", alarm1Date, str_alarm1mode, gpio_pin_get(gpio1, _onAlarm_pin));

    // whether a alarm fired
    // Serial.print(" Fired: ");
    // Serial.print(rtc.alarmFired(1));
    LOG_INF(" Fired: %d", rtc.alarmFired(1));
    // Serial.print(" Alarm2: ");
    // Serial.println(rtc.alarmFired(2));
    // control register values (see https://datasheets.maximintegrated.com/en/ds/DS3231.pdf page 13)
    // Serial.print(" Control: 0b");
    // Serial.println(read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), BIN);

    // resetting SQW and alarm 1 flag
    // using setAlarm1, the next alarm could now be configurated
    if (rtc.alarmFired(1)) {
        rtc.clearAlarm(1);
        // Serial.print(" - Alarm cleared");
        LOG_INF(" - Alarm cleared");
    }
    // Serial.println();

    _7segmentsX4->showNumberDecEx(cnt, 0b01000000, true);
    // delay(2000);
    k_msleep(2000);
}

int main(void)
{
    int ret = usb_enable(NULL);
	if (ret != 0) {
		return -1;
	}

    LOG_INF("Start All in one hub");

    setup();

    while(true) {
        loop();
    }

    return 0;
}


/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <hal/nrf_power.h>

static int board_nrf52840dongle_nrf52840_init(void)
{

	/* if the nrf52840dongle_nrf52840 board is powered from USB
	 * (high voltage mode), GPIO output voltage is set to 1.8 volts by
	 * default and that is not enough to turn the green and blue LEDs on.
	 * Increase GPIO voltage to 3.0 volts.
	 */
	if ((nrf_power_mainregstatus_get(NRF_POWER) ==
	     NRF_POWER_MAINREGSTATUS_HIGH) &&
	    ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
	     (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		NRF_UICR->REGOUT0 =
		    (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
		    (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		/* a reset is required for changes to take effect */
		NVIC_SystemReset();
	}

	return 0;
}

SYS_INIT(board_nrf52840dongle_nrf52840_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

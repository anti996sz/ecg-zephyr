/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "fs_fat.h"

LOG_MODULE_REGISTER(main);

#define ADS1298R DT_NODELABEL(ads1298r)
#define ADS1294R DT_NODELABEL(ads1294r)

struct sensor_value value_1298r[9], value_1294r[5];
// const struct device *dev = DEVICE_DT_GET_ANY(ti_ads129xr);
const struct device *ads1294r = DEVICE_DT_GET(ADS1294R);
const struct device *ads1298r = DEVICE_DT_GET(ADS1298R);

int sd_ok = 0; // SD card init and open file status, 0 - success

static void trigger_handler(const struct device *dev, struct sensor_trigger *trig)
{
	sensor_channel_get(ads1294r, SENSOR_CHAN_ALL, value_1294r);
	sensor_channel_get(ads1298r, SENSOR_CHAN_ALL, value_1298r);

	int16_t buff[12];
	static int counter = 1; // conunt the times this func called

	for (size_t i = 1; i < 9; i++)
	{
		buff[i-1] = (int16_t)(sensor_value_to_double(&value_1298r[i])*1000);
	}

	for (size_t i = 1; i < 5; i++)
	{
		buff[7+i] = (int16_t)(sensor_value_to_double(&value_1294r[i])*1000);
	}

	LOG_INF("%d volt(mV) : %d %d %d %d %d %d %d %d %d %d %d %d", counter++, 
		buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], 
		buff[8], buff[9], buff[10], buff[11]);

	// save ECG data as integer with µV unit
	if(sd_ok == 0){
		save_data(buff, sizeof(buff), counter);
	};
};



void main(void)
{
	sd_ok = fs_init();
	
	if (!device_is_ready(ads1294r)) {
		printk("Device %s is not ready\n", ads1294r->name);
		return;
	}
	if (!device_is_ready(ads1298r)) {
		printk("Device %s is not ready\n", ads1298r->name);
		return;
	}

	printk("Hello World! %s\n", CONFIG_BOARD);

	
	// 设置中断触发器
	static struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	// 数据准备好中断触发器只需要调用一次
	int rc = sensor_trigger_set(ads1298r, &drdy_trigger, trigger_handler);

	if (rc != 0) {
		printk("Trigger set failed: %d\n", rc);
	}

	printk("Trigger set success.\n");

	
	// while (1) {
	// 	// printk("Hello World! %s\n", CONFIG_BOARD);
	// 	k_msleep(SLEEP_TIME_MS);
	// }
}

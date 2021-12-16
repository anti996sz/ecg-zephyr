/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/sensor.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000
#define ADS1298R DT_NODELABEL(ads1298r)
#define ADS1294R DT_NODELABEL(ads1294r)

struct sensor_value value_1298r[9], value_1294r[5];
// const struct device *dev = DEVICE_DT_GET_ANY(ti_ads129xr);
const struct device *ads1294r = DEVICE_DT_GET(ADS1294R);
const struct device *ads1298r = DEVICE_DT_GET(ADS1298R);

int counter = 0;

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}


static void trigger_handler(const struct device *dev,
			    struct sensor_trigger *trig)
{
	sensor_channel_get(ads1294r, SENSOR_CHAN_ALL, value_1294r);
	sensor_channel_get(ads1298r, SENSOR_CHAN_ALL, value_1298r);

	printk("\n%s: %s %d volt %d.%06d mv", 
		now_str(), dev->name, counter++, value_1298r[1].val1, value_1298r[1].val2);

	// printk("\nADS1298R: volt %d.%06d mv", value_1298r[1].val1, value_1298r[1].val2);
	// printk("\nADS1294R: volt %d.%06d mv", value_1294r[1].val1, value_1294r[1].val2);
};



void main(void)
{
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

	printk("Trigger set got %d\n", rc);

	
	while (1) {
		// printk("Hello World! %s\n", CONFIG_BOARD);
		k_msleep(SLEEP_TIME_MS);
	}
}

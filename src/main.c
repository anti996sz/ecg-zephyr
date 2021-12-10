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
#define ADS129xR0 DT_NODELABEL(ads129xr0)
#define ADS129xR1 DT_NODELABEL(ads129xr1)

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
	printk("\n%s: Trigger handler called in %s: %d", now_str(), dev->name, counter++);
};



void main(void)
{
	struct sensor_value value;
	// const struct device *dev = DEVICE_DT_GET_ANY(ti_ads129xr);
	const struct device *ads129xr0 = DEVICE_DT_GET(ADS129xR0);
	const struct device *ads129xr1 = DEVICE_DT_GET(ADS129xR1);

	if (!device_is_ready(ads129xr0)) {
		printk("Device %s is not ready\n", ads129xr0->name);
		return;
	}
	if (!device_is_ready(ads129xr1)) {
		printk("Device %s is not ready\n", ads129xr1->name);
		return;
	}


	// sensor_channel_get(ads129xr0, SENSOR_CHAN_ALL, &value);
	sensor_channel_get(ads129xr1, SENSOR_CHAN_ALL, &value);

	// printk("ads129xr volt %d.%d\r\n", value.val1, value.val2);

	printk("Hello World! %s\n", CONFIG_BOARD);

	
	// 设置中断触发器
	static struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	int rc;
	rc = sensor_trigger_set(ads129xr0, &drdy_trigger, trigger_handler);
	rc = sensor_trigger_set(ads129xr1, &drdy_trigger, trigger_handler);

	if (rc != 0) {
		printk("Trigger set failed: %d\n", rc);
	}

	printk("Trigger set got %d\n", rc);

	
	while (1) {
		// printk("Hello World! %s\n", CONFIG_BOARD);
		k_msleep(SLEEP_TIME_MS);
	}
}

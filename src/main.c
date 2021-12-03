/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/sensor.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000
#define ads129xr0 DT_NODELABEL(ads129xr0)
#define ads129xr1 DT_NODELABEL(ads129xr1)

void main(void)
{
	struct sensor_value value;
	// const struct device *dev = DEVICE_DT_GET_ANY(ti_ads129xr);
	const struct device *ads129xr0 = DEVICE_DT_GET(ads129xr0);
	const struct device *ads129xr1 = DEVICE_DT_GET(ads129xr1);

	if (!device_is_ready(ads129xr0)) {
		printk("Device %s is not ready\n", ads129xr0->name);
		return;
	}
	if (!device_is_ready(ads129xr1)) {
		printk("Device %s is not ready\n", ads129xr1->name);
		return;
	}

	sensor_channel_get(ads129xr0, SENSOR_CHAN_ALL, &value);
	sensor_channel_get(ads129xr1, SENSOR_CHAN_ALL, &value);

	// printk("ads129xr volt %d.%d\r\n", value.val1, value.val2);

	printk("Hello World! %s\n", CONFIG_BOARD);
	
	while (1) {
		// printk("Hello World! %s\n", CONFIG_BOARD);
		k_msleep(SLEEP_TIME_MS);
	}
}

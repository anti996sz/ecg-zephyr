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

void main(void)
{
	struct sensor_value accel[3];
	const struct device *dev = DEVICE_DT_GET_ANY(ads129xr);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

	printk("Hello World! %s\n", CONFIG_BOARD);
	while (1) {
		printk("Hello World! %s\n", CONFIG_BOARD);
		k_msleep(SLEEP_TIME_MS);
	}
}

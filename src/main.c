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
#define ADS129xR0 DT_NODELABEL(ads129xr0)
#define ADS129xR1 DT_NODELABEL(ads129xr1)

static void trigger_handler(const struct device *dev,
			    struct sensor_trigger *trig)
{
	printk("\nTrigger handler called in %s", dev->name);
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


	sensor_channel_get(ads129xr1, SENSOR_CHAN_ALL, &value);
	sensor_channel_get(ads129xr0, SENSOR_CHAN_ALL, &value);

	// printk("ads129xr volt %d.%d\r\n", value.val1, value.val2);

	printk("Hello World! %s\n", CONFIG_BOARD);

	// static struct sensor_trigger drdy_trigger = {
	// 	.type = SENSOR_TRIG_DATA_READY,
	// 	.chan = SENSOR_CHAN_ALL,
	// };

	// int rc;
	// rc = sensor_trigger_set(ads129xr0, &drdy_trigger, trigger_handler);
	// rc = sensor_trigger_set(ads129xr1, &drdy_trigger, trigger_handler);

	// if (rc != 0) {
	// 	printk("Trigger set failed: %d\n", rc);
	// }

	// printk("Trigger set got %d\n", rc);
	
	while (1) {
		// printk("Hello World! %s\n", CONFIG_BOARD);
		k_msleep(SLEEP_TIME_MS);
	}
}

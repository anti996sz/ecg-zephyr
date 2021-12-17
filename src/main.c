/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/sensor.h>

#include <storage/disk_access.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>

LOG_MODULE_REGISTER(main);

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


static void trigger_handler(const struct device *dev, struct sensor_trigger *trig)
{
	sensor_channel_get(ads1294r, SENSOR_CHAN_ALL, value_1294r);
	sensor_channel_get(ads1298r, SENSOR_CHAN_ALL, value_1298r);

	printk("\n%s: %d volt(mV) ", now_str(), counter++);

	for (size_t i = 1; i < 9; i++)
	{
		printk("%d.%06d\t", value_1298r[i].val1, value_1298r[i].val2);
	}

	for (size_t i = 1; i < 5; i++)
	{
		printk("%d.%06d\t", value_1294r[i].val1, value_1294r[i].val2);
	}
	
};


// static int lsdir(const char *path);
void fs_test(void){
	
	static FATFS fat_fs;
	/* mounting info */
	static struct fs_mount_t mp = {
		.type = FS_FATFS,
		.fs_data = &fat_fs,
	};

	/*
	*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
	*  in ffconf.h
	*/
	static const char *disk_mount_pt = "/SD:";

		/* raw disk i/o */
	do {
		static const char *disk_pdrv = "SD";
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_init(disk_pdrv) != 0) {
			LOG_ERR("Storage init ERROR!");
			break;
		}

		if (disk_access_ioctl(disk_pdrv,
			DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			LOG_ERR("Unable to get sector count");
			break;
		}
		LOG_INF("Block count %u", block_count);

		if (disk_access_ioctl(disk_pdrv,
			DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			LOG_ERR("Unable to get sector size");
			break;
		}
		printk("Sector size %u\n", block_size);

		memory_size_mb = (uint64_t)block_count * block_size;
		printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

	} while (0);

};

void main(void)
{
	fs_test();

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

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include <storage/disk_access.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>

#include "fs_fat.h"

#define SDPWR_NODE 	DT_NODELABEL(sdpwr)
#define SDPWR_GPIO	DT_GPIO_LABEL(SDPWR_NODE, gpios)
#define PIN			DT_GPIO_PIN(SDPWR_NODE, gpios)
#define FLAGS		DT_GPIO_FLAGS(SDPWR_NODE, gpios)

LOG_MODULE_REGISTER(fs_fat);

static int lsdir(const char *path);

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

		const struct device *dev = device_get_binding(SDPWR_GPIO);
		int ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
		if (ret < 0) {
			LOG_ERR("SD power controller port config error.");
			return;
		}

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

	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);

	if (res == FR_OK) {
		printk("Disk mounted.\n");
		lsdir(disk_mount_pt);
	} else {
		printk("Error mounting disk.\n");
	}

	// while (1) {
	// 	k_sleep(K_MSEC(1000));
	// }

	uint8_t str[500] = {0x33};

	for (size_t i = 0; i < 500; i++)
	{
		str[i] = i;
		
	};


	struct fs_file_t zfp;
	fs_file_t_init(&zfp);

	int result = fs_open(&zfp, "/SD:/test.dat", FS_O_CREATE | FS_O_RDWR);

	if(result != FR_OK){
		printk("Open file result: %d\n", result);
	}
	LOG_INF("Creat and open file");


	result = fs_write(&zfp, &str, sizeof(str));
	LOG_INF("Bytes write");
	printk("Bytes writen: %d\n", result);

	result = fs_sync(&zfp);
	LOG_INF("File Sync result");
	printk("File Sync result: %d\n", result);

	result = fs_close(&zfp);
	LOG_INF("File Close result");
	printk("File Close result: %d\n", result);
};

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}
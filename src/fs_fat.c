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

struct fs_file_t zfp;
char file_name[] = "/SD:/YSECG001.dat";
uint8_t max_file_order = 0;

int fs_init(void){

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
	mp.mnt_point = disk_mount_pt;

		/* raw disk i/o */
	do {

		const struct device *dev = device_get_binding(SDPWR_GPIO);
		if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS) < 0) 
		{
			LOG_ERR("SD power controller port config error.");
			break;
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

		if (fs_mount(&mp) == FR_OK) {
			printk("Disk mounted.\n");
			lsdir(disk_mount_pt);
		} else {
			printk("Error mounting disk.\n");
			break;
		}

		max_file_order += 1;
		file_name[10] = max_file_order % 1000 / 100 + '0';
		file_name[11] = max_file_order % 100 / 10 + '0';
		file_name[12] = max_file_order % 10 + '0';
		printk("New file name: %s", file_name);
		fs_unlink(file_name);	// delete the file if it exist.
		
		fs_file_t_init(&zfp);
		int result = fs_open(&zfp, file_name, FS_O_CREATE | FS_O_RDWR | FS_O_APPEND);
		LOG_INF("Creat and open fileresult: %d\n", result);

		k_msleep(10000);
		return result; // fs init success.

	} while (0);

	return -1; // fs init fail.
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
		LOG_ERR("Error opening dir %s [%d]", path, res);
		return res;
	}

	LOG_INF("\nListing dir %s ...", path);
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
			printk("[FILE] %s (size = %zu)",
				entry.name, entry.size);
			
			uint8_t file_order = 0;
			for (size_t i = 5; i < sizeof(entry.name)-4; i++) // file name will be YSECGXXX.dat
			{
				if(entry.name[i] >= '0' && entry.name[i] <= '9'){
					file_order = file_order * 10 + (entry.name[i] - '0'); // ASCII('0') => 48
				}
			}

			max_file_order = file_order > max_file_order ? file_order : max_file_order;
			LOG_INF("File order: %d, max file order %d", file_order, max_file_order);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
};

//save ECG data as mV unit
void save_data(int16_t data[], uint8_t length, uint64_t counter)
{
	int result = fs_write(&zfp, &data, length);
	LOG_INF("Bytes write: %d", result);

	// sync to disk every second
	if (counter % 250 == 0)
	{
		LOG_INF("File Sync begin ...");
		int result = fs_sync(&zfp);
		LOG_INF("File Sync result: %d", result);
	};
};


void close_file(void)
{
	int result = fs_close(&zfp);
	LOG_INF("File Close result: %d", result);
};
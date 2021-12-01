/* ads129xr.c - ADS1294R ADS1296R ADS1298R ECG acqusition spi device */

/*
 * Copyright (c) 2021 scottwan@foxmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

#include "ads129xr.h"

// Compatible with "ti,ads129xr"
#define DT_DRV_COMPAT ti_ads129xr

static const struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA,
	.frequency = 4000000,
	.slave = 0,
	.cs = SPI_CS_CONTROL_PTR_DT(DT_NODELABEL(ads129xr0), 2),
};

/**
 * @brief ads129x spi interface
 * 
 * @param dev spi device pointer
 * @param opcode opcode command to read or write register, 
 *        only one byte for system commands and data read commands, two bytes for register read/write command, 
 *        the second byte is the number of registers to read or write.
 * @param op_length length of opcode
 * @param data data will write to register or will received
 * @param data_length the length of data
 * @return int return of spi_transceive()
 */
static int ads129xr_spi_transceive(const struct device *dev, 
							uint8_t *opcode, 
							size_t op_length,
							uint8_t *data,
							size_t data_length)
{
	struct ads129xr_data *drv_data = dev->data;

	printk("\nSPI sent:     ");

	for (int i = 0; i < op_length; i++)
	{
		printk("0x%02x ", opcode[i]);
	}

	const struct spi_buf buf[2] = {
		{
			.buf = opcode,
			.len = op_length
		},
		{
			.buf = data,
			.len = data_length
		}
	};

	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = opcode[0] >> 5 == 0x010 ? 2 : 1	// WREG: opcode[0] >> 5 == 0x010
	};

	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = opcode[0] >> 5 == 0x010 ? 0 : 2	// WREG: opcode[0] >> 5 == 0x010
	};

	int err = spi_transceive(drv_data->spi, &spi_cfg, &tx, &rx);

	if (err) {
		printk("SPI error: %d\n", err);
	} else {

		printk("\nSPI received: ");
		
		for (int i = 0; i < data_length; i++)
		{
			printk("0x%02x ", data[i]);
		}

		printk("\n------------------------------\n");
	};

	return err;
};

static int ads129xr_init(const struct device *dev)
{
	const struct ads129xr_config *drv_cfg = dev->config;

	int ret = gpio_pin_configure(drv_cfg->pwdwn_gpio_spec.port, \
								drv_cfg->pwdwn_gpio_spec.pin, \
								GPIO_OUTPUT_INACTIVE | drv_cfg->pwdwn_gpio_spec.dt_flags);

	ret = gpio_pin_configure(drv_cfg->reset_gpio_spec.port, 
							drv_cfg->reset_gpio_spec.pin, 
							GPIO_OUTPUT_INACTIVE | drv_cfg->reset_gpio_spec.dt_flags);


	k_msleep(1000);	// 确保ADS129xR加电后到正常状态 waite the ads1298 to normal after power on 

	ret = gpio_pin_configure(drv_cfg->ledpw_gpio_spec.port, 
							drv_cfg->ledpw_gpio_spec.pin, 
							GPIO_OUTPUT_ACTIVE | drv_cfg->ledpw_gpio_spec.dt_flags);
	// ret = gpio_pin_set(drv_cfg->ledpw_gpio_spec.port, drv_cfg->ledpw_gpio_spec.pin, 1);

	if (ret < 0) {
		printk("ads1298 pwdwn gpio init error");
		return -1;
	}
    // GPIO的配置
    // GPIO中断安装
    // 旋转编码器GPIO初始化状态读取
    // 驱动初始化状态设置
    // 驱动线程创建
    // 使能中断

	struct ads129xr_data *drv_data = dev->data;
	drv_data->spi = device_get_binding(DT_INST_BUS_LABEL(0));

	uint8_t opcode[1] = {SDATAC}, data[0];
	ads129xr_spi_transceive(dev, opcode, 1, data, 0);
	k_msleep(1);

	return 0;
};


static int ads129xr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	uint8_t opcode[2] = {RREG, 0x00}, data[1];	
	ads129xr_spi_transceive(dev, opcode, sizeof(opcode), data, sizeof(data));

	// k_msleep(1000);

	// 测试读取前2个寄存器的结果
	uint8_t opcode2[2] = {RREG, 0x01}, data2[2];	
	ads129xr_spi_transceive(dev, opcode2, sizeof(opcode2), data2, sizeof(data2));

	printk("ADS129XR Driver !!! %s\n", CONFIG_BOARD);

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	// acc = drv_data->pulses;

	// val->val1 = acc * FULL_ANGLE / (drv_cfg->ppr * drv_cfg->spp);
	// val->val2 = acc * FULL_ANGLE - val->val1 * (drv_cfg->ppr * drv_cfg->spp);
	// if (val->val2) {
	// 	val->val2 *= 1000000;
	// 	val->val2 /= (drv_cfg->ppr * drv_cfg->spp);
	// }

	val->val1 = 1;
	val->val2 = 2;

	return 0;
};

static const struct sensor_driver_api ads129xr_driver_api = {
	// .trigger_set = ads129xr_trigger_set,
	.channel_get = ads129xr_channel_get,
};


//创建管理数据和配置数据的宏
#define ADS129XR_INST(inst)										\
    static struct ads129xr_data ads129xr_data_##inst = {        \
        /* initialize RAM values as needed, e.g.: */            \
        .freq = DT_INST_PROP(inst, spi_max_frequency),          \
    };                                                          \
	const struct ads129xr_config ads129xr_cfg_##inst = {		\
	    /* initialize ROM values as needed. */                  \
		.start_gpio_spec = GPIO_DT_SPEC_INST_GET(inst, start_gpios),	\
		.ready_gpio_spec = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios),	\
		.reset_gpio_spec = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),	\
		.pwdwn_gpio_spec = GPIO_DT_SPEC_INST_GET(inst, pwdn_gpios),		\
		.ledpw_gpio_spec = GPIO_DT_SPEC_INST_GET(inst, ledpw_gpios),	\
		.spi_cfg = SPI_CONFIG_DT_INST(inst,			\
					   ADS129XR_SPI_OPERATION,	\
					   0),				\
    };                                                          \
    DEVICE_DT_INST_DEFINE(inst,				\
		ads129xr_init,						\
		NULL,								\
		&ads129xr_data_##inst,				\
		&ads129xr_cfg_##inst,				\
		POST_KERNEL,						\
		CONFIG_SENSOR_INIT_PRIORITY,		\
		&ads129xr_driver_api);


/* Call the device creation macro for each instance: */
DT_INST_FOREACH_STATUS_OKAY(ADS129XR_INST);

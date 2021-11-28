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
	// .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,
	.operation = SPI_WORD_SET(8) | SPI_MODE_CPHA | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB,
	.frequency = 4000000,
	.slave = 0,
	.cs = SPI_CS_CONTROL_PTR_DT(DT_NODELABEL(ads129xr0), 2),
};

int ads129xr_spi_transceive(const struct device *dev, uint8_t buffer)
{
	struct ads129xr_data *drv_data = dev->data;
	// drv_data->spi = device_get_binding(DT_INST_BUS_LABEL(0));

	uint8_t tx_buffer[1] = {buffer};
	uint8_t rx_buffer[1];

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = 1
	};

	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = 1,
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	int err = spi_transceive(drv_data->spi, &spi_cfg, &tx, &rx);

	if (err) {
		printk("SPI error: %d\n", err);
	} else {
		printk("SPI sent/received: 0x%x/0x%x\n", tx_buffer[0], rx_buffer[0]);
	};

	return err;
};

int ads129xr_init(const struct device *dev)
{
	const struct ads129xr_config *drv_cfg = dev->config;

	int ret = gpio_pin_configure(drv_cfg->pwdwn_gpio_spec.port, drv_cfg->pwdwn_gpio_spec.pin, GPIO_OUTPUT_INACTIVE | drv_cfg->pwdwn_gpio_spec.dt_flags);
	// ret = gpio_pin_set(drv_cfg->pwdwn_gpio_spec.port, drv_cfg->pwdwn_gpio_spec.pin, 1);

	ret = gpio_pin_configure(drv_cfg->reset_gpio_spec.port, drv_cfg->reset_gpio_spec.pin, GPIO_OUTPUT_INACTIVE | drv_cfg->reset_gpio_spec.dt_flags);
	// ret = gpio_pin_set(drv_cfg->reset_gpio_spec.port, drv_cfg->reset_gpio_spec.pin, 1);

	ret = gpio_pin_configure(drv_cfg->ledpw_gpio_spec.port, drv_cfg->ledpw_gpio_spec.pin, GPIO_OUTPUT_ACTIVE | drv_cfg->ledpw_gpio_spec.dt_flags);
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

	k_msleep(1000);	// 确保ADS129xR加电后到正常状态
	ads129xr_spi_transceive(dev, SDATAC);

	return 0;
};


static int ads129xr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	// struct ads129xr_data *drv_data = dev->data;
	// const struct ads129xr_config *drv_cfg = dev->config;
	// int32_t acc;

	ads129xr_spi_transceive(dev, SDATAC);
	ads129xr_spi_transceive(dev, RREG | ID);
	ads129xr_spi_transceive(dev, 0x00);
	ads129xr_spi_transceive(dev, 0x00);

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

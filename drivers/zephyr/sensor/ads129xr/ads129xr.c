/* ads129xr.c - ADS1294R ADS1296R ADS1298R ECG acqusition spi device */

/*
 * Copyright (c) 2021 scottwan@foxmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

#include "ads129xr.h"

// Compatible with "ti,ads129xr"

#define DT_DRV_COMPAT ti_ads129xr

struct ads129xr_config {
    const uint8_t start_pin;    //开始AD转换，高电平有效
	const uint8_t ready_pin;    //AD转换数据准备好，低电平有效
    const uint8_t reset_pin;    //寄存器重置，低电平有效
    const uint8_t pwd_pin;      //关闭电源，低电平有效
};

struct ads129xr_data {
	uint32_t freq; /* initial clock frequency in Hz */
};

int ads129xr_init(const struct device *dev)
{
    // GPIO的配置
    // GPIO中断安装
    // 旋转编码器GPIO初始化状态读取
    // 驱动初始化状态设置
    // 驱动线程创建
    // 使能中断
	return 0;
};


static int ads129xr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	// struct ads129xr_data *drv_data = dev->data;
	// const struct ads129xr_config *drv_cfg = dev->config;
	// int32_t acc;

	printk("Hello World in ADS129XR Driver !!! %s\n", CONFIG_BOARD);

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
		.start_pin = DT_INST_GPIO_PIN(inst, start_gpios),			\
        .ready_pin = DT_INST_GPIO_PIN(inst, drdy_gpios),			\
        .reset_pin = DT_INST_GPIO_PIN(inst, drdy_gpios),			\
        .pwd_pin = DT_INST_GPIO_PIN(inst, drdy_gpios),				\
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

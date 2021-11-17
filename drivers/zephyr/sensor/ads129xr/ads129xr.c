/* ads129xr.c - ADS1294R ADS1296R ADS1298R ECG acqusition spi device */

/*
 * Copyright (c) 2021 scottwan@foxmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Compatible with "ti,ads129xr"

#define DT_DRV_COMPAT ti_ads129xr

#include <kernel.h>
#include <drivers/sensor.h>

#include "ads129xr.h"

struct ads129xr_config {
    const uint8_t start_pin;    //开始AD转换
	const uint8_t ready_pin;    //AD转换数据准备好
    const uint8_t reset_pin;    //重启
    const uint8_t pwd_pin;      //关闭电源
};

struct ads129xr_data {

};

//创建管理数据和配置数据的宏
#define ADS129XR_INST(n)											      \
	struct ads129xr_data ads129xr_data_##n;								      \
	const struct ads129xr_config ads129xr_cfg_##n = {							      \
		.start_pin = DT_INST_GPIO_PIN(n, start_gpios),							      \
        .ready_pin = DT_INST_GPIO_PIN(n, drdy_gpios),							      \
        .reset_pin = DT_INST_GPIO_PIN(n, drdy_gpios),							      \
        .pwd_pin = DT_INST_GPIO_PIN(n, drdy_gpios),							      \
	};		

//根据设备树对node进行初始化，会从设备树中读取硬件信息放在struct ads129x_config变量中
DT_INST_FOREACH_STATUS_OKAY(ADS129XR_INST);

int ads129xr_init(const struct device *dev)
{
    // GPIO的配置
    // GPIO中断安装
    // 旋转编码器GPIO初始化状态读取
    // 驱动初始化状态设置
    // 驱动线程创建
    // 使能中断
};


static int ads129xr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	// struct ads129xr_data *drv_data = dev->data;
	// const struct ads129xr_config *drv_cfg = dev->config;
	// int32_t acc;

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

	return 0;
}

static const struct sensor_driver_api ads129xr_driver_api = {
	// .trigger_set = ads129xr_trigger_set,
	.channel_get = ads129xr_channel_get,
};

//注册驱动
DEVICE_AND_API_INIT(ads129xr_##n, DT_INST_LABEL(n), ads129xr_init, &ads129xr_data_##n, &ads129xr_cfg_##n, \
			    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ads129xr_driver_api);
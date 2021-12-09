/* ads129xr.c - ADS1294R ADS1296R ADS1298R ECG acqusition spi device */

/*
 * Copyright (c) 2021 scottwan@foxmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/printk.h>
#include <drivers/sensor.h>

#include "ads129xr.h"

// Compatible with "ti,ads129xr"
#define DT_DRV_COMPAT ti_ads129xr

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
	const struct ads129xr_config *drv_cfg = dev->config;

	// If WREG sent, opcode[0] >> 5 == 0x02 must be 1
	bool isWREG = (opcode[0] >> 5 == 0x02);
	bool isRREG = (opcode[0] >> 5 == 0x01);
	bool isReadData = (opcode[0] == RDATA) || (opcode[0] == RDATAC);
	
	printk("\n------------------------------");
	printk("\nDevice Name:  %s", dev->name);
	printk("\nSPI sent:     0x");
	for (int i = 0; i < op_length; i++)
	{
		printk("%02x ", opcode[i]);
	}

	isWREG ? printk(" %02x", data[0]) : "";
	printk("\n");

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
		.count = isWREG ? 2 : 1
	};

	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = isWREG ? 0 : 2
	};

	// spi_transceive() return:
	// frames – Positive number of frames received in slave mode.
	// 0 – If successful in master mode.
	// -errno – Negative errno code on failure.
	int err = spi_transceive(drv_data->spi, &drv_cfg->spi_cfg, &tx, &rx);

	if (err == 0) {

		if(isRREG || isReadData) // if read register then print the data received
		{
			printk("\nSPI received: 0x");
			for (int i = 0; i < data_length; i++)
			{
				printk("%02x ", data[i]);
			}
			printk("\n");
		}
		
	} else {

		printk("\nSPI error: %d\n", err);
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

	struct ads129xr_data *drv_data = dev->data;
	drv_data->spi = device_get_binding(DT_INST_BUS_LABEL(0));

	// 停止缺省的连续读数模式
	uint8_t opcode[1] = {SDATAC}, data[0]; //SDATAC: 0x11
	ads129xr_spi_transceive(dev, opcode, 1, data, 0);

	k_msleep(1);

	// 设置内部时钟输出给级联模式中的第二片芯片
	uint8_t opcode_wreg[2] = {WREG | CONFIG1, 0x00};
	uint8_t wreg_data[1] = {LOW_POWR_250_SPS | CLK_EN};	//0x26
	ads129xr_spi_transceive(dev, opcode_wreg, sizeof(opcode_wreg), wreg_data, sizeof(wreg_data));

	// 使用内部参考 Enable internal reference
	opcode_wreg[0] = WREG | CONFIG3;
	wreg_data[0] = CONFIG3_const | PD_REFBUF; //0xc0;
	ads129xr_spi_transceive(dev, opcode_wreg, sizeof(opcode_wreg), wreg_data, sizeof(wreg_data));

	// k_msleep(1000);

	// 测试读取前2个寄存器的结果
	// uint8_t opcode2[2] = {RREG, 0x01}, data2[2];	
	// ads129xr_spi_transceive(dev, opcode2, sizeof(opcode2), data2, sizeof(data2));

	// ads129xr_trigger_mode_init(dev);

	return 0;
};


static int ads129xr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	uint8_t wreg_opcode[2] = {0x00, 0x00};
	uint8_t wreg_data[1];

	// 设置发送测试信号
	wreg_opcode[0] = WREG | CONFIG2;
	wreg_data[0] = CONFIG2_const | INT_TEST; //0x10;
	ads129xr_spi_transceive(dev, wreg_opcode, sizeof(wreg_opcode), wreg_data, sizeof(wreg_data));

	// 设置通道输入测试信号
	wreg_opcode[0] = WREG | CH1SET; //0x45
	wreg_opcode[1] = 0x03;
	// wreg_data[0] = CHnSET_const | TEST_SIGNAL; //0x05;
	uint8_t wreg_data_8[8] = {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05};
	uint8_t wreg_data_4[4] = {0x05, 0x05, 0x05, 0x05};
	ads129xr_spi_transceive(dev, wreg_opcode, sizeof(wreg_opcode), wreg_data_4, sizeof(wreg_data_4));

	// uint8_t opcode_wreg[2] = {0x00,0x00};
	// uint8_t wreg_data[1];

	// // WREG CONFIG1 0x86
	// opcode_wreg[0] = WREG | CONFIG1;
	// wreg_data[0] = 0x86;
	// ads129xr_spi_transceive(dev, opcode_wreg, sizeof(opcode_wreg), wreg_data, sizeof(wreg_data));

	// // WREG CONFIG2 0x00
	// opcode_wreg[0] = WREG | CONFIG2;
	// wreg_data[0] = 0x00;
	// ads129xr_spi_transceive(dev, opcode_wreg, sizeof(opcode_wreg), wreg_data, sizeof(wreg_data));

	// // WREG CHnSET 0x01
	// opcode_wreg[0] = WREG | CH1SET;
	// opcode_wreg[1] = 0x07;
	// uint8_t wreg_data_8[8] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
	// ads129xr_spi_transceive(dev, opcode_wreg, sizeof(opcode_wreg), wreg_data_8, sizeof(wreg_data_8));


	// 测试读取前n个寄存器的结果
	uint8_t rreg_opcode[2] = {RREG, 0x0c};
	uint8_t rreg_data[13];	
	ads129xr_spi_transceive(dev, rreg_opcode, sizeof(rreg_opcode), rreg_data, sizeof(rreg_data));



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
	.trigger_set = ads129xr_trigger_set,
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

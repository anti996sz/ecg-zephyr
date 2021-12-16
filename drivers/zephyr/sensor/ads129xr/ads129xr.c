/* ads129xr.c - ADS1294R ADS1296R ADS1298R ECG acqusition spi device */

/*
 * Copyright (c) 2021 scottwan@foxmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/printk.h>
#include <drivers/sensor.h>
#include <string.h>

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
int ads129xr_spi_transceive(const struct device *dev, 
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
	bool isReadData = (opcode[0] == RDATA) || (op_length == 0); // op_length: RDATAC
	
	printk("\n------------------------------");
	printk("\nDevice Name:  %s", dev->name);
	printk("\nSPI sent:     0x");
	for (int i = 0; i < op_length; i++)
	{
		printk("%02x ", opcode[i]);
	}

	if(isWREG){
		for (int i = 0; i < data_length; i++)
		{
			printk("%02x ", data[i]);
		}
		
	}
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

	if (ret < 0) {
		printk("ads1298 gpio init error");
		return -1;
	}

	struct ads129xr_data *drv_data = dev->data;
	drv_data->spi = device_get_binding(DT_INST_BUS_LABEL(0));

	// 停止缺省的连续读数模式
	uint8_t opcode[1] = {SDATAC}, data[0]; //SDATAC: 0x11
	ads129xr_spi_transceive(dev, opcode, 1, data, 0);

	k_msleep(1);

	// 设置寄存器
	uint8_t wreg_opcode[2] = {WREG | CONFIG1, 11};
	uint8_t wreg_data[12] = {
		CONFIG1_const | LOW_POWR_250_SPS | CLK_EN, 
		CONFIG2_const | INT_TEST, 
		CONFIG3_const | PD_REFBUF,
		CONFIG4_const,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10,
		CH1SET_const | TEST_SIGNAL | GAIN10
	};

	ads129xr_spi_transceive(dev, wreg_opcode, sizeof(wreg_opcode), wreg_data, sizeof(wreg_data));

	// 测试读取前n个寄存器的结果
	uint8_t rreg_opcode[2] = {RREG, 12};
	uint8_t rreg_data[13];	
	ads129xr_spi_transceive(dev, rreg_opcode, sizeof(rreg_opcode), rreg_data, sizeof(rreg_data));

	// start conversion see ads129xr_trigger_set(), a unknow reason cause restart if set start conversion here.
	// another reason is the second ads129xr not prsent when start conversion here.

	return 0;
};


static int ads129xr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	// uint8_t opcode[0]; //= {RDATAC};
	uint8_t opcode[1] = {RDATA};

	int64_t temp;
	const uint64_t max_pos_input = 0x7FFFFF; // positive full-scale input
	const uint64_t min_neg_input = 0xFFFFFF; // negitive minimum input
	double volt; // reale volt output

	if(strcmp(dev->name, "ADS1298R") == 0){
		
		uint8_t data_9[27];	// receive ads1298r RDATAC data 24*9 = 216 bits
		ads129xr_spi_transceive(dev, opcode, sizeof(opcode), data_9, sizeof(data_9));
		val[0].val1 = (data_9[0] << 16) | (data_9[1] << 8) | data_9[2];

		for (size_t i = 1; i < 9; i++)
		{
			temp = (data_9[i*3] << 16) | (data_9[i*3+1] << 8) | data_9[i*3+2];
			temp = temp < max_pos_input ? temp : temp - min_neg_input;
			volt = temp * 2.4 / max_pos_input * 1000; // set unit to mV

			sensor_value_from_double(&val[i], volt);
		}
		
	} else {

		uint8_t data_5[15];	// receive ads1294r RDATAC data 24*5 = 120 bits
		ads129xr_spi_transceive(dev, opcode, sizeof(opcode), data_5, sizeof(data_5));
		val[0].val1 = (data_5[0] << 16) | (data_5[1] << 8) | data_5[2];

		for (size_t i = 1; i < 5; i++)
		{
			temp = (data_5[i*3] << 16) | (data_5[i*3+1] << 8) | data_5[i*3+2];
			temp = temp < max_pos_input ? temp : temp - min_neg_input;
			volt = temp * 2.4 / max_pos_input * 1000; // set unit to mV

			sensor_value_from_double(&val[i], volt);
		}
	}

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

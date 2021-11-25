#include <device.h>
#include <sys/util.h>

#ifndef ADS129XR_H
#define ADS129XR_H

struct ads129xr_config {
    const uint8_t start_pin;    //开始AD转换，高电平有效
	const uint8_t ready_pin;    //AD转换数据准备好，低电平有效
    const uint8_t reset_pin;    //寄存器重置，低电平有效
    const uint8_t pwd_pin;      //关闭电源，低电平有效
};

struct ads129xr_data {
	const struct device *spi;
	uint32_t freq; /* initial clock frequency in Hz */
};

#endif
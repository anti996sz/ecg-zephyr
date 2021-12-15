#include <kernel.h>
#include <sys/printk.h>
#include <drivers/sensor.h>

#include "ads129xr.h"

// Compatible with "ti,ads129xr"
#define DT_DRV_COMPAT ti_ads129xr

static K_THREAD_STACK_DEFINE(ads129xr_thread_stack,
			     CONFIG_ADS129XR_THREAD_STACK_SIZE);
static struct k_thread ads129xr_thread;


static void ads129xr_thread_cb(const struct device *dev)
{
	struct ads129xr_data *drv_data = dev->data;

    static struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (drv_data->drdy_handler) {
		drv_data->drdy_handler(dev, &drdy_trigger);
	}
}


static void ads129xr_thread_main(void *dev_void, void *unused1, void *unused2)
{
    const struct device *dev = (const struct device *)dev_void;
    struct ads129xr_data *drv_data = dev->data;

    ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		ads129xr_thread_cb(dev);
	}
}


static void ads129xr_gpio_callback(const struct device *port,
				 struct gpio_callback *cb,
				 uint32_t pin)
{
	struct ads129xr_data *drv_data =
		CONTAINER_OF(cb, struct ads129xr_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

    k_sem_give(&drv_data->gpio_sem);
}


int ads129xr_trigger_mode_init(const struct device *dev)
{
    struct ads129xr_data *drv_data = dev->data;
    const struct ads129xr_config *drv_cfg = dev->config;

    if (!device_is_ready(drv_cfg->ready_gpio_spec.port)) {
		printk("INT device is not ready");
		return -ENODEV;
	}

    k_sem_init(&drv_data->gpio_sem, 0, 1);

	k_thread_create(
		&ads129xr_thread,
		ads129xr_thread_stack,
		CONFIG_ADS129XR_THREAD_STACK_SIZE,
		ads129xr_thread_main,
		(void *)dev,
		NULL,
		NULL,
		K_PRIO_COOP(CONFIG_ADS129XR_THREAD_PRIORITY),
		0,
		K_NO_WAIT);

    gpio_pin_configure(drv_cfg->ready_gpio_spec.port,
			   drv_cfg->ready_gpio_spec.pin,
			   GPIO_INPUT | drv_cfg->ready_gpio_spec.dt_flags);

	gpio_init_callback(&drv_data->gpio_cb,
			   ads129xr_gpio_callback,
			   BIT(drv_cfg->ready_gpio_spec.pin));

	gpio_add_callback(drv_cfg->ready_gpio_spec.port, &drv_data->gpio_cb);

	gpio_pin_interrupt_configure(drv_cfg->ready_gpio_spec.port,
				     drv_cfg->ready_gpio_spec.pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}


int ads129xr_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	// 设置中断触发器，中断触发器只需要初始化一次
	static bool trigger_inited = false;
	if(!trigger_inited){

		// enabel RDATAC mode
		// uint8_t opcode[1] = {RDATAC};
		// ads129xr_spi_transceive(dev, opcode, 1, NULL, 0);

		// start conversion
		const struct ads129xr_config *drv_cfg = dev->config;
		int ret = gpio_pin_configure(drv_cfg->start_gpio_spec.port, 
							drv_cfg->start_gpio_spec.pin, 
							GPIO_OUTPUT_ACTIVE | drv_cfg->start_gpio_spec.dt_flags);

		if (ret < 0) {
			printk("ads1298 gpio init error");
			return -1;
		}

		ads129xr_trigger_mode_init(dev);	
		trigger_inited = true;
	}

    struct ads129xr_data *drv_data = dev->data;

    if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

    drv_data->drdy_handler = handler;

	return 0;
}
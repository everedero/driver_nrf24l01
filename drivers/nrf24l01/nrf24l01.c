/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nrf24l01

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include "nrf24l01.h"

LOG_MODULE_REGISTER(nrf24l01, CONFIG_NRF24L01_LOG_LEVEL);

struct nrf24l01_config {
	struct gpio_dt_spec ce;
	struct gpio_dt_spec irq;
};

struct nrf24l01_data {
	int addr_width;
};

static int nrf24l01_read(const struct device *dev) 
{
	return(0);
}

static int nrf24l01_write(const struct device *dev) 
{
	return(0);
}

static const struct nrf24l01_api nrf24l01_api = {
	.read = nrf24l01_read,
	.write = nrf24l01_write,
};

static int nrf24l01_init(const struct device *dev)
{
	const struct nrf24l01_config *config = dev->config;
	int ret;


	ret = gpio_pin_configure_dt(&config->ce, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure CE GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->irq, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure IRQ GPIO (%d)", ret);
		return ret;
	}

	return 0;
}

#define NRF24L01_DEFINE(i)                                             \
	static const struct nrf24l01_config nrf24l01_config_##i = {        \
		.ce = GPIO_DT_SPEC_INST_GET(i, ce_gpio),                              \
		.irq = GPIO_DT_SPEC_INST_GET(i, irq_gpio)                              \
	};                                                                     \
                                                                               \
	static struct nrf24l01_data nrf24l01_##i = {                            \
		.addr_width = DT_INST_PROP(i, addr_width)                            \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(i, nrf24l01_init, NULL, NULL,                  \
			      &nrf24l01_config_##i, POST_KERNEL,             \
			      CONFIG_NRF24L01_INIT_PRIORITY, &nrf24l01_api);

DT_INST_FOREACH_STATUS_OKAY(NRF24L01_DEFINE)

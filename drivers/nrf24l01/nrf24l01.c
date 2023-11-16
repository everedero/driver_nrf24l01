/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf24l01

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <app/drivers/nrf24.h>
#include "nrf24l01_defines.h"

LOG_MODULE_REGISTER(nrf24l01, CONFIG_NRF24L01_LOG_LEVEL);

struct nrf24l01_config {
	const struct spi_dt_spec spi;
	struct gpio_dt_spec ce;
	struct gpio_dt_spec irq;
};

struct nrf24l01_data {
	int addr_width;
	int channel_frequency;
	bool data_rate_2mbps;
	int rf_power_attenuation;
	bool lna_gain;
	bool crc_encoding_twobytes;
	uint8_t tx_address[5];
	bool dynamic_payload;
	bool payload_ack;
	int rx_datapipes_number;
	uint8_t rx_datapipe0_address[5];
	uint8_t rx_datapipe1_address[5];
	uint8_t rx_datapipe2_address;
	uint8_t rx_datapipe3_address;
	uint8_t rx_datapipe4_address;
	uint8_t rx_datapipe5_address;
	uint8_t rx_datapipes_dynamic_payload[6];
	uint8_t rx_datapipes_fixed_size_payload[6];
};

uint8_t nrf24l01_write_register(const struct device *dev, uint8_t reg, uint8_t data)
{
	const struct nrf24l01_config *config = dev->config;
	uint8_t tx_data[2];
	uint8_t rx_data;
	int ret;
	const struct spi_buf buf[2] = {
		{
			.buf = tx_data,
			.len = 2
		}, {
			.buf = &rx_data,
			.len = 1
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
	};
	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = 2
	};

	// 5 lower bits for address, the 6th is 0 for read and 1 for write
	tx_data[0] = ( W_REGISTER | ( REGISTER_MASK & reg ) );
	tx_data[1] = data;

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("Error transceive %d\n", ret);
		return 0;
	}
	return rx_data;
}

uint8_t nrf24l01_read_register(const struct device *dev, uint8_t reg)
{
	const struct nrf24l01_config *config = dev->config;
	uint8_t tx_data[2];
	uint8_t rx_data[2];
	int ret;
	const struct spi_buf buf[2] = {
		{
			.buf = tx_data,
			.len = 2
		}, {
			.buf = rx_data,
			.len = 2
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
	};
	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = 2
	};

	// 5 lower bits for address, the 6th is 0 for read and 1 for write
	tx_data[0] = ( R_REGISTER | ( REGISTER_MASK & reg ) );
	tx_data[1] = RF24_NOP;

	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		LOG_ERR("Error transceive %d\n", ret);
		return 0;
	}

	// status is 1st byte of receive buffer
	LOG_DBG("0x%x: 0x%x 0x%x\n", reg, rx_data[0], rx_data[1]);
	return rx_data[1];
}

uint8_t nrf24l01_set_channel(const struct device *dev)
{
	const struct nrf24l01_config *config = dev->config;
	const struct nrf24l01_data *data = dev->data;
	const uint8_t max_channel = 125;
	return nrf24l01_write_register(dev, RF_CH, MIN(data->channel_frequency, max_channel));
}

uint8_t nrf24l01_get_channel(const struct device *dev)
{
	return nrf24l01_read_register(dev, RF_CH);
}

static int nrf24l01_read(const struct device *dev, uint8_t *buffer)
{
	return(0);
}

static int nrf24l01_write(const struct device *dev, uint8_t *buffer)
{
	return(0);
}

static const struct nrf24_api nrf24l01_api = {
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

	ret = nrf24l01_set_channel(dev);

	return 0;
}

#define NRF24L01_SPI_MODE   (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

#define NRF24L01_DEFINE(i)                                             \
	static const struct nrf24l01_config nrf24l01_config_##i = {        \
		.spi = SPI_DT_SPEC_INST_GET(i, NRF24L01_SPI_MODE, 0),           \
		.ce = GPIO_DT_SPEC_INST_GET_OR(i, ce_gpios, {}),               \
		.irq = GPIO_DT_SPEC_INST_GET_OR(i, irq_gpios, {}),             \
	};                                                                            \
                                                                                  \
	static struct nrf24l01_data nrf24l01_##i = {                                  \
		.addr_width = DT_INST_PROP(i, addr_width),                                \
		.channel_frequency = DT_INST_PROP(i, channel_frequency),                  \
		.data_rate_2mbps = DT_INST_PROP_OR(i, data_rate_2mbps, false),            \
		.rf_power_attenuation = DT_INST_PROP(i, rf_power_attenuation),            \
		.lna_gain = DT_INST_PROP_OR(i, lna_gain, false),                          \
		.crc_encoding_twobytes = DT_INST_PROP_OR(i, crc_encoding_twobytes, false),\
		.tx_address = DT_INST_PROP(i, tx_address),                                \
		.dynamic_payload = DT_INST_PROP_OR(i, dynamic_payload, false),            \
		.payload_ack = DT_INST_PROP_OR(i, payload_ack, false),                    \
		.rx_datapipes_number = DT_INST_PROP(i, payload_ack),                      \
		.rx_datapipe0_address = DT_INST_PROP(i, rx_datapipe0_address),            \
		.rx_datapipe1_address = DT_INST_PROP(i, rx_datapipe1_address),            \
		.rx_datapipe2_address = DT_INST_PROP(i, rx_datapipe2_address),           \
		.rx_datapipe3_address = DT_INST_PROP(i, rx_datapipe3_address),            \
		.rx_datapipe4_address = DT_INST_PROP(i, rx_datapipe4_address),            \
		.rx_datapipe5_address = DT_INST_PROP(i, rx_datapipe5_address),            \
		.rx_datapipes_dynamic_payload = DT_INST_PROP_OR(i,                        \
				rx_datapipes_dynamic_payload, {}),                                \
		.rx_datapipes_fixed_size_payload = DT_INST_PROP_OR(i,                     \
				rx_datapipes_fixed_size_payload, {}),                             \
	};                                                                            \
	DEVICE_DT_INST_DEFINE(i, nrf24l01_init, NULL, &nrf24l01_##i,  \
			      &nrf24l01_config_##i, POST_KERNEL,              \
			      CONFIG_NRF24L01_INIT_PRIORITY, &nrf24l01_api);

DT_INST_FOREACH_STATUS_OKAY(NRF24L01_DEFINE)

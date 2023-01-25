/**
 * Copyright 2023 Legytma Soluções Inteligentes LTDA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * ethernet_driver.h
 *
 *  Created on: 23 de ago de 2022
 *      Author: Alex Manoel Ferreira Silva
 */

#pragma once

#include <stdint.h>

/*#if CONFIG_ETH_USE_SPI_ETHERNET
	#include "driver/spi_common.h"
	#include "driver/spi_master.h"
#endif // CONFIG_ETH_USE_SPI_ETHERNET

#include "esp_netif_types.h"
#include "esp_netif_defaults.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "esp_eth.h"
#include "esp_mac.h"
#include "esp_netif.h"*/

#include "driver/gpio.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#if CONFIG_ETH_USE_SPI_ETHERNET
	#include "driver/spi_master.h"
#endif // CONFIG_ETH_USE_SPI_ETHERNET

#include "sdkconfig.h"

#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
	#define ETHERNET_DRIVER_CONFIG_INTERNAL_DEFAULT() \
		{ .netif = NULL, .eth_mac = NULL, .eth_phy = NULL, .eth_handle = NULL, }
/*
 .netif_config   = ESP_NETIF_DEFAULT_ETH(),  \
			 .eth_mac_config = ETH_MAC_DEFAULT_CONFIG(), \
			 .eth_phy_config = ETH_PHY_DEFAULT_CONFIG(), \
			 */
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET

#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
	#if CONFIG_ETHERNET_DRIVER_USE_KSZ8851SNL
		#define ETHERNET_DRIVER_CONFIG_SPI_SPECIFIC                     \
			{                                                           \
				.mode = 0,                                              \
				.clock_speed_hz =                                       \
					CONFIG_ETHERNET_DRIVER_SPI_CLOCK_MHZ * 1000 * 1000, \
				.queue_size = 20                                        \
			}
	#elif CONFIG_ETHERNET_DRIVER_USE_DM9051
		#define ETHERNET_DRIVER_CONFIG_SPI_SPECIFIC                     \
			{                                                           \
				.command_bits = 1, .address_bits = 7, .mode = 0,        \
				.clock_speed_hz =                                       \
					CONFIG_ETHERNET_DRIVER_SPI_CLOCK_MHZ * 1000 * 1000, \
				.queue_size = 20                                        \
			}
	#elif CONFIG_ETHERNET_DRIVER_USE_W5500
		#define ETHERNET_DRIVER_CONFIG_SPI_SPECIFIC                     \
			{                                                           \
				.command_bits = 16, .address_bits = 8, .mode = 0,       \
				.clock_speed_hz =                                       \
					CONFIG_ETHERNET_DRIVER_SPI_CLOCK_MHZ * 1000 * 1000, \
				.queue_size = 20, .flags = SPI_DEVICE_NO_DUMMY,         \
			}
	#endif

	#define ETHERNET_DRIVER_CONFIG_SPI_DEFAULT()                            \
		{                                                                   \
			.netif_config =                                                 \
				{                                                           \
					.base  = NULL,                                          \
					.stack = ESP_NETIF_NETSTACK_DEFAULT_ETH,                \
				},                                                          \
			.netif =                                                        \
				{                                                           \
					NULL,                                                   \
				},                                                          \
			.device_handle =                                                \
				{                                                           \
					NULL,                                                   \
				},                                                          \
			.bus_config =                                                   \
				{                                                           \
					.miso_io_num   = CONFIG_ETHERNET_DRIVER_SPI_MISO_GPIO,  \
					.mosi_io_num   = CONFIG_ETHERNET_DRIVER_SPI_MOSI_GPIO,  \
					.sclk_io_num   = CONFIG_ETHERNET_DRIVER_SPI_SCLK_GPIO,  \
					.quadwp_io_num = -1,                                    \
					.quadhd_io_num = -1,                                    \
				},                                                          \
			.device_interface_config = ETHERNET_DRIVER_CONFIG_SPI_SPECIFIC, \
			.eth_mac =                                                      \
				{                                                           \
					NULL,                                                   \
				},                                                          \
			.eth_phy =                                                      \
				{                                                           \
					NULL,                                                   \
				},                                                          \
			.eth_handle = {                                                 \
				NULL,                                                       \
			},                                                              \
		}
/*
 .netif_inherent_config = ESP_NETIF_INHERENT_DEFAULT_ETH(),      \
			 .eth_mac_config          = ETH_MAC_DEFAULT_CONFIG(),            \
			 .eth_phy_config = ETH_PHY_DEFAULT_CONFIG(), \
			  */

	#define INIT_SPI_ETHERNET_MODULE_CONFIG(spi_module_config, num) \
		do {                                                        \
			spi_module_config[num].spi_cs_gpio =                    \
				CONFIG_ETHERNET_DRIVER_SPI_CS##num##_GPIO;          \
			spi_module_config[num].int_gpio =                       \
				CONFIG_ETHERNET_DRIVER_SPI_INT##num##_GPIO;         \
			spi_module_config[num].phy_reset_gpio =                 \
				CONFIG_ETHERNET_DRIVER_SPI_PHY_RST##num##_GPIO;     \
			spi_module_config[num].phy_addr =                       \
				CONFIG_ETHERNET_DRIVER_SPI_PHY_ADDR##num;           \
		} while (0)
#endif // CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET

#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
typedef struct ethernet_driver_internal_config_s {
	esp_netif_config_t netif_config;
	esp_netif_t       *netif;
	eth_mac_config_t   eth_mac_config;
	esp_eth_mac_t     *eth_mac;
	eth_phy_config_t   eth_phy_config;
	esp_eth_phy_t     *eth_phy;
	esp_eth_config_t   eth_config;
	esp_eth_handle_t   eth_handle;
} ethernet_driver_internal_config_t;
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET

#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
typedef struct ethernet_driver_spi_module_config_s {
	uint8_t spi_cs_gpio;
	uint8_t int_gpio;
	int8_t  phy_reset_gpio;
	uint8_t phy_addr;
} ethernet_driver_spi_module_config_t;

typedef struct ethernet_driver_spi_config_s {
	esp_netif_inherent_config_t netif_inherent_config;
	esp_netif_config_t          netif_config;
	esp_netif_t				*netif[CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM];
	spi_device_handle_t device_handle[CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM];
	#if CONFIG_ETHERNET_DRIVER_USE_KSZ8851SNL
	eth_ksz8851snl_config_t device_config;
	#elif CONFIG_ETHERNET_DRIVER_USE_DM9051
	eth_dm9051_config_t device_config;
	#elif CONFIG_ETHERNET_DRIVER_USE_W5500
	eth_w5500_config_t device_config;
	#endif
	spi_bus_config_t bus_config;
	ethernet_driver_spi_module_config_t
		module_config[CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM];
	spi_device_interface_config_t device_interface_config;
	eth_mac_config_t              eth_mac_config;
	esp_eth_mac_t   *eth_mac[CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM];
	eth_phy_config_t eth_phy_config;
	esp_eth_phy_t   *eth_phy[CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM];
	esp_eth_config_t eth_config;
	esp_eth_handle_t eth_handle[CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM];
} ethernet_driver_spi_config_t;
#endif // CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET

typedef struct ethernet_driver_config_s {
#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
	ethernet_driver_internal_config_t internal_config;
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
	ethernet_driver_spi_config_t spi_config;
#endif // CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
} ethernet_driver_config_t;

#ifdef __cplusplus
extern "C" {
#endif
void ethernet_driver_init(ethernet_driver_config_t *config);
#ifdef __cplusplus
}
#endif

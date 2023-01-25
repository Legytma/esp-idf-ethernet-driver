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
 * ethernet_driver.c
 *
 *  Created on: 23 de ago de 2022
 *      Author: Alex Manoel Ferreira Silva
 */

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#if CONFIG_ETH_USE_SPI_ETHERNET
	#include "driver/spi_common.h"
	#include "driver/spi_master.h"
#endif // CONFIG_ETH_USE_SPI_ETHERNET

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_netif_types.h"
#include "esp_netif_defaults.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"

#include "log_utils.h"

#include "ethernet_driver.h"

LOG_TAG("ethernet_driver");

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
							  int32_t event_id, void *event_data) {
	uint8_t mac_addr[6] = {0};
	/* we can get the ethernet driver handle from event data */
	esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

	switch (event_id) {
		case ETHERNET_EVENT_CONNECTED:
			esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
			LOGI("Ethernet Link Up");
			LOGI("Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0],
				 mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
				 mac_addr[5]);
			break;
		case ETHERNET_EVENT_DISCONNECTED:
			LOGI("Ethernet Link Down");
			break;
		case ETHERNET_EVENT_START:
			LOGI("Ethernet Started");
			break;
		case ETHERNET_EVENT_STOP:
			LOGI("Ethernet Stopped");
			break;
		default:
			break;
	}
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
								 int32_t event_id, void *event_data) {
	ip_event_got_ip_t         *event   = (ip_event_got_ip_t *)event_data;
	const esp_netif_ip_info_t *ip_info = &event->ip_info;

	LOGI("Ethernet Got IP Address");
	LOGI("~~~~~~~~~~~");
	LOGI("ETHIP:" IPSTR, IP2STR(&ip_info->ip));
	LOGI("ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
	LOGI("ETHGW:" IPSTR, IP2STR(&ip_info->gw));
	LOGI("~~~~~~~~~~~");
}

/*
static void main_app(void) {
	ethernet_driver_config_t config = {
#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
		.internal_config = ETHERNET_DRIVER_CONFIG_INTERNAL_DEFAULT(),
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
		.spi_config = ETHERNET_DRIVER_CONFIG_SPI_DEFAULT(),
#endif // CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
	};

#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
	config.internal_config.netif_config   = ESP_NETIF_DEFAULT_ETH();
	config.internal_config.eth_mac_config = ETH_MAC_DEFAULT_CONFIG();
	config.internal_config.eth_phy_config = ETH_PHY_DEFAULT_CONFIG();
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
	config.spi_config.netif_inherent_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
	config.spi_config.netif_config          = ESP_NETIF_DEFAULT_ETH();
	config.spi_config.eth_mac_config        = ETH_MAC_DEFAULT_CONFIG();
	config.spi_config.eth_phy_config        = ETH_PHY_DEFAULT_CONFIG();
#endif // CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET

	ethernet_driver_init(&config);
}
*/

void ethernet_driver_init(ethernet_driver_config_t *config) {
	// Initialize TCP/IP network interface (should be called only once in
	// application)
	ESP_ERROR_CHECK(esp_netif_init());
	// Create default event loop that running in background
	// ESP_ERROR_CHECK(esp_event_loop_create_default());

#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
	// Create new default instance of esp-netif for Ethernet
	config->internal_config.eth_netif =
		esp_netif_new(&config->internal_config.netif_config);

	// Init MAC and PHY configs to default
	config->internal_config.eth_phy_config.phy_addr =
		CONFIG_ETHERNET_DRIVER_PHY_ADDR;
	config->internal_config.eth_phy_config.reset_gpio_num =
		CONFIG_ETHERNET_DRIVER_PHY_RST_GPIO;
	config->internal_config.eth_mac_config.smi_mdc_gpio_num =
		CONFIG_ETHERNET_DRIVER_MDC_GPIO;
	config->internal_config.eth_mac_config.smi_mdio_gpio_num =
		CONFIG_ETHERNET_DRIVER_MDIO_GPIO;

	config->internal_config.eth_mac =
		esp_eth_mac_new_esp32(&config->internal_config.eth_mac_config);

	#if CONFIG_ETHERNET_DRIVER_PHY_IP101
	config->internal_config.eth_phy =
		esp_eth_phy_new_ip101(&config->internal_config.eth_phy_config);
	#elif CONFIG_ETHERNET_DRIVER_PHY_RTL8201
	config->internal_config.eth_phy =
		esp_eth_phy_new_rtl8201(&config->internal_config.eth_phy_config);
	#elif CONFIG_ETHERNET_DRIVER_PHY_LAN87XX
	config->internal_config.eth_phy =
		esp_eth_phy_new_lan87xx(&config->internal_config.eth_phy_config);
	#elif CONFIG_ETHERNET_DRIVER_PHY_DP83848
	config->internal_config.eth_phy =
		esp_eth_phy_new_dp83848(&config->internal_config.eth_phy_config);
	#elif CONFIG_EXAMPLE_ETH_PHY_KSZ80XX
	config->internal_config.eth_phy =
		esp_eth_phy_new_ksz80xx(&config->internal_config.eth_phy_config);
	#endif

	config->internal_config.eth_config = ETH_DEFAULT_CONFIG(
		config->internal_config.eth_mac, config->internal_config.eth_phy);

	ESP_ERROR_CHECK(
		esp_eth_driver_install(&config->internal_config.eth_config,
							   &config->internal_config.eth_handle));
	/* attach Ethernet driver to TCP/IP stack */
	ESP_ERROR_CHECK(esp_netif_attach(
		config->internal_config.eth_netif,
		esp_eth_new_netif_glue(config->internal_config.eth_handle)));
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET

#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
	// Create instance(s) of esp-netif for SPI Ethernet(s)
	config->spi_config.netif_config.base =
		&config->spi_config.netif_inherent_config;

	char if_key_str[10];
	char if_desc_str[10];
	char num_str[3];

	for (int i = 0; i < CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM; i++) {
		itoa(i, num_str, 10);
		strcat(strcpy(if_key_str, "ETH_SPI_"), num_str);
		strcat(strcpy(if_desc_str, "eth"), num_str);

		config->spi_config.netif_inherent_config.if_key     = if_key_str;
		config->spi_config.netif_inherent_config.if_desc    = if_desc_str;
		config->spi_config.netif_inherent_config.route_prio = 30 - i;

		config->spi_config.netif[i] =
			esp_netif_new(&config->spi_config.netif_config);
	}

	// Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
	ESP_ERROR_CHECK(gpio_install_isr_service(0));

	// Init SPI bus
	ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_ETHERNET_DRIVER_SPI_HOST,
									   &config->spi_config.bus_config,
									   SPI_DMA_CH_AUTO));

	// Init specific SPI Ethernet module configuration from Kconfig (CS GPIO,
	// Interrupt GPIO, etc.)
	INIT_SPI_ETHERNET_MODULE_CONFIG(config->spi_config.module_config, 0);
	#if CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM > 1
	INIT_SPI_ETHERNET_MODULE_CONFIG(config->spi_config.module_config, 1);
	#endif

	// Configure SPI interface and Ethernet driver for specific SPI module
	for (int i = 0; i < CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM; i++) {
	#if CONFIG_ETHERNET_DRIVER_USE_KSZ8851SNL
		// Set SPI module Chip Select GPIO
		config->spi_config.device_interface_config.spics_io_num =
			config->spi_config.module_config[i].spi_cs_gpio;

		// ESP_ERROR_CHECK(
		// 	spi_bus_add_device(CONFIG_ETHERNET_DRIVER_SPI_HOST,
		// 					   &config->spi_config.device_interface_config,
		// 					   &config->spi_config.device_handle[i]));
		// KSZ8851SNL ethernet driver is based on spi driver
		config->spi_config.device_config =
			ETH_KSZ8851SNL_DEFAULT_CONFIG(&config->spi_config.device_interface_config);

		// Set remaining GPIO numbers and configuration used by the SPI module
		config->spi_config.device_config.int_gpio_num =
			config->spi_config.module_config[i].int_gpio;
		config->spi_config.eth_phy_config.phy_addr =
			config->spi_config.module_config[i].phy_addr;
		config->spi_config.eth_phy_config.reset_gpio_num =
			config->spi_config.module_config[i].phy_reset_gpio;

		config->spi_config.eth_mac[i] =
			esp_eth_mac_new_ksz8851snl(&config->spi_config.device_config,
									   &config->spi_config.eth_mac_config);
		config->spi_config.eth_phy[i] =
			esp_eth_phy_new_ksz8851snl(&config->spi_config.eth_phy_config);
	#elif CONFIG_ETHERNET_DRIVER_USE_DM9051
		// Set SPI module Chip Select GPIO
		config->spi_config.device_interface_config.spics_io_num =
			config->spi_config.module_config[i].spi_cs_gpio;

		// ESP_ERROR_CHECK(
		// 	spi_bus_add_device(CONFIG_ETHERNET_DRIVER_SPI_HOST,
		// 					   &config->spi_config.device_interface_config,
		// 					   &config->spi_config.device_handle[i]));
		// dm9051 ethernet driver is based on spi driver
		config->spi_config.device_config =
			ETH_DM9051_DEFAULT_CONFIG(&config->spi_config.device_interface_config);

		// Set remaining GPIO numbers and configuration used by the SPI module
		config->spi_config.device_config.int_gpio_num =
			config->spi_config.module_config[i].int_gpio;
		config->spi_config.eth_phy_config.phy_addr =
			config->spi_config.module_config[i].phy_addr;
		config->spi_config.eth_phy_config.reset_gpio_num =
			config->spi_config.module_config[i].phy_reset_gpio;

		config->spi_config.eth_mac[i] =
			esp_eth_mac_new_dm9051(&config->spi_config.device_config,
								   &config->spi_config.eth_mac_config);
		config->spi_config.eth_phy[i] =
			esp_eth_phy_new_dm9051(&config->spi_config.eth_phy_config);
	#elif CONFIG_ETHERNET_DRIVER_USE_W5500
		// Set SPI module Chip Select GPIO
		config->spi_config.device_interface_config.spics_io_num =
			config->spi_config.module_config[i].spi_cs_gpio;

		// ESP_ERROR_CHECK(
		// 	spi_bus_add_device(CONFIG_ETHERNET_DRIVER_SPI_HOST,
		// 					   &config->spi_config.device_interface_config,
		// 					   &config->spi_config.device_interface_config));
		// w5500 ethernet driver is based on spi driver
		config->spi_config.device_config =
			(eth_w5500_config_t)ETH_W5500_DEFAULT_CONFIG(CONFIG_ETHERNET_DRIVER_SPI_HOST,
				&config->spi_config.device_interface_config);

		// Set remaining GPIO numbers and configuration used by the SPI module
		config->spi_config.device_config.int_gpio_num =
			config->spi_config.module_config[i].int_gpio;
		config->spi_config.eth_phy_config.phy_addr =
			config->spi_config.module_config[i].phy_addr;
		config->spi_config.eth_phy_config.reset_gpio_num =
			config->spi_config.module_config[i].phy_reset_gpio;

		config->spi_config.eth_mac[i] =
			esp_eth_mac_new_w5500(&config->spi_config.device_config,
								  &config->spi_config.eth_mac_config);
		config->spi_config.eth_phy[i] =
			esp_eth_phy_new_w5500(&config->spi_config.eth_phy_config);
	#endif // CONFIG_ETHERNET_DRIVER_USE_W5500
	}

	uint8_t mac_address[6] = {0};
	ESP_ERROR_CHECK(esp_read_mac(mac_address, ESP_MAC_ETH));

	for (int i = 0; i < CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM; i++) {
		config->spi_config.eth_config = (esp_eth_config_t)ETH_DEFAULT_CONFIG(
			config->spi_config.eth_mac[i], config->spi_config.eth_phy[i]);

		ESP_ERROR_CHECK(esp_eth_driver_install(
			&config->spi_config.eth_config, &config->spi_config.eth_handle[i]));

		/* The SPI Ethernet module might not have a burned factory MAC address,
	   we cat to set it manually. 02:00:00 is a Locally Administered OUI range
	   so should not be used except when testing on a LAN under your control.
		*/
		mac_address[5] = mac_address[5] + i;

		ESP_ERROR_CHECK(esp_eth_ioctl(config->spi_config.eth_handle[i],
									  ETH_CMD_S_MAC_ADDR, mac_address));

		// attach Ethernet driver to TCP/IP stack
		ESP_ERROR_CHECK(esp_netif_attach(
			config->spi_config.netif[i],
			esp_eth_new_netif_glue(config->spi_config.eth_handle[i])));
	}
#endif // CONFIG_ETH_USE_SPI_ETHERNET

	// Register user defined event handers
	ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
											   &eth_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
											   &got_ip_event_handler, NULL));

	/* start Ethernet driver state machine */
#if CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET
	ESP_ERROR_CHECK(esp_eth_start(config->internal_config.eth_handle));
#endif // CONFIG_ETHERNET_DRIVER_USE_INTERNAL_ETHERNET

#if CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
	for (int i = 0; i < CONFIG_ETHERNET_DRIVER_SPI_ETHERNETS_NUM; i++) {
		ESP_ERROR_CHECK(esp_eth_start(config->spi_config.eth_handle[i]));
	}
#endif // CONFIG_ETHERNET_DRIVER_USE_SPI_ETHERNET
}

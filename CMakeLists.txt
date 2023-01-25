idf_component_register(
    SRCS "ethernet_driver.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES log_utils driver esp_eth esp_netif
)

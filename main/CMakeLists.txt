idf_component_register(SRCS "main.c"
                    REQUIRES chip_info camera
                    PRIV_REQUIRES spi_flash esp_event esp_wifi nvs_flash
                    INCLUDE_DIRS ".")

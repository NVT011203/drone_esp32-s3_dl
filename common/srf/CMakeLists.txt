idf_component_register(SRCS "srf.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_timer
                       PRIV_REQUIRES)

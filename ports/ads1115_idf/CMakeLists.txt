idf_component_register(SRCS "ads_port.c"
                    INCLUDE_DIRS "."
                    REQUIRES ads1115_core esp_driver_i2c)

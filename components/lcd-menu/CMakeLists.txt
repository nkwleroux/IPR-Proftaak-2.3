set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "lcd-menu.c")
set(COMPONENT_PRIV_REQUIRES esp32-i2c-lcd1602 
                            esp32-smbus
                            http-request
                            json
                            esp_peripherals
                            input_key_service
                            touch-buttons
                            clock-sync)
register_component()

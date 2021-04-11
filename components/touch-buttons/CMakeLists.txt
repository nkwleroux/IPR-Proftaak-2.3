set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "touch-buttons.c")
set(COMPONENT_PRIV_REQUIRES esp_peripherals
                            input_key_service
                            esp32-smbus 
                            esp32-i2c-lcd1602 
                            lcd-menu
                            )
register_component()

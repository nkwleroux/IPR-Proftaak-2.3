set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_REQUIRES  esp_peripherals
                        input_key_service
                        esp32-smbus 
                        esp32-i2c-lcd1602 
                        lcd-menu)
register_component()
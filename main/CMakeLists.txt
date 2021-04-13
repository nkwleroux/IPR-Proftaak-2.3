set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_REQUIRES  esp32-smbus 
                        esp32-i2c-lcd1602 
                        lcd-menu
                        wifi-connect
                        clock-sync
                        http-request
                        esp_peripherals
                        input_key_service
                        touch-buttons)
register_component()
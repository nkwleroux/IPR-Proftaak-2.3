set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_REQUIRES  esp_peripherals
                        input_key_service)
register_component()
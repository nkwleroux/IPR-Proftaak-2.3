set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "http-request.c")
set(COMPONENT_PRIV_REQUIRES lwip
                            json
                            wifi-connect)
register_component()

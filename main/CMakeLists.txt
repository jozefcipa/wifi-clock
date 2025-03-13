set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

idf_component_register(SRCS "wifi.c" "main.c" "vfd.c" "datetime.c" "wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos esp_wifi nvs_flash driver
                    )

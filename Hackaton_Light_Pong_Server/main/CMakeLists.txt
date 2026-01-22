idf_component_register(SRCS "blink_example_main.c"
                            "drivers/dmx/dmx.c"
                            "drivers/mh_x25/mh_x25.c"
                       INCLUDE_DIRS "." "drivers" "drivers/dmx" "drivers/mh_x25"
                       REQUIRES driver esp_timer esp_event esp_netif esp_wifi nvs_flash)

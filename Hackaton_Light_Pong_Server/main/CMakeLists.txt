idf_component_register(SRCS "blink_example_main.c"
                            "dmx.c"
                            "mh_x25.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_timer)

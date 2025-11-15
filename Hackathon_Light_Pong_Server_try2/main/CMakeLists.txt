idf_component_register(SRCS "main.c"
    "dmx/dmx_controller.c"
    "dmx/mh_x25_controller.c"
    INCLUDE_DIRS "." "dmx"
    REQUIRES driver esp_timer)
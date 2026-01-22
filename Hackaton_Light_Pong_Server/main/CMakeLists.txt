idf_component_register(SRCS "blink_example_main.c"
                            "communication/espnow_handler.c"
                            "effects/light_effects.c"
                            "game/game_controller.c"
                       INCLUDE_DIRS "." 
                                    "config"
                                    "communication"
                                    "effects"
                                    "game"
                       REQUIRES driver esp_timer esp_event esp_netif esp_wifi nvs_flash 
                                dmx_driver mh_x25_driver)


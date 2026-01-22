idf_component_register(SRCS "light_pong_main.c"
                            "game/game_controller.c"
                       INCLUDE_DIRS "." 
                                    "config"
                                    "game"
                       REQUIRES driver esp_timer esp_event esp_netif esp_wifi nvs_flash 
                                dmx_driver mh_x25_driver light_effects espnow_comm)


idf_component_register(SRCS "src/inbound_message_pool.cpp"
                            "src/companion_inbound_message_consumer.cpp"
                            "src/companion_message_reply_sender.cpp"
                            "src/get_device_info_handler.cpp"
                            "src/set_wifi_credentials_handler.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES platform companion_protocol settings
                       PRIV_REQUIRES log)

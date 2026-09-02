idf_component_register(SRCS "src/mailbox.cpp"
                            "src/runtime.cpp"
                            "src/scheduler.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES log
                       PRIV_REQUIRES esp_system esp_timer freertos)

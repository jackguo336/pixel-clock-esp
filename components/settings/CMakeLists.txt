idf_component_register(SRCS "src/nvs_init.cpp" "src/nvs_wifi_credentials_store.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES nvs_flash log)

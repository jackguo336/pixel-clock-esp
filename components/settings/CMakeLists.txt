idf_component_register(SRCS "src/nvs_init.cpp" "src/wifi_credentials_store.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES nvs_flash log)

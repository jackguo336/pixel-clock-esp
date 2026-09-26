idf_component_register(SRCS "pixel_clock_esp_main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES platform display_runtime file_system littlefs)

littlefs_create_partition_image(assets ../assets FLASH_IN_PROJECT)

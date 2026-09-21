idf_component_register(SRCS "pixel_clock_esp_main.cpp"
                            "demo_ticks.cpp"
                            "assets_fs.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES platform display littlefs)

littlefs_create_partition_image(assets ../assets FLASH_IN_PROJECT)

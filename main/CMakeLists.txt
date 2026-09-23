idf_component_register(SRCS "pixel_clock_esp_main.cpp"
                            "demo_ticks.cpp"
                            "demo_bitmap_load.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES platform display file_system littlefs)

littlefs_create_partition_image(assets ../assets FLASH_IN_PROJECT)

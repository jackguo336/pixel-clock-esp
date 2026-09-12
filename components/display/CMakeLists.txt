idf_component_register(SRCS "src/bitmap.cpp"
                            "src/logical_framebuffer.cpp"
                            "src/bitmap_file_loader.cpp"
                            "src/bitmap_rasterizer.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES vfs)

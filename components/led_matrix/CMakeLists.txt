idf_component_register(SRCS "src/led_matrix_map.cpp"
                            "src/led_matrix_output.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "src"
                       REQUIRES display
                       PRIV_REQUIRES led_strip log)

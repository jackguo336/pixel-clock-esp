# ESP-IDF test-component adapter. Unity cases and the wrapped led_strip
# stand-in live under embedded/; native-host GoogleTest sources are not compiled here.
idf_component_register(SRC_DIRS "embedded"
                       INCLUDE_DIRS "embedded"
                       PRIV_INCLUDE_DIRS "../src"
                       REQUIRES unity
                       PRIV_REQUIRES led_matrix display)

# Linker wraps replace the led_strip driver so matrix tests never touch SPI.
target_link_libraries(${COMPONENT_LIB} INTERFACE
                      "-Wl,--wrap=led_strip_new_spi_device"
                      "-Wl,--wrap=led_strip_set_pixel"
                      "-Wl,--wrap=led_strip_refresh"
                      "-Wl,--wrap=led_strip_del")

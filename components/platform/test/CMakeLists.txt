idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS "."
                       REQUIRES unity
                       PRIV_REQUIRES platform freertos esp_timer)

# Linker wraps replace IDF's esp_timer create/start/stop/delete so scheduler
# tests never arm a real timer. Unwrapped handles still reach the originals.
target_link_libraries(${COMPONENT_LIB} INTERFACE
                      "-Wl,--wrap=esp_timer_create"
                      "-Wl,--wrap=esp_timer_delete"
                      "-Wl,--wrap=esp_timer_start_once"
                      "-Wl,--wrap=esp_timer_start_periodic"
                      "-Wl,--wrap=esp_timer_stop")

# ESP-IDF test-component adapter. IDF discovers this file at
# components/<name>/test/CMakeLists.txt. Unity cases live under embedded/.
# There is no native-host suite: the store calls NVS.
idf_component_register(SRC_DIRS "embedded"
                       INCLUDE_DIRS "embedded"
                       REQUIRES unity
                       PRIV_REQUIRES settings nvs_flash)

# The shared runner has one setUp, and platform's suite already defines it.
# Wrap that call so each case initializes NVS and clears the key, then runs
# platform's setUp.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=setUp")

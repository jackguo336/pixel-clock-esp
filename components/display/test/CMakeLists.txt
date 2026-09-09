# ESP-IDF test-component adapter. IDF discovers this file at
# components/<name>/test/CMakeLists.txt. Unity cases and the in-memory VFS
# driver live under embedded/; native-host GoogleTest sources are not compiled here.
idf_component_register(SRC_DIRS "embedded"
                       INCLUDE_DIRS "embedded"
                       PRIV_INCLUDE_DIRS "../src"
                       REQUIRES unity
                       PRIV_REQUIRES display vfs)

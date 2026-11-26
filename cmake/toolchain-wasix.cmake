# WASIX (wasix-libc) via wasixcc; fork/exec/termios are real here, so this is
# not CMAKE_SYSTEM_NAME=WASI. WASIXCC_* env vars select the sysroot and EH mode.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_C_COMPILER ${CMAKE_CURRENT_LIST_DIR}/wasixcc-nopic)
set(CMAKE_CXX_COMPILER wasix++)
set(CMAKE_AR wasixar)
set(CMAKE_RANLIB wasixranlib)
set(CMAKE_EXECUTABLE_SUFFIX .wasm)
# no shared libraries (zlib would link libz.so with -shared, a PIC link wasixcc rejects)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

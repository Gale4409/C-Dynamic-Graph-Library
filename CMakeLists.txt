cmake_minimum_required(VERSION 3.20)
project(DynaGraph C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- Compiler flags -------------------------------------------------
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wno-unused-parameter
    )
endif()

# ---- Main executable ------------------------------------------------
add_executable(DynaGraph
    main.c
    dynagraph.c
    hdt.c
    et_tree.c
    queue.c
    hash_table.c
    item.c
    UF.c
)

# POSIX clock_gettime requires linking -lrt on some Linux systems
if(UNIX AND NOT APPLE)
    target_link_libraries(DynaGraph PRIVATE rt)
endif()

# ---- Optional: sanitiser build for debugging -----------------------
option(ENABLE_ASAN "Build with AddressSanitiser" OFF)
if(ENABLE_ASAN)
    target_compile_options(DynaGraph PRIVATE -fsanitize=address,undefined -g)
    target_link_options   (DynaGraph PRIVATE -fsanitize=address,undefined)
endif()

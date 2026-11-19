cmake_minimum_required(VERSION 3.20)
project(minifs C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

# Executavel principal do MiniFS
add_executable(minifs
    src/disk.c
    src/bitmap.c
    src/metadata.c
    src/fs_ops.c
    src/cli.c
    src/main.c
)

# Executavel da suite de testes
add_executable(test_minifs
    src/disk.c
    src/bitmap.c
    src/metadata.c
    src/fs_ops.c
    tests/test_minifs.c
)

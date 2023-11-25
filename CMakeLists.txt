cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(elf_test)

add_executable(elf_test src/main.cpp src/elf_view.cpp src/elf_loader.cpp)
target_link_libraries(elf_test PRIVATE librt.so)
target_include_directories(elf_test PUBLIC "include")
target_include_directories(elf_test PUBLIC "/home/sbondi/workspace/plugins-nextgen/shm/src/device")
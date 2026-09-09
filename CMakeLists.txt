cmake_minimum_required(VERSION 3.16)
project(MiniFFT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(minifft cpp/dft.cpp cpp/fft.cpp)
target_include_directories(minifft PUBLIC cpp)
target_compile_options(minifft PRIVATE -Wall -Wextra -Wpedantic)

add_executable(signal_generator cpp/signal_generator.cpp)
target_link_libraries(signal_generator PRIVATE minifft)
target_compile_options(signal_generator PRIVATE -Wall -Wextra -Wpedantic)

add_executable(fsk_demo cpp/fsk_demo.cpp)
target_link_libraries(fsk_demo PRIVATE minifft)
target_compile_options(fsk_demo PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME generate_and_validate COMMAND signal_generator ${CMAKE_SOURCE_DIR})
add_test(NAME fsk_default_channel COMMAND fsk_demo)

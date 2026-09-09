cmake_minimum_required(VERSION 3.16)
project(mdfft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mdfft cpp/dft.cpp cpp/fft.cpp)
target_include_directories(mdfft PUBLIC cpp)
target_compile_options(mdfft PRIVATE -Wall -Wextra -Wpedantic)

add_executable(signal_generator cpp/signal_generator.cpp)
target_link_libraries(signal_generator PRIVATE mdfft)
target_compile_options(signal_generator PRIVATE -Wall -Wextra -Wpedantic)

add_executable(fsk_demo cpp/fsk_demo.cpp)
target_link_libraries(fsk_demo PRIVATE mdfft)
target_compile_options(fsk_demo PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME generate_and_validate COMMAND signal_generator ${CMAKE_SOURCE_DIR})
add_test(NAME fsk_default_channel COMMAND fsk_demo)

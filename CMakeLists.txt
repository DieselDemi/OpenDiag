cmake_minimum_required(VERSION 3.22)

set(CMAKE_SYSTEM_VERSION 10.0.19041.0)

project(ptdiag_reversed C)

set(CMAKE_C_STANDARD 99)

message(STATUS "test: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

add_executable(ptdiag_reversed WIN32 dummy_callbacks.c utils.c configuration.c init_file.c about.c main_dialog.c logfile.c ptdiag.c)
target_sources(ptdiag_reversed PRIVATE ptdiag.rc)

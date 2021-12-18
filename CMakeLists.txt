# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
# cmake_minimum_required(VERSION 3.17)

# list(APPEND DTS_ROOT ${CMAKE_SOURCE_DIR})

list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers
)

include_directories(src)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

# target_sources(app PRIVATE src/main.c)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
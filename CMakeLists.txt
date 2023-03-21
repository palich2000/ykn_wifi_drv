# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

#set(ZEPHYR_EXTRA_MODULES "/home/palich/zephyrproject/modules/spi_gecko;/home/palich/zephyrproject/modules/wfm200")
#for locally delivered modules or edit west.yml

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ykn_wifi)

FILE(GLOB pktqueue_sources src/*.c)

target_sources(app PRIVATE ${pktqueue_sources})

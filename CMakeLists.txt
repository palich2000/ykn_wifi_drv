# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ykn_wifi)


#zephyr_library_import(lib ${CMAKE_CURRENT_SOURCE_DIR}/liblib.a)
#target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_link_libraries(lib INTERFACE kernel)
#target_link_libraries(app PRIVATE lib)
#target_sources(app PRIVATE src/main.c)

FILE(GLOB pktqueue_sources src/*.c)

#set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/firmware)
#set_target_properties(mylib_lib PROPERTIES INTERFAC${CMAKE_CURRENT_SOURCE_DIR}/firmwareE_INCLUDE_DIRECTORIES ${INCLUDE_DIR})

#target_include_directories(${OFFSETS_LIB} PRIVATE

#add_library(LITTLEFS INTERFACE)

target_include_directories(app PRIVATE
     ${CMAKE_CURRENT_SOURCE_DIR}/firmware
     ${CMAKE_CURRENT_SOURCE_DIR}/config
     ${CMAKE_CURRENT_SOURCE_DIR}/bus
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/src
     ${CMAKE_CURRENT_SOURCE_DIR}/wfx_host
     ${CMAKE_CURRENT_SOURCE_DIR}/pds
  )


target_sources(app PRIVATE ${pktqueue_sources})

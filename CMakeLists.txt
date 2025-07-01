cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
 
project(ENIX)

spiffs_create_partition_image(rootfs rootfs FLASH_IN_PROJECT)
cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32s3" CACHE STRING "ESP-IDF target" FORCE)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sunflower-buddy)

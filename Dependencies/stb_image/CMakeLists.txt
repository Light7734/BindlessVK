cmake_minimum_required(VERSION 3.14)
project (stb_image VERSION 69 LANGUAGES C)

file(GLOB STB_IMAGE_FILE stb_image.c stb_image.h)

add_library(stb_image STATIC ${STB_IMAGE_FILE})
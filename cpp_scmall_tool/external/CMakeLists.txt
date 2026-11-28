cmake_minimum_required(VERSION 3.17)
project(external)

add_subdirectory(zlib)
add_subdirectory(tinyxml2)
add_subdirectory(minizip) 

add_library(external INTERFACE)
target_link_libraries(external INTERFACE zlib tinyxml2 minizip)  

set_target_properties(zlib tinyxml2 minizip PROPERTIES FOLDER "External")
cmake_minimum_required(VERSION 3.17)
project(external)

add_subdirectory(zlib)
add_subdirectory(tinyxml2)
add_subdirectory(unzip) 

add_library(external INTERFACE)
target_link_libraries(external INTERFACE zlib tinyxml2 unzip)  

set_target_properties(zlib tinyxml2 unzip PROPERTIES FOLDER "External")
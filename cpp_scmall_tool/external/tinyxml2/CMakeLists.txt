set(TINYXML2_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2.cpp
)

add_library(tinyxml2 STATIC ${TINYXML2_FILES})

target_include_directories(tinyxml2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#set_target_properties(tinyxml2 PROPERTIES FOLDER "External")   # бя

if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"    # бя
                 FILES ${TINYXML2_FILES})
endif()
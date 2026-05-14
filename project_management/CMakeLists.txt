# project_management: CAM project options and project-level data

file(GLOB PM_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB PM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(project_management STATIC
    ${PM_HEADERS}
    ${PM_SOURCES}
)

target_include_directories(project_management PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(project_management PRIVATE /utf-8)
endif()

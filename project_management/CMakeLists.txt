# project_management: CAM project options and project-level data

add_library(project_management STATIC
    cam_options.h
    cam_options.cpp
)

target_include_directories(project_management PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(project_management PRIVATE /utf-8)
endif()

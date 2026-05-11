# user 模块动态库：登录 UI（QCef）、HTTP（libhv）、会话与 token 持久化
# 注意：QCefView 在根 CMakeLists.txt 中定义

file(GLOB USER_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB USER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(user SHARED
    ${USER_HEADERS}
    ${USER_SOURCES}
)

target_include_directories(user PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QCEF_ROOT}/include"
)

target_compile_definitions(user PRIVATE USER_LIBRARY)

target_link_libraries(user PUBLIC
    project_management
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
    QCefView
    libhv
)

if(MSVC)
    target_compile_options(user PRIVATE /utf-8)
endif()

set_target_properties(user PROPERTIES
    DEBUG_POSTFIX "_d"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

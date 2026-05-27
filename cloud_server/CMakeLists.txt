# cloud_server 模块动态库：登录 UI（QCef）、HTTP（libhv）、会话与 token 持久化
# 注意：QCefView 在根 CMakeLists.txt 中定义

file(GLOB CLOUD_SERVER_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB CLOUD_SERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(cloud_server SHARED
    ${CLOUD_SERVER_HEADERS}
    ${CLOUD_SERVER_SOURCES}
)

target_include_directories(cloud_server PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QCEF_ROOT}/include"
)

target_compile_definitions(cloud_server PRIVATE CLOUD_SERVER_LIBRARY)

target_link_libraries(cloud_server PUBLIC
    project_management
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
    QCefView
    SARibbonBar::SARibbonBar
    libhv
)

if(MSVC)
    target_compile_options(cloud_server PRIVATE /utf-8)
endif()

add_custom_command(TARGET cloud_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${VCPKG_OPENSSL_DLL_DEBUG_DIR}/libcrypto-3-x64.dll>$<$<NOT:$<CONFIG:Debug>>:${VCPKG_OPENSSL_DLL_RELEASE_DIR}/libcrypto-3-x64.dll>" "$<TARGET_FILE_DIR:cloud_server>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${VCPKG_OPENSSL_DLL_DEBUG_DIR}/libssl-3-x64.dll>$<$<NOT:$<CONFIG:Debug>>:${VCPKG_OPENSSL_DLL_RELEASE_DIR}/libssl-3-x64.dll>" "$<TARGET_FILE_DIR:cloud_server>"
)

set_target_properties(cloud_server PROPERTIES
    DEBUG_POSTFIX "_d"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CAMDEMO_RUNTIME_OUTPUT_DIRECTORY}"
    VS_GLOBAL_VcpkgApplocalDeps "false"
)

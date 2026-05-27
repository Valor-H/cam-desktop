# qjcam: main application executable

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(qjcam WIN32
    main.cpp
    qjcam.qrc
)

target_include_directories(qjcam PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(qjcam PRIVATE
    ultramill
)

# Copy runtime files
add_custom_command(TARGET qjcam POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${QCEF_ROOT}/$<CONFIG>/bin" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${LIBHV_DEBUG_ROOT}/bin/hv.dll>$<$<NOT:$<CONFIG:Debug>>:${LIBHV_RELEASE_ROOT}/bin/hv.dll>" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${SARIBBON_DLL_DEBUG}>$<$<NOT:$<CONFIG:Debug>>:${SARIBBON_DLL_RELEASE}>" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${VCPKG_OPENSSL_DLL_DEBUG_DIR}/libcrypto-3-x64.dll>$<$<NOT:$<CONFIG:Debug>>:${VCPKG_OPENSSL_DLL_RELEASE_DIR}/libcrypto-3-x64.dll>" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${VCPKG_OPENSSL_DLL_DEBUG_DIR}/libssl-3-x64.dll>$<$<NOT:$<CONFIG:Debug>>:${VCPKG_OPENSSL_DLL_RELEASE_DIR}/libssl-3-x64.dll>" "$<TARGET_FILE_DIR:qjcam>"
)

set_target_properties(qjcam PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:qjcam>"
    RUNTIME_OUTPUT_DIRECTORY "${CAMDEMO_RUNTIME_OUTPUT_DIRECTORY}"
    VS_GLOBAL_VcpkgApplocalDeps "false"
)

if(MSVC)
    target_compile_options(qjcam PRIVATE /utf-8)
endif()

# qjcam: main application executable

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(qjcam WIN32
    main.cpp
    qjcam.qrc
)

target_link_libraries(qjcam PRIVATE
    ultramill
)

# Copy runtime files
add_custom_command(TARGET qjcam POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${QCEF_ROOT}/$<CONFIG>/bin" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBHV_ROOT}/bin/hv.dll" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SARIBBON_DLL}" "$<TARGET_FILE_DIR:qjcam>"
)

set_target_properties(qjcam PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:qjcam>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(qjcam PRIVATE /utf-8)
endif()

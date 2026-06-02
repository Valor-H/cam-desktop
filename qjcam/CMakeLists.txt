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

set(QCEF_BIN_DIR "$<$<CONFIG:Debug>:${QCEF_ROOT}/Debug/bin>$<$<NOT:$<CONFIG:Debug>>:${QCEF_ROOT}/Release/bin>")

# Copy runtime files
add_custom_command(TARGET qjcam POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QCEF_BIN_DIR}/QCefView.dll" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${QCEF_BIN_DIR}/CefView" "$<TARGET_FILE_DIR:qjcam>/CefView"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${LIBHV_DEBUG_ROOT}/bin/hv.dll>$<$<NOT:$<CONFIG:Debug>>:${LIBHV_RELEASE_ROOT}/bin/hv.dll>" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${SARIBBON_DLL_DEBUG}>$<$<NOT:$<CONFIG:Debug>>:${SARIBBON_DLL_RELEASE}>" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CLOUD_SERVER_QM_FILES} "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:qjcam>/plugins"
    COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:qjcam>/qt.conf" "$<TARGET_FILE_DIR:qjcam>/Qt5Core.dll" "$<TARGET_FILE_DIR:qjcam>/Qt5Gui.dll" "$<TARGET_FILE_DIR:qjcam>/Qt5Widgets.dll" "$<TARGET_FILE_DIR:qjcam>/Qt5Network.dll" "$<TARGET_FILE_DIR:qjcam>/Qt5Concurrent.dll" "$<TARGET_FILE_DIR:qjcam>/qgif.dll" "$<TARGET_FILE_DIR:qjcam>/qico.dll" "$<TARGET_FILE_DIR:qjcam>/qjpeg.dll" "$<TARGET_FILE_DIR:qjcam>/qwindows.dll" "$<TARGET_FILE_DIR:qjcam>/qwindowsvistastyle.dll" "$<TARGET_FILE_DIR:qjcam>/zlib1.dll"
    COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}" "$<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>" --force --no-translations --compiler-runtime --dir "$<TARGET_FILE_DIR:qjcam>" "$<TARGET_FILE:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${QT_BIN_DIR}/Qt5Concurrentd.dll>$<$<NOT:$<CONFIG:Debug>>:${QT_BIN_DIR}/Qt5Concurrent.dll>" "$<TARGET_FILE_DIR:qjcam>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<$<CONFIG:Debug>:${QT_BIN_DIR}/Qt5Networkd.dll>$<$<NOT:$<CONFIG:Debug>>:${QT_BIN_DIR}/Qt5Network.dll>" "$<TARGET_FILE_DIR:qjcam>"
)

set_target_properties(qjcam PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:qjcam>"
    RUNTIME_OUTPUT_DIRECTORY "${CAMDEMO_RUNTIME_OUTPUT_DIRECTORY}"
    VS_GLOBAL_VcpkgApplocalDeps "false"
)

if(MSVC)
    target_compile_options(qjcam PRIVATE /utf-8)
endif()

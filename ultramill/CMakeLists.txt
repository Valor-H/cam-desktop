# ultramill: workstation shell library
# Contains NApplication, NMainWindow, TitleBarUserChip, DesktopWebServer, QCefRuntime

file(GLOB ULTRAMILL_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB ULTRAMILL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(ultramill STATIC
    ${ULTRAMILL_HEADERS}
    ${ULTRAMILL_SOURCES}
    ultramill.qrc
)

set(ULTRAMILL_TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ultramill_zh_CN.ts"
)

qt5_add_translation(ULTRAMILL_QM_FILES ${ULTRAMILL_TS_FILES})

add_custom_target(ultramill_translations DEPENDS ${ULTRAMILL_QM_FILES})
add_dependencies(ultramill ultramill_translations)

set(ULTRAMILL_QM_FILES ${ULTRAMILL_QM_FILES} PARENT_SCOPE)

target_include_directories(ultramill PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(ultramill PUBLIC
    project_management
    SARibbonBar::SARibbonBar
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

if(ENABLE_CLOUD_SERVER_MODULE)
    target_link_libraries(ultramill PUBLIC
        cloud_server
        QCefView
        Qt5::Network
        libhv
    )
endif()

if(MSVC)
    target_compile_options(ultramill PRIVATE /utf-8)
endif()

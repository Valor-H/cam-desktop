# ultramill: workstation shell library
# Contains NApplication, NMainWindow, TitleBarUserChip, DesktopFrontendServer, QCefRuntime, MockMainWorkspace

add_library(ultramill STATIC
    napplication.h
    napplication.cpp
    nmainwindow.h
    nmainwindow.cpp
    title_bar_user_chip.h
    title_bar_user_chip.cpp
    desktop_frontend_server.h
    desktop_frontend_server.cpp
    q_cef_runtime.h
    q_cef_runtime.cpp
    mock_main_workspace.h
    ultramill.qrc
)

target_include_directories(ultramill PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(ultramill PUBLIC
    user
    QCefView
    SARibbonBar::SARibbonBar
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    libhv
)

if(MSVC)
    target_compile_options(ultramill PRIVATE /utf-8)
endif()

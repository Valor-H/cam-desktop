cmake_minimum_required(VERSION 3.19)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_TOOLSET "v142" CACHE STRING "Platform Toolset" FORCE)
endif()

project(CamDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

# ========== 1. 鏌ユ壘 Qt ==========
set(CMAKE_PREFIX_PATH "D:/Tools/QT5/5.15.2/msvc2019_64")
find_package(Qt5 COMPONENTS Core Gui Widgets Network Concurrent LinguistTools REQUIRED)

# ========== 2. QCefView锛堢敱 user 妯″潡鍐呴儴浣跨敤锛?=========
# 甯冨眬锛?./3rdparty/QCefView/Debug/bin/QCefView.dll銆丏ebug/lib/QCefView.lib锛堜笌 Release 瀵圭О锛?
set(QCEF_ROOT "${CMAKE_SOURCE_DIR}/../3rdparty/QCefView")

# QCefView 瀵煎叆搴擄紙user 妯″潡鍜屼富绋嬪簭閮介渶瑕侀摼鎺ワ級
add_library(QCefView SHARED IMPORTED GLOBAL)
set_target_properties(QCefView PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release"
    IMPORTED_LOCATION_DEBUG "${QCEF_ROOT}/Debug/bin/QCefView.dll"
    IMPORTED_IMPLIB_DEBUG "${QCEF_ROOT}/Debug/lib/QCefView.lib"
    IMPORTED_LOCATION_RELEASE "${QCEF_ROOT}/Release/bin/QCefView.dll"
    IMPORTED_IMPLIB_RELEASE "${QCEF_ROOT}/Release/lib/QCefView.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${QCEF_ROOT}/include"
)

# ========== 3. SARibbonBar ==========
set(SARibbonBar_DIR "D:/Codes/SARibbon/bin_qt5.15.2_MSVC_x64/lib/cmake/SARibbonBar")
find_package(SARibbonBar REQUIRED)
set(SARIBBON_DLL "D:/Codes/SARibbon/bin_qt5.15.2_MSVC_x64/bin/SARibbonBard.dll")

# ========== 4. libhv ==========
set(LIBHV_ROOT "D:/Codes/libhv/installed_debug")
add_library(libhv SHARED IMPORTED GLOBAL)
set_target_properties(libhv PROPERTIES
    IMPORTED_LOCATION "${LIBHV_ROOT}/bin/hv.dll"
    IMPORTED_IMPLIB "${LIBHV_ROOT}/lib/hv.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBHV_ROOT}/include"
    INTERFACE_LINK_LIBRARIES "secur32;crypt32;winmm;iphlpapi;ws2_32"
)

# ========== 5. user 妯″潡 ==========
add_subdirectory(user)

# ========== 6. 涓荤▼搴?==========
add_executable(CamDemo WIN32
    main.cpp
    NApplication.cpp
    NApplication.h
    NMainWindow.cpp
    NMainWindow.h
    MockMainWorkspace.h
    DesktopFrontendServer.cpp
    DesktopFrontendServer.h
    QCefRuntime.cpp
    QCefRuntime.h
    CamDemo.cpp
    CamDemo.h
    TitleBarUserChip.cpp
    TitleBarUserChip.h
    CamDemo.ui
    CamDemo.qrc
)

# ========== 7. 缈昏瘧 ==========
set(CAMDEMO_TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/CamDemo_zh_CN.ts)
qt5_add_translation(CAMDemo_QM_FILES ${CAMDEMO_TS_FILES})
add_custom_target(CamDemoTranslations ALL DEPENDS ${CAMDemo_QM_FILES})
add_dependencies(CamDemo CamDemoTranslations)

# ========== 8. 閾炬帴 ==========
# 娉ㄦ剰锛歈CefView 蹇呴』鍚屾椂閾炬帴锛岄潤鎬佸簱渚濊禆涓嶄細鑷姩浼犻€?
target_link_libraries(CamDemo PRIVATE
    user
    QCefView
    SARibbonBar::SARibbonBar
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)

# ========== 9. 澶嶅埗杩愯鏃?==========
add_custom_command(TARGET CamDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${QCEF_ROOT}/$<CONFIG>/bin" "$<TARGET_FILE_DIR:CamDemo>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBHV_ROOT}/bin/hv.dll" "$<TARGET_FILE_DIR:CamDemo>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SARIBBON_DLL}" "$<TARGET_FILE_DIR:CamDemo>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:CamDemo>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CAMDemo_QM_FILES} "$<TARGET_FILE_DIR:CamDemo>/translations"
)

# ========== 10. 灞炴€?==========
set_target_properties(CamDemo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CamDemo>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(CamDemo PRIVATE /utf-8)
endif()

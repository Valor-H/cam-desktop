cmake_minimum_required(VERSION 3.19)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_TOOLSET "v142" CACHE STRING "Platform Toolset" FORCE)
    set(CMAKE_VS_GLOBALS "VcpkgEnabled=false;VcpkgApplocalDeps=false" CACHE STRING "Visual Studio globals" FORCE)
endif()

project(CamDemo VERSION 0.1 LANGUAGES CXX)
option(ENABLE_CLOUD_SERVER_MODULE "Enable cloud features for cam-desktop" ON)
if(ENABLE_CLOUD_SERVER_MODULE)
    add_compile_definitions(ENABLE_CLOUD_SERVER_MODULE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CAMDEMO_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/x64")

# 1. Qt
set(QT_ROOT "D:/Tools/QT5/5.15.2/msvc2019_64")
set(QT_BIN_DIR "${QT_ROOT}/bin")
set(QT_WINDEPLOYQT_EXECUTABLE "${QT_BIN_DIR}/windeployqt.exe")
set(CMAKE_PREFIX_PATH "${QT_ROOT}")
if(NOT EXISTS "${QT_WINDEPLOYQT_EXECUTABLE}")
    message(FATAL_ERROR "windeployqt.exe not found under ${QT_BIN_DIR}")
endif()
find_package(Qt5 COMPONENTS Core Gui Widgets Network Concurrent LinguistTools REQUIRED)

# 2. QCefView
set(THIRD_PARTY_ROOT "${CMAKE_SOURCE_DIR}/3rdparty")
if(ENABLE_CLOUD_SERVER_MODULE)
    set(QCEF_ROOT "${THIRD_PARTY_ROOT}/QCefView")

    add_library(QCefView SHARED IMPORTED GLOBAL)
    set_target_properties(QCefView PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${QCEF_ROOT}/Debug/bin/QCefView.dll"
        IMPORTED_IMPLIB_DEBUG "${QCEF_ROOT}/Debug/lib/QCefView.lib"
        IMPORTED_LOCATION_RELEASE "${QCEF_ROOT}/Release/bin/QCefView.dll"
        IMPORTED_IMPLIB_RELEASE "${QCEF_ROOT}/Release/lib/QCefView.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${QCEF_ROOT}/include"
    )
endif()

# 3. SARibbonBar
set(SARibbonBar_DIR "${THIRD_PARTY_ROOT}/SARibbon/lib/cmake/SARibbonBar")
find_package(SARibbonBar REQUIRED)
set_property(TARGET SARibbonBar::SARibbonBar APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/SARibbon/include"
)
set(SARIBBON_RUNTIME_DIR "${THIRD_PARTY_ROOT}/SARibbon/bin")
set(SARIBBON_DLL_DEBUG "${SARIBBON_RUNTIME_DIR}/SARibbonBard.dll")
set(SARIBBON_DLL_RELEASE "${SARIBBON_RUNTIME_DIR}/SARibbonBar.dll")

# 4. libhv
if(ENABLE_CLOUD_SERVER_MODULE)
    set(LIBHV_DEBUG_ROOT "${THIRD_PARTY_ROOT}/libhv/Debug" CACHE PATH "libhv debug build root" FORCE)
    set(LIBHV_RELEASE_ROOT "${THIRD_PARTY_ROOT}/libhv/Release" CACHE PATH "libhv release build root" FORCE)
    add_library(libhv SHARED IMPORTED GLOBAL)
    set_target_properties(libhv PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${LIBHV_DEBUG_ROOT}/bin/hv.dll"
        IMPORTED_IMPLIB_DEBUG "${LIBHV_DEBUG_ROOT}/lib/hv.lib"
        IMPORTED_LOCATION_RELEASE "${LIBHV_RELEASE_ROOT}/bin/hv.dll"
        IMPORTED_IMPLIB_RELEASE "${LIBHV_RELEASE_ROOT}/lib/hv.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBHV_DEBUG_ROOT}/include"
        INTERFACE_LINK_LIBRARIES "secur32;crypt32;winmm;iphlpapi;ws2_32"
    )
endif()

# 5. modules
add_subdirectory(project_management)
if(ENABLE_CLOUD_SERVER_MODULE)
    add_subdirectory(cloud_server)
endif()
add_subdirectory(ultramill)
add_subdirectory(qjcam)

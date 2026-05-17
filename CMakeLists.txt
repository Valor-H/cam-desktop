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

# 1. Qt
set(CMAKE_PREFIX_PATH "D:/Tools/Qt/5.15.2/msvc2019_64")
find_package(Qt5 COMPONENTS Core Gui Widgets Network Concurrent REQUIRED)

# 2. QCefView
set(QCEF_ROOT "D:/Codes/3rdparty/QCefView")

add_library(QCefView SHARED IMPORTED GLOBAL)
set_target_properties(QCefView PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release"
    IMPORTED_LOCATION_DEBUG "${QCEF_ROOT}/Debug/bin/QCefView.dll"
    IMPORTED_IMPLIB_DEBUG "${QCEF_ROOT}/Debug/lib/QCefView.lib"
    IMPORTED_LOCATION_RELEASE "${QCEF_ROOT}/Release/bin/QCefView.dll"
    IMPORTED_IMPLIB_RELEASE "${QCEF_ROOT}/Release/lib/QCefView.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${QCEF_ROOT}/include"
)

# 3. SARibbonBar
set(SARibbonBar_DIR "D:/Codes/3rdparty/SARibbon/bin_qt5.15.2_MSVC_x64/lib/cmake/SARibbonBar")
find_package(SARibbonBar REQUIRED)
set_property(TARGET SARibbonBar::SARibbonBar APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES "D:/Codes/3rdparty/SARibbon/bin_qt5.15.2_MSVC_x64/include"
)
set(SARIBBON_RUNTIME_DIR "D:/Codes/3rdparty/SARibbon/bin_qt5.15.2_MSVC_x64/bin")
set(SARIBBON_DLL_DEBUG "${SARIBBON_RUNTIME_DIR}/SARibbonBard.dll")
set(SARIBBON_DLL_RELEASE "${SARIBBON_RUNTIME_DIR}/SARibbonBar.dll")

# 4. libhv
set(LIBHV_DEBUG_ROOT "D:/Codes/3rdparty/libhv_ssl_debug" CACHE PATH "libhv debug build root" FORCE)
set(LIBHV_RELEASE_ROOT "D:/Codes/3rdparty/libhv_ssl_release" CACHE PATH "libhv release build root" FORCE)
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

# 5. modules
add_subdirectory(project_management)
add_subdirectory(cloud-server)
add_subdirectory(ultramill)
add_subdirectory(qjcam)

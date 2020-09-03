cmake_minimum_required(VERSION 3.5)

project(S25Viewer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

if(ANDROID)
  add_library(S25Viewer SHARED
    main.cpp
    s25imageview.cpp
    s25imageview.h
    widget.cpp
    widget.h
    widget.ui
    S25LayerModel.cpp
    S25LayerModel.h
    s25decoder/S25Decoder.h
    S25DecoderWrapper.h
  )
else()
  add_executable(S25Viewer MACOSX_BUNDLE WIN32
    main.cpp
    s25imageview.cpp
    s25imageview.h
    widget.cpp
    widget.h
    widget.ui
    S25LayerModel.cpp
    S25LayerModel.h
    s25decoder/S25Decoder.h
    S25DecoderWrapper.h
  )
endif()

if(APPLE)
  set_target_properties(S25Viewer PROPERTIES
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2020 Hikaru Terazono (3c1u). All rights reserved."
    MACOSX_BUNDLE_LONG_VERSION_STRING "1.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.1.0"
    MACOSX_BUNDLE_BUNDLE_NAME "S25 Viewer"
    MACOSX_BUNDLE_INFO_STRING "S25 Viewer"
    MACOSX_BUNDLE_GUI_IDENTIFIER "tokyo.vulpesgames.3c1u.s25viewer"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info-CMake.plist
  )
endif()

add_custom_target(
    s25decoder-build
    COMMAND cargo build --release
)

target_link_directories(S25Viewer PRIVATE ${CMAKE_SOURCE_DIR}/target/release)

add_dependencies(S25Viewer s25decoder-build)

target_link_libraries(S25Viewer PRIVATE Qt5::Widgets)
target_link_libraries(S25Viewer PRIVATE s25decoder)

if (UNIX AND NOT APPLE)
  target_link_libraries(S25Viewer PRIVATE dl pthread)
endif()

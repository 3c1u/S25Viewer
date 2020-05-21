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
    s25decoder/S25Decoder.h
    S25DecoderWrapper.h
  )
else()
  add_executable(S25Viewer
    main.cpp
    s25imageview.cpp
    s25imageview.h
    widget.cpp
    widget.h
    widget.ui
    s25decoder/S25Decoder.h
    S25DecoderWrapper.h
  )
endif()

execute_process(COMMAND cd ${CMAKE_SOURCE_DIR}/s25decoder)
execute_process(COMMAND cargo build --release)
execute_process(COMMAND cd ${CMAKE_BINARY_DIR})

target_link_directories(S25Viewer PRIVATE ${CMAKE_SOURCE_DIR}/s25decoder/target/release)

target_link_libraries(S25Viewer PRIVATE Qt5::Widgets)
target_link_libraries(S25Viewer PRIVATE s25decoder)

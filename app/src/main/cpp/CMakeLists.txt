# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# 添加头文件地址 .h
include_directories(${CMAKE_SOURCE_DIR}/include)

set(FF ${CMAKE_SOURCE_DIR}/../../../libs/${ANDROID_ABI})
#add_library(avcodec SHARED IMPORTED)
#set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FF}/libavcodec.so)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${FF}")

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        native-lib.cpp
        )


#find_library( # Sets the name of the path variable.
#        log-lib
#        log)

target_link_libraries( # Specifies the target library.
        native-lib
        android
        avcodec avformat avutil swscale swresample avfilter
        #        ${log-lib}
        log
        )
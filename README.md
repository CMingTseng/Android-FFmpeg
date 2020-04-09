# Android-FFmpeg
FFmpeg 使用指南

## 1.配置cmake文件
```c++
# 添加头文件地址 .h
include_directories(${CMAKE_SOURCE_DIR}/include)
#　设置so文件地址
set(FF ${CMAKE_SOURCE_DIR}/../../../libs/${ANDROID_ABI})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${FF}")

target_link_libraries( # Specifies the target library.
        native-lib
        android
        avcodec avformat avutil swscale swresample avfilter
        log
        )
```
## 2.流程

结合代码参考下面的图片，食用更佳

![ffmpeg 流程](https://github.com/bugyun/Android-FFmpeg/blob/master/file/use-ffmpeg.png)

# 获取当前目录名作为库名
get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# 自动收集所有 .c 文件
file(GLOB LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
# 定义一个静态库
add_library(${LIB_NAME} STATIC ${LIB_SOURCES})

# 如果你有头文件需要导出，可以加上：
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})



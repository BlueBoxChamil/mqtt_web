cmake_minimum_required(VERSION 3.10.0)
project(myPro VERSION 0.1.0 LANGUAGES C CXX)

# 设置项目名字
set(TARGET_NAME ${PROJECT_NAME})

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# 自动添加 components 目录下的所有子目录
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/components ${CMAKE_CURRENT_SOURCE_DIR}/components/*)
foreach(subdir ${SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/components/${subdir}")
        add_subdirectory(components/${subdir})
    endif()
endforeach()

add_executable(${TARGET_NAME} main.c)

# 自动链接所有库（假设每个子目录的库名都等于目录名）
target_link_libraries(${TARGET_NAME} PUBLIC ${SUBDIRS})



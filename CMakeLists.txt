cmake_minimum_required(VERSION 3.16)
project(ImageServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 查找nlohmann/json包
find_package(nlohmann_json QUIET)

# 如果找不到CMake配置文件，手动设置
if(NOT nlohmann_json_FOUND)
    message(STATUS "未找到nlohmann_json CMake配置文件，尝试手动设置...")
    set(nlohmann_json_INCLUDE_DIRS "/usr/local/include")
    set(nlohmann_json_FOUND TRUE)
    message(STATUS "手动设置nlohmann_json包含目录: ${nlohmann_json_INCLUDE_DIRS}")
endif()


# 包含目录
include_directories(include)

# 源文件
set(SOURCES
    src/main.cpp
    src/Server.cpp
    src/HttpParser.cpp
    src/ImageProcessor.cpp
    src/YOLOv8Detector.cpp
    src/ThreadPool.cpp
    src/ConfigManager.cpp
    src/Logger.cpp
)

# 创建可执行文件
add_executable(image_server ${SOURCES})

# 链接OpenCV库和nlohmann/json库
target_link_libraries(image_server ${OpenCV_LIBS})

# 根据nlohmann/json的查找结果进行链接
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(image_server nlohmann_json::nlohmann_json)
else()
    # 手动设置包含目录
    target_include_directories(image_server PRIVATE ${nlohmann_json_INCLUDE_DIRS})
endif()

# 设置编译选项
if(WIN32)
    target_compile_definitions(image_server PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(image_server ws2_32)
else()
    target_link_libraries(image_server pthread)
endif()

# 复制web文件和配置文件到构建目录
file(COPY web DESTINATION ${CMAKE_BINARY_DIR})
file(COPY config.json DESTINATION ${CMAKE_BINARY_DIR})

# 创建models目录
# file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/models)


# 安装规则
install(TARGETS image_server DESTINATION bin)
install(DIRECTORY web DESTINATION share/image_server)

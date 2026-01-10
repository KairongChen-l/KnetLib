cmake_minimum_required(VERSION 3.10)
project(HTTPServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)

# 收集所有源文件
set(SRC_FILES
    src/utils.cpp
    src/InetAddress.cpp
    src/Socket.cpp
    src/Epoll.cpp
    src/Channel.cpp
    src/EventLoop.cpp
    src/Acceptor.cpp
    src/Connection.cpp
    src/Server.cpp
    src/Buffer.cpp
    src/ThreadPool.cpp
)

# 创建服务器可执行文件
add_executable(server
    server.cpp
    ${SRC_FILES}
)

# 创建客户端可执行文件
add_executable(client
    client.cpp
    src/utils.cpp
    src/Buffer.cpp
    src/Socket.cpp
    src/InetAddress.cpp
)

# 创建测试可执行文件
add_executable(ThreadPoolTest
    test/ThreadPoolTest.cpp
    src/ThreadPool.cpp
)

# 创建压力测试可执行文件
add_executable(test
    test/test.cpp
    src/utils.cpp
    src/Buffer.cpp
    src/InetAddress.cpp
    src/Socket.cpp
    src/ThreadPool.cpp
)

# 设置输出目录
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(ThreadPoolTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 添加测试运行目标
add_custom_target(run-test
    COMMAND ${CMAKE_BINARY_DIR}/bin/ThreadPoolTest
    DEPENDS ThreadPoolTest
    COMMENT "Running ThreadPoolTest"
)

# 添加清理目标
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMENT "Cleaning build directory"
)


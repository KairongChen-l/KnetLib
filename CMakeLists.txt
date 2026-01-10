cmake_minimum_required(VERSION 3.14)
project(HTTPServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 启用测试
enable_testing()

# 添加 GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9fd7189c2c0cc2a8c8e3c90.zip
)
# 对于 Windows: 防止覆盖父项目的编译器/链接器设置
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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
    src/TcpConnection.cpp
    src/TcpServerSingle.cpp
    src/TcpServer.cpp
)

# 创建服务器可执行文件
add_executable(server
    server.cpp
    ${SRC_FILES}
)

# 创建客户端可执行文件
# add_executable(client
#     client.cpp
#     src/utils.cpp
#     src/Buffer.cpp
#     src/Socket.cpp
#     src/InetAddress.cpp
# )

# 创建测试可执行文件
add_executable(ThreadPoolTest
    test/ThreadPoolTest.cpp
    src/ThreadPool.cpp
)

# 创建压力测试可执行文件
# add_executable(test
#     test/test.cpp
#     src/utils.cpp
#     src/Buffer.cpp
#     src/InetAddress.cpp
#     src/Socket.cpp
#     src/ThreadPool.cpp
# )

# 设置输出目录
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# set_target_properties(client PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
# )

set_target_properties(ThreadPoolTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# set_target_properties(test PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
# )

# 创建测试库（包含所有源文件）
add_library(knetlib_lib STATIC ${SRC_FILES})

# GoogleTest 测试可执行文件
add_executable(BufferTest
    test/BufferTest.cpp
)
target_link_libraries(BufferTest 
    PRIVATE 
    knetlib_lib
    GTest::gtest 
    GTest::gtest_main
)

add_executable(ChannelTest
    test/ChannelTest.cpp
)
target_link_libraries(ChannelTest 
    PRIVATE 
    knetlib_lib
    GTest::gtest 
    GTest::gtest_main
)

add_executable(EventLoopTest
    test/EventLoopTest.cpp
)
target_link_libraries(EventLoopTest 
    PRIVATE 
    knetlib_lib
    GTest::gtest 
    GTest::gtest_main
)

# 设置测试输出目录
set_target_properties(BufferTest ChannelTest EventLoopTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 使用 CTest 运行测试
include(GoogleTest)
add_test(NAME BufferTest COMMAND BufferTest)
add_test(NAME ChannelTest COMMAND ChannelTest)
add_test(NAME EventLoopTest COMMAND EventLoopTest)

# 添加测试运行目标
add_custom_target(run-test
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS BufferTest ChannelTest EventLoopTest
    COMMENT "Running all tests"
)

# 保留旧的 ThreadPoolTest 目标
add_custom_target(run-threadpool-test
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


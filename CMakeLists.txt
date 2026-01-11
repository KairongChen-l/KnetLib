cmake_minimum_required(VERSION 3.14)

# 优先使用 GCC 13（如果已安装）
# 在 project() 之前设置编译器，这样 CMake 会使用指定的编译器
find_program(GCC_13_FOUND gcc-13)
find_program(GXX_13_FOUND g++-13)
if(GCC_13_FOUND AND GXX_13_FOUND)
    set(CMAKE_C_COMPILER gcc-13)
    set(CMAKE_CXX_COMPILER g++-13)
    message(STATUS "检测到 GCC 13，将使用: ${CMAKE_CXX_COMPILER}")
else()
    message(STATUS "未找到 GCC 13，将使用系统默认编译器")
endif()

project(KnetLib VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准（GCC 13 完全支持 C++20）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

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
    src/TimerQueue.cpp
    src/Connector.cpp
    src/TcpClient.cpp
)

# 创建静态库
add_library(knetlib_lib STATIC ${SRC_FILES})
target_include_directories(knetlib_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

# 创建服务器可执行文件
add_executable(server
    server.cpp
)
target_link_libraries(server PRIVATE knetlib_lib)
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 创建客户端可执行文件
add_executable(client
    client.cpp
)
target_link_libraries(client PRIVATE knetlib_lib)
set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# 测试配置
# ============================================================================

# 启用测试
enable_testing()

# 添加 GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 测试辅助函数
function(add_knetlib_test TEST_NAME)
    add_executable(${TEST_NAME} test/${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} 
        PRIVATE 
        knetlib_lib
        GTest::gtest 
        GTest::gtest_main
    )
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    include(GoogleTest)
    gtest_discover_tests(${TEST_NAME})
endfunction()

# 添加所有测试
add_knetlib_test(BufferTest)
add_knetlib_test(ChannelTest)
add_knetlib_test(EventLoopTest)
add_knetlib_test(LoggerTest)
add_knetlib_test(TimerTest)
add_knetlib_test(TimerQueueTest)
add_knetlib_test(InetAddressTest)
add_knetlib_test(TcpConnectionTest)
add_knetlib_test(EpollTest)
add_knetlib_test(TcpServerTest)
add_knetlib_test(TcpServerSingleTest)
add_knetlib_test(TcpClientTest)
add_knetlib_test(AcceptorTest)
add_knetlib_test(ConnectorTest)
add_knetlib_test(SocketTest)

# 创建测试组
set(TEST_TARGETS
    BufferTest
    ChannelTest
    EventLoopTest
    LoggerTest
    TimerTest
    TimerQueueTest
    InetAddressTest
    TcpConnectionTest
    EpollTest
    TcpServerTest
    TcpServerSingleTest
    TcpClientTest
    AcceptorTest
    ConnectorTest
    SocketTest
)

# 添加测试运行目标
add_custom_target(run-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS ${TEST_TARGETS}
    COMMENT "Running all tests"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 添加测试覆盖率目标（如果安装了 gcov/lcov）
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
    add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${LCOV_PATH} --capture --directory . --output-file coverage.info
        COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
        COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage
        COMMENT "Generating code coverage report"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# ============================================================================
# 其他可执行文件
# ============================================================================

# ThreadPool 测试（旧测试，保留兼容性）
add_executable(ThreadPoolTest
    test/ThreadPoolTest.cpp
    src/ThreadPool.cpp
)
target_link_libraries(ThreadPoolTest PRIVATE knetlib_lib)
set_target_properties(ThreadPoolTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_target(run-threadpool-test
    COMMAND ${CMAKE_BINARY_DIR}/bin/ThreadPoolTest
    DEPENDS ThreadPoolTest
    COMMENT "Running ThreadPoolTest"
)

# 网络库完整集成测试
add_executable(test_network
    test_network.cpp
)
target_link_libraries(test_network PRIVATE knetlib_lib)
set_target_properties(test_network PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_target(run-network-test
    COMMAND ${CMAKE_BINARY_DIR}/bin/test_network
    DEPENDS test_network
    COMMENT "Running network integration test"
)

# ============================================================================
# 安装配置（可选）
# ============================================================================

# 安装库和头文件
install(TARGETS knetlib_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY src/
    DESTINATION include/knetlib
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# 清理目标
# ============================================================================

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/_deps
    COMMENT "Cleaning build directory"
)

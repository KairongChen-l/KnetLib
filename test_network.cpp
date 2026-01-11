#include "src/TcpServer.h"
#include "src/TcpClient.h"
#include "src/TcpConnection.h"
#include "src/Buffer.h"
#include "src/EventLoop.h"
#include "src/InetAddress.h"
#include "src/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <cassert>
#include <mutex>

// 测试统计
struct TestStats {
    std::atomic<int> connections{0};
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};
    std::atomic<int> bytes_sent{0};
    std::atomic<int> bytes_received{0};
    std::atomic<int> connections_closed{0};
    std::mutex mutex;
    std::vector<std::string> received_messages;
    
    void addReceivedMessage(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        received_messages.push_back(msg);
    }
    
    void print() {
        std::cout << "\n========== 测试统计 ==========\n";
        std::cout << "连接数: " << connections.load() << "\n";
        std::cout << "发送消息数: " << messages_sent.load() << "\n";
        std::cout << "接收消息数: " << messages_received.load() << "\n";
        std::cout << "发送字节数: " << bytes_sent.load() << "\n";
        std::cout << "接收字节数: " << bytes_received.load() << "\n";
        std::cout << "关闭连接数: " << connections_closed.load() << "\n";
        std::cout << "==============================\n\n";
    }
};

TestStats server_stats;
TestStats client_stats;

// ============================================================================
// 测试1: 基本回显服务器
// ============================================================================
void test1_basic_echo_server() {
    std::cout << "\n[测试1] 基本回显服务器测试\n";
    std::cout << "================================\n";
    
    InetAddress server_addr(8888);
    std::atomic<EventLoop*> server_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    
    // 在单独线程中创建EventLoop和Server
    std::thread server_thread([&server_loop_ptr, &server_ptr, server_addr]() {
        EventLoop loop;
        TcpServer srv(&loop, server_addr);
        server_loop_ptr = &loop;
        server_ptr = &srv;
        
        // 设置服务器回调
        srv.setConnectionCallback([](const TcpConnectionPtr& conn) {
            if (conn->connected()) {
                server_stats.connections++;
                INFO("新连接: %s", conn->name().c_str());
            } else {
                server_stats.connections_closed++;
                INFO("连接关闭: %s", conn->name().c_str());
            }
        });
        
        srv.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
            std::string msg = buf.retrieveAllAsString();
            server_stats.messages_received++;
            server_stats.bytes_received += msg.length();
            server_stats.addReceivedMessage(msg);
            
            INFO("服务器收到: %s (长度: %zu)", msg.c_str(), msg.length());
            
            // 回显消息
            conn->send("Echo: " + msg);
            server_stats.messages_sent++;
            server_stats.bytes_sent += msg.length() + 6;
        });
        
        // 在EventLoop线程中调用start()
        loop.runInLoop([&srv]() {
            srv.start();
        });
        loop.loop();
    });
    
    // 等待服务器启动
    while (server_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 创建客户端
    InetAddress peer_addr("127.0.0.1", 8888);
    std::atomic<EventLoop*> client_loop_ptr{nullptr};
    std::atomic<TcpClient*> client_ptr{nullptr};
    
    std::atomic<bool> client_connected{false};
    std::atomic<bool> received_echo{false};
    std::atomic<bool> client_closed{false};
    std::string received_data;
    TcpConnectionPtr client_conn;
    std::mutex conn_mutex;
    
    // 在单独线程中创建EventLoop和Client
    std::thread client_thread([&client_loop_ptr, &client_ptr, peer_addr, &client_connected, &client_conn, &conn_mutex, &received_echo, &received_data, &client_closed]() {
        EventLoop loop;
        TcpClient cli(&loop, peer_addr);
        client_loop_ptr = &loop;
        client_ptr = &cli;
        
        cli.setConnectionCallback([&client_connected, &client_conn, &conn_mutex, &client_closed](const TcpConnectionPtr& conn) {
            std::lock_guard<std::mutex> lock(conn_mutex);
            if (conn->connected()) {
                client_stats.connections++;
                client_connected = true;
                client_conn = conn;
                INFO("客户端已连接");
            } else {
                client_stats.connections_closed++;
                client_conn.reset();
                client_closed = true;
                INFO("客户端连接已关闭");
            }
        });
        
        cli.setMessageCallback([&received_echo, &received_data](const TcpConnectionPtr& /*conn*/, Buffer& buf) {
            received_data = buf.retrieveAllAsString();
            client_stats.messages_received++;
            client_stats.bytes_received += received_data.length();
            received_echo = true;
            INFO("客户端收到: %s", received_data.c_str());
        });
        
        // 在EventLoop线程中调用start()
        loop.runInLoop([&cli]() {
            cli.start();
        });
        loop.loop();
    });
    
    // 等待连接建立
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 发送测试消息
    {
        std::lock_guard<std::mutex> lock(conn_mutex);
        if (client_connected.load() && client_conn && client_conn->connected()) {
            std::string test_msg = "Hello, Server!";
            client_conn->send(test_msg);
            client_stats.messages_sent++;
            client_stats.bytes_sent += test_msg.length();
            INFO("客户端发送: %s", test_msg.c_str());
        }
    }
    
    // 等待响应
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证结果
    assert(received_echo.load() && "应该收到回显消息");
    assert(received_data.find("Echo: Hello, Server!") != std::string::npos && "回显消息内容不正确");
    std::cout << "✓ 测试1通过: 基本回显功能正常\n";
    
    // 关闭连接 - 使用 shutdown 而不是 forceClose，让连接自然关闭
    {
        std::lock_guard<std::mutex> lock(conn_mutex);
        if (client_conn && client_conn->connected()) {
            client_conn->shutdown();  // 半关闭，让服务器检测到关闭
        }
    }
    
    // 等待连接关闭完成 - 给足够的时间让服务器端也关闭连接
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 清理 - 先退出 EventLoop，让它们处理完所有待处理的任务
    if (auto* loop = client_loop_ptr.load()) {
        loop->quit();
    }
    if (auto* loop = server_loop_ptr.load()) {
        loop->quit();
    }
    
    // 等待 EventLoop 线程退出
    client_thread.join();
    server_thread.join();
}

// ============================================================================
// 测试2: 多消息收发
// ============================================================================
void test2_multiple_messages() {
    std::cout << "\n[测试2] 多消息收发测试\n";
    std::cout << "================================\n";
    
    InetAddress server_addr(8889);
    std::atomic<EventLoop*> server_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    
    std::thread server_thread([&server_loop_ptr, &server_ptr, server_addr]() {
        EventLoop loop;
        TcpServer srv(&loop, server_addr);
        server_loop_ptr = &loop;
        server_ptr = &srv;
        
        srv.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
            std::string msg = buf.retrieveAllAsString();
            conn->send("Reply: " + msg);
        });
        
        loop.runInLoop([&srv]() {
            srv.start();
        });
        loop.loop();
    });
    
    while (server_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    InetAddress peer_addr("127.0.0.1", 8889);
    std::atomic<EventLoop*> client_loop_ptr{nullptr};
    std::atomic<TcpClient*> client_ptr{nullptr};
    
    std::vector<std::string> received_msgs;
    std::mutex msgs_mutex;
    TcpConnectionPtr test_client_conn;
    std::mutex test_conn_mutex;
    
    std::thread client_thread([&client_loop_ptr, &client_ptr, peer_addr, &test_client_conn, &test_conn_mutex, &received_msgs, &msgs_mutex]() {
        EventLoop loop;
        TcpClient cli(&loop, peer_addr);
        client_loop_ptr = &loop;
        client_ptr = &cli;
        
        cli.setConnectionCallback([&test_client_conn, &test_conn_mutex](const TcpConnectionPtr& conn) {
            std::lock_guard<std::mutex> lock(test_conn_mutex);
            if (conn->connected()) {
                test_client_conn = conn;
            } else {
                test_client_conn.reset();
            }
        });
        
        cli.setMessageCallback([&received_msgs, &msgs_mutex](const TcpConnectionPtr& /*conn*/, Buffer& buf) {
            std::lock_guard<std::mutex> lock(msgs_mutex);
            received_msgs.push_back(buf.retrieveAllAsString());
        });
        
        loop.runInLoop([&cli]() {
            cli.start();
        });
        loop.loop();
    });
    
    while (client_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::vector<std::string> test_messages = {
        "Message 1",
        "Message 2",
        "Message 3",
        "测试中文",
        "Large message: " + std::string(100, 'A')
    };
    
    for (const auto& msg : test_messages) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::lock_guard<std::mutex> lock(test_conn_mutex);
        if (test_client_conn && test_client_conn->connected()) {
            test_client_conn->send(msg);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 关闭连接
    {
        std::lock_guard<std::mutex> lock(test_conn_mutex);
        if (test_client_conn && test_client_conn->connected()) {
            test_client_conn->shutdown();
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证
    std::lock_guard<std::mutex> lock(msgs_mutex);
    assert(received_msgs.size() == test_messages.size() && "应该收到所有消息");
    std::cout << "✓ 测试2通过: 多消息收发正常 (收到 " << received_msgs.size() << " 条消息)\n";
    
    if (auto* loop = client_loop_ptr.load()) loop->quit();
    if (auto* loop = server_loop_ptr.load()) loop->quit();
    client_thread.join();
    server_thread.join();
}

// ============================================================================
// 测试3: 多客户端连接
// ============================================================================
void test3_multiple_clients() {
    std::cout << "\n[测试3] 多客户端连接测试\n";
    std::cout << "================================\n";
    
    InetAddress server_addr(8890);
    std::atomic<EventLoop*> server_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    std::atomic<int> connection_count{0};
    
    std::thread server_thread([&server_loop_ptr, &server_ptr, server_addr, &connection_count]() {
        EventLoop loop;
        TcpServer srv(&loop, server_addr);
        server_loop_ptr = &loop;
        server_ptr = &srv;
        
        srv.setConnectionCallback([&connection_count](const TcpConnectionPtr& conn) {
            if (conn->connected()) {
                connection_count++;
                INFO("连接 #%d: %s", connection_count.load(), conn->name().c_str());
            }
        });
        
        srv.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
            std::string msg = buf.retrieveAllAsString();
            conn->send("Server received: " + msg);
        });
        
        loop.runInLoop([&srv]() {
            srv.start();
        });
        loop.loop();
    });
    
    while (server_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 创建多个客户端
    const int num_clients = 5;
    std::vector<std::thread> client_threads;
    
    for (int i = 0; i < num_clients; i++) {
        client_threads.emplace_back([]() {
            // 在 EventLoop 线程中创建 EventLoop 和 Client
            // 这样 tid_ 就是运行 loop() 的线程 ID
            std::atomic<EventLoop*> loop_ptr{nullptr};
            std::atomic<TcpClient*> client_ptr{nullptr};
            std::atomic<bool> loop_ready{false};
            
            std::thread loop_thread([&loop_ptr, &client_ptr, &loop_ready]() {
                EventLoop loop;
                TcpClient client(&loop, InetAddress("127.0.0.1", 8890));
                loop_ptr = &loop;
                client_ptr = &client;
                
                client.setMessageCallback([](const TcpConnectionPtr& /*conn*/, Buffer& /*buf*/) {
                    // 客户端收到响应
                });
                
                loop_ready = true;
                
                // 在 EventLoop 线程中启动客户端
                loop.runInLoop([&client]() {
                    client.start();
                });
                
                loop.loop();
            });
            
            // 等待 EventLoop 创建完成
            while (!loop_ready.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            // 保持连接一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 在退出 EventLoop 之前，先关闭连接
            if (auto* client = client_ptr.load()) {
                client->disconnect();
            }
            
            // 等待连接关闭完成
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // 退出 EventLoop
            if (auto* loop = loop_ptr.load()) {
                loop->quit();
            }
            loop_thread.join();
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证连接数
    assert(connection_count.load() == num_clients && "应该有5个客户端连接");
    std::cout << "✓ 测试3通过: 多客户端连接正常 (连接数: " << connection_count.load() << ")\n";
    
    // 清理 - 先等待客户端线程退出（它们会关闭连接）
    for (auto& t : client_threads) {
        t.join();
    }
    
    // 等待连接关闭完成
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // 然后退出服务器
    if (auto* loop = server_loop_ptr.load()) loop->quit();
    
    server_thread.join();
}

// ============================================================================
// 测试4: 大数据量传输
// ============================================================================
void test4_large_data_transfer() {
    std::cout << "\n[测试4] 大数据量传输测试\n";
    std::cout << "================================\n";
    
    InetAddress server_addr(8891);
    std::atomic<EventLoop*> server_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    std::atomic<size_t> total_received{0};
    
    std::thread server_thread([&server_loop_ptr, &server_ptr, server_addr, &total_received]() {
        EventLoop loop;
        TcpServer srv(&loop, server_addr);
        server_loop_ptr = &loop;
        server_ptr = &srv;
        
        srv.setMessageCallback([&total_received](const TcpConnectionPtr& conn, Buffer& buf) {
            size_t len = buf.readableBytes();
            total_received += len;
            std::string data = buf.retrieveAllAsString();
            // 回显数据
            conn->send(data);
        });
        
        loop.runInLoop([&srv]() {
            srv.start();
        });
        loop.loop();
    });
    
    while (server_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    InetAddress peer_addr("127.0.0.1", 8891);
    std::atomic<EventLoop*> client_loop_ptr{nullptr};
    std::atomic<TcpClient*> client_ptr{nullptr};
    
    std::atomic<size_t> total_sent{0};
    std::atomic<size_t> total_echoed{0};
    TcpConnectionPtr large_data_conn;
    std::mutex large_data_mutex;
    
    std::thread client_thread([&client_loop_ptr, &client_ptr, peer_addr, &large_data_conn, &large_data_mutex, &total_echoed]() {
        EventLoop loop;
        TcpClient cli(&loop, peer_addr);
        client_loop_ptr = &loop;
        client_ptr = &cli;
        
        cli.setConnectionCallback([&large_data_conn, &large_data_mutex](const TcpConnectionPtr& conn) {
            std::lock_guard<std::mutex> lock(large_data_mutex);
            if (conn->connected()) {
                large_data_conn = conn;
            } else {
                large_data_conn.reset();
            }
        });
        
        cli.setMessageCallback([&total_echoed](const TcpConnectionPtr& /*conn*/, Buffer& buf) {
            total_echoed += buf.readableBytes();
            buf.retrieveAll();
        });
        
        loop.runInLoop([&cli]() {
            cli.start();
        });
        loop.loop();
    });
    
    while (client_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 发送大数据
    const size_t data_size = 1024 * 100; // 100KB
    std::string large_data(data_size, 'X');
    
    {
        std::lock_guard<std::mutex> lock(large_data_mutex);
        if (large_data_conn && large_data_conn->connected()) {
            large_data_conn->send(large_data);
            total_sent = data_size;
            INFO("发送大数据: %zu 字节", data_size);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 关闭连接
    {
        std::lock_guard<std::mutex> lock(large_data_mutex);
        if (large_data_conn && large_data_conn->connected()) {
            large_data_conn->shutdown();
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证
    assert(total_received.load() == data_size && "服务器应该收到所有数据");
    assert(total_echoed.load() == data_size && "客户端应该收到所有回显数据");
    std::cout << "✓ 测试4通过: 大数据量传输正常 (发送/接收: " 
              << total_sent.load() << "/" << total_received.load() << " 字节)\n";
    
    if (auto* loop = client_loop_ptr.load()) loop->quit();
    if (auto* loop = server_loop_ptr.load()) loop->quit();
    client_thread.join();
    server_thread.join();
}

// ============================================================================
// 测试5: 连接关闭
// ============================================================================
void test5_connection_close() {
    std::cout << "\n[测试5] 连接关闭测试\n";
    std::cout << "================================\n";
    
    InetAddress server_addr(8892);
    std::atomic<EventLoop*> server_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    std::atomic<bool> connection_closed{false};
    
    std::thread server_thread([&server_loop_ptr, &server_ptr, server_addr, &connection_closed]() {
        EventLoop loop;
        TcpServer srv(&loop, server_addr);
        server_loop_ptr = &loop;
        server_ptr = &srv;
        
        srv.setConnectionCallback([&connection_closed](const TcpConnectionPtr& conn) {
            if (!conn->connected()) {
                connection_closed = true;
                INFO("服务器检测到连接关闭");
            }
        });
        
        loop.runInLoop([&srv]() {
            srv.start();
        });
        loop.loop();
    });
    
    while (server_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    InetAddress peer_addr("127.0.0.1", 8892);
    std::atomic<EventLoop*> client_loop_ptr{nullptr};
    std::atomic<TcpClient*> client_ptr{nullptr};
    
    std::atomic<bool> client_closed{false};
    TcpConnectionPtr close_test_conn;
    std::mutex close_test_mutex;
    
    std::thread client_thread([&client_loop_ptr, &client_ptr, peer_addr, &client_closed, &close_test_conn, &close_test_mutex]() {
        EventLoop loop;
        TcpClient cli(&loop, peer_addr);
        client_loop_ptr = &loop;
        client_ptr = &cli;
        
        cli.setConnectionCallback([&client_closed, &close_test_conn, &close_test_mutex](const TcpConnectionPtr& conn) {
            std::lock_guard<std::mutex> lock(close_test_mutex);
            if (conn->connected()) {
                close_test_conn = conn;
            } else {
                close_test_conn.reset();
                client_closed = true;
                INFO("客户端检测到连接关闭");
            }
        });
        
        loop.runInLoop([&cli]() {
            cli.start();
        });
        loop.loop();
    });
    
    while (client_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 客户端主动关闭
    {
        std::lock_guard<std::mutex> lock(close_test_mutex);
        if (close_test_conn && close_test_conn->connected()) {
            close_test_conn->shutdown();
            INFO("客户端关闭连接");
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 验证
    assert(connection_closed.load() && "服务器应该检测到连接关闭");
    std::cout << "✓ 测试5通过: 连接关闭正常\n";
    
    if (auto* loop = client_loop_ptr.load()) loop->quit();
    if (auto* loop = server_loop_ptr.load()) loop->quit();
    client_thread.join();
    server_thread.join();
}

// ============================================================================
// 测试6: 多线程服务器
// ============================================================================
void test6_multithreaded_server() {
    std::cout << "\n[测试6] 多线程服务器测试\n";
    std::cout << "================================\n";
    
    InetAddress server_addr(8893);
    std::atomic<EventLoop*> base_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    std::atomic<int> thread_init_count{0};
    std::atomic<int> connection_count{0};
    
    std::thread server_thread([&base_loop_ptr, &server_ptr, server_addr, &thread_init_count, &connection_count]() {
        EventLoop base_loop;
        TcpServer srv(&base_loop, server_addr);
        base_loop_ptr = &base_loop;
        server_ptr = &srv;
        
        // 设置多线程
        srv.setNumThread(4);
        
        srv.setThreadInitCallback([&thread_init_count](size_t index) {
            thread_init_count++;
            INFO("线程 #%zu 初始化", index);
        });
        
        srv.setConnectionCallback([&connection_count](const TcpConnectionPtr& conn) {
            if (conn->connected()) {
                connection_count++;
            }
        });
        
        srv.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
            std::string msg = buf.retrieveAllAsString();
            conn->send("Echo: " + msg);
        });
        
        base_loop.runInLoop([&srv]() {
            srv.start();
        });
        base_loop.loop();
    });
    
    while (base_loop_ptr.load() == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 创建多个客户端测试多线程处理
    const int num_clients = 10;
    std::vector<std::thread> client_threads;
    
    // 使用原子变量来存储客户端指针，以便在退出前关闭连接
    std::vector<std::atomic<TcpClient*>> client_ptrs(num_clients);
    std::vector<std::atomic<EventLoop*>> client_loop_ptrs(num_clients);
    
    for (int i = 0; i < num_clients; i++) {
        client_ptrs[i] = nullptr;
        client_loop_ptrs[i] = nullptr;
        
        client_threads.emplace_back([&client_ptrs, &client_loop_ptrs, i]() {
            EventLoop loop;
            TcpClient client(&loop, InetAddress("127.0.0.1", 8893));
            client_loop_ptrs[i] = &loop;
            client_ptrs[i] = &client;
            
            client.setMessageCallback([](const TcpConnectionPtr& /*conn*/, Buffer& /*buf*/) {
                // 收到回显
            });
            
            loop.runInLoop([&client]() {
                client.start();
            });
            loop.loop();
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 验证
    assert(thread_init_count.load() >= 4 && "应该初始化至少4个线程");
    assert(connection_count.load() == num_clients && "应该有10个客户端连接");
    std::cout << "✓ 测试6通过: 多线程服务器正常 (线程数: " 
              << thread_init_count.load() << ", 连接数: " << connection_count.load() << ")\n";
    
    // 在退出 EventLoop 之前，先关闭所有客户端连接
    for (int i = 0; i < num_clients; i++) {
        if (auto* client = client_ptrs[i].load()) {
            client->disconnect();
        }
    }
    
    // 等待连接关闭完成
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // 退出所有客户端 EventLoop
    for (int i = 0; i < num_clients; i++) {
        if (auto* loop = client_loop_ptrs[i].load()) {
            loop->quit();
        }
    }
    
    // 等待客户端线程退出
    for (auto& t : client_threads) {
        t.join();
    }
    
    // 退出服务器 EventLoop
    if (auto* loop = base_loop_ptr.load()) loop->quit();
    
    // 等待服务器线程退出
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server_thread.join();
}

// ============================================================================
// 主函数
// ============================================================================
int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "    KnetLib 网络库完整测试\n";
    std::cout << "========================================\n";
    
    try {
        test1_basic_echo_server();
        test2_multiple_messages();
        test3_multiple_clients();
        test4_large_data_transfer();
        test5_connection_close();
        test6_multithreaded_server();
        
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "    所有测试通过！✓\n";
        std::cout << "========================================\n";
        
        // 打印统计信息
        server_stats.print();
        client_stats.print();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n测试失败: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n测试失败: 未知错误\n";
        return 1;
    }
}


// 专门的内存泄漏检测测试
#include "knetlib/TcpServer.h"
#include "knetlib/TcpClient.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/Buffer.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
#include "knetlib/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <memory>

// 测试多次连接和断开，检查是否有内存泄漏
void test_memory_leak() {
    std::cout << "\n[内存泄漏测试] 多次连接/断开测试\n";
    std::cout << "================================\n";
    
    const int TEST_ROUNDS = 100;
    const int CONNECTIONS_PER_ROUND = 5;
    
    InetAddress server_addr(9999);
    std::atomic<EventLoop*> server_loop_ptr{nullptr};
    std::atomic<TcpServer*> server_ptr{nullptr};
    std::atomic<int> total_connections{0};
    
    // 服务器线程
    std::thread server_thread([&server_loop_ptr, &server_ptr, server_addr, &total_connections]() {
        EventLoop loop;
        TcpServer srv(&loop, server_addr);
        server_loop_ptr = &loop;
        server_ptr = &srv;
        
        srv.setConnectionCallback([&total_connections](const TcpConnectionPtr& conn) {
            if (conn->connected()) {
                total_connections++;
            }
        });
        
        srv.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
            std::string msg = buf.retrieveAllAsString();
            conn->send("Echo: " + msg);
        });
        
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
    
    // 运行多轮测试
    for (int round = 0; round < TEST_ROUNDS; round++) {
        std::vector<std::thread> client_threads;
        
        // 每轮创建多个客户端
        for (int i = 0; i < CONNECTIONS_PER_ROUND; i++) {
            client_threads.emplace_back([round, i]() {
                EventLoop loop;
                TcpClient client(&loop, InetAddress("127.0.0.1", 9999));
                
                std::atomic<bool> connected{false};
                TcpConnectionPtr conn;
                std::mutex conn_mutex;
                
                client.setConnectionCallback([&connected, &conn, &conn_mutex](const TcpConnectionPtr& c) {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    if (c->connected()) {
                        connected = true;
                        conn = c;
                    } else {
                        conn.reset();
                    }
                });
                
                client.setMessageCallback([](const TcpConnectionPtr& /*conn*/, Buffer& /*buf*/) {
                    // 接收消息
                });
                
                loop.runInLoop([&client]() {
                    client.start();
                });
                
                // 等待连接建立
                int wait_count = 0;
                while (!connected.load() && wait_count < 50) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    wait_count++;
                }
                
                // 发送消息
                {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    if (conn && conn->connected()) {
                        conn->send("Test message from round " + std::to_string(round) + " client " + std::to_string(i));
                    }
                }
                
                // 等待响应
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                
                // 断开连接
                {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    if (conn && conn->connected()) {
                        conn->shutdown();
                    }
                }
                
                // 等待断开完成
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // 退出EventLoop
                loop.quit();
                loop.loop();
            });
        }
        
        // 等待所有客户端完成
        for (auto& t : client_threads) {
            t.join();
        }
        
        // 每10轮输出一次进度
        if ((round + 1) % 10 == 0) {
            std::cout << "  完成 " << (round + 1) << "/" << TEST_ROUNDS << " 轮测试\n";
        }
        
        // 短暂休息，让系统清理资源
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "✓ 完成 " << TEST_ROUNDS << " 轮测试，每轮 " << CONNECTIONS_PER_ROUND << " 个连接\n";
    std::cout << "  总连接数: " << total_connections.load() << "\n";
    
    // 退出服务器
    if (auto* loop = server_loop_ptr.load()) {
        loop->quit();
    }
    server_thread.join();
    
    std::cout << "✓ 内存泄漏测试完成\n";
    std::cout << "  如果使用 AddressSanitizer 编译，请检查是否有内存泄漏报告\n";
}

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "   KnetLib 内存泄漏检测测试\n";
    std::cout << "========================================\n";
    
    try {
        test_memory_leak();
        
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "    测试完成\n";
        std::cout << "========================================\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n测试失败: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n测试失败: 未知错误\n";
        return 1;
    }
}


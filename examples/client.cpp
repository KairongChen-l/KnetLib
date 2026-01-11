#include <iostream>
#include "knetlib/TcpClient.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
#include "knetlib/Buffer.h"
#include "knetlib/Logger.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <signal.h>
#include <csignal>

// 全局变量用于控制客户端
EventLoop* g_client_loop = nullptr;
TcpConnectionPtr g_connection = nullptr;
std::mutex g_conn_mutex;
std::atomic<bool> g_connected{false};
std::atomic<bool> g_should_exit{false};

void signalHandler(int sig) {
    if (g_client_loop) {
        std::cout << "\n收到信号 " << sig << "，正在退出...\n";
        g_should_exit = true;
        {
            std::lock_guard<std::mutex> lock(g_conn_mutex);
            if (g_connection && g_connection->connected()) {
                g_connection->shutdown();
            }
        }
        g_client_loop->quit();
    }
}

// 输入线程函数
void inputThread() {
    // 等待连接建立
    int wait_count = 0;
    while (!g_should_exit && !g_connected.load() && wait_count < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        wait_count++;
    }
    
    if (!g_connected.load()) {
        std::cout << "[错误] 连接超时，无法建立连接\n";
        g_should_exit = true;
        if (g_client_loop) {
            g_client_loop->quit();
        }
        return;
    }
    
    std::string line;
    while (!g_should_exit && std::getline(std::cin, line)) {
        if (g_should_exit) break;
        
        // 检查是否是退出命令
        if (line == "quit" || line == "exit") {
            g_should_exit = true;
            break;
        }
        
        std::lock_guard<std::mutex> lock(g_conn_mutex);
        if (g_connection && g_connection->connected()) {
            g_connection->send(line);
            std::cout << "[发送] " << line << std::endl;
        } else {
            std::cout << "[错误] 连接已断开，无法发送消息\n";
            break;
        }
    }
    g_should_exit = true;
    if (g_client_loop) {
        g_client_loop->quit();
    }
}

int main(int argc, char* argv[]) {
    // 设置日志级别
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    
    // 解析命令行参数
    std::string server_ip = "127.0.0.1";
    uint16_t server_port = 8888;
    
    if (argc >= 2) {
        server_port = static_cast<uint16_t>(std::stoi(argv[1]));
    }
    if (argc >= 3) {
        server_ip = argv[2];
    }
    
    std::cout << "连接到服务器: " << server_ip << ":" << server_port << std::endl;
    
    EventLoop loop;
    g_client_loop = &loop;
    
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    InetAddress peer_addr(server_ip, server_port);
    TcpClient client(&loop, peer_addr);
    
    // 设置连接回调
    client.setConnectionCallback([](const TcpConnectionPtr& conn) {
        std::lock_guard<std::mutex> lock(g_conn_mutex);
        if (conn->connected()) {
            g_connection = conn;
            g_connected = true;
            INFO("已连接到服务器: %s", conn->name().c_str());
            std::cout << "[连接] 已连接到服务器，可以开始发送消息\n";
            std::cout << "[提示] 输入消息并按回车发送，输入 quit 或 exit 退出\n";
        } else {
            g_connected = false;
            g_connection.reset();
            INFO("连接已断开");
            std::cout << "[断开] 连接已断开\n";
        }
    });
    
    // 设置消息回调
    client.setMessageCallback([](const TcpConnectionPtr& /*conn*/, Buffer& buf) {
        std::string msg = buf.retrieveAllAsString();
        std::cout << "[接收] " << msg << std::endl;
    });
    
    // 设置错误回调
    client.setErrorCallback([]() {
        WARN("连接错误");
        std::cout << "[错误] 连接发生错误\n";
        g_should_exit = true;
        if (g_client_loop) {
            g_client_loop->quit();
        }
    });
    
    // 启动输入线程
    std::thread input_thread(inputThread);
    
    // 在EventLoop线程中启动客户端
    loop.runInLoop([&client]() {
        client.start();
    });
    
    // 运行事件循环
    loop.loop();
    
    // 等待输入线程退出
    if (input_thread.joinable()) {
        input_thread.join();
    }
    
    INFO("客户端退出");
    return 0;
}

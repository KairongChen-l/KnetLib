#include "src/EventLoop.h"
#include "src/TcpServer.h"
#include "src/TcpConnection.h"
#include "src/InetAddress.h"
#include "src/Buffer.h"
#include "src/Logger.h"
#include <iostream>
#include <signal.h>
#include <csignal>

// 全局变量用于优雅退出
EventLoop* g_loop = nullptr;

void signalHandler(int sig) {
    if (g_loop) {
        std::cout << "\n收到信号 " << sig << "，正在优雅退出...\n";
        g_loop->quit();
    }
}

int main() {
    // 设置日志级别
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    
    EventLoop loop;
    g_loop = &loop;
    
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    InetAddress local(8888);  // 监听端口 8888
    TcpServer server(&loop, local);
    
    // 设置连接回调
    server.setConnectionCallback([](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            INFO("新连接建立: %s", conn->name().c_str());
        } else {
            INFO("连接关闭: %s", conn->name().c_str());
        }
    });
    
    // 设置消息回调 - 回显服务器
    server.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
        std::string msg = buf.retrieveAllAsString();
        INFO("收到消息 (长度: %zu): %s", msg.length(), msg.c_str());
        
        // 回显消息
        conn->send("Echo: " + msg);
        INFO("发送回显: Echo: %s", msg.c_str());
    });
    
    // 启动服务器
    server.start();
    INFO("服务器启动，监听端口 8888");
    
    // 运行事件循环
    loop.loop();
    
    INFO("服务器退出");
    return 0;
}
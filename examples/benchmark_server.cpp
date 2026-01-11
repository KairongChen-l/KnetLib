#include "knetlib/EventLoop.h"
#include "knetlib/TcpServer.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/InetAddress.h"
#include "knetlib/Buffer.h"
#include "knetlib/Logger.h"
#include <iostream>
#include <signal.h>
#include <csignal>
#include <string>
#include <sstream>
#include <unordered_map>

// 全局变量用于优雅退出
EventLoop* g_loop = nullptr;

void signalHandler(int sig) {
    if (g_loop) {
        std::cout << "\n收到信号 " << sig << "，正在优雅退出...\n";
        g_loop->quit();
    }
}

// 简单的 HTTP 请求解析
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    bool parse(const std::string& raw) {
        std::istringstream iss(raw);
        std::string line;
        
        // 解析请求行
        if (!std::getline(iss, line)) return false;
        std::istringstream requestLine(line);
        requestLine >> method >> path >> version;
        
        // 解析头部
        while (std::getline(iss, line) && line != "\r" && !line.empty()) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                // 去除前后空格
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                headers[key] = value;
            }
        }
        
        return true;
    }
};

// 生成 HTTP 响应
std::string generateHttpResponse(const std::string& content, const std::string& contentType = "text/html") {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << content.length() << "\r\n";
    oss << "Connection: keep-alive\r\n";
    oss << "Server: KnetLib/1.0\r\n";
    oss << "\r\n";
    oss << content;
    return oss.str();
}

// 生成简单的 HTML 响应
std::string generateHtmlResponse(const std::string& title, const std::string& body) {
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n";
    oss << "<html><head><title>" << title << "</title></head>\n";
    oss << "<body><h1>" << title << "</h1>\n";
    oss << "<p>" << body << "</p>\n";
    oss << "</body></html>\n";
    return oss.str();
}

int main(int argc, char* argv[]) {
    // 设置日志级别（性能测试时减少日志输出）
    setLogLevel(LOG_LEVEL::LOG_LEVEL_WARN);
    
    int port = 8888;
    size_t numThreads = 1;
    
    // 解析命令行参数
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    if (argc > 2) {
        numThreads = std::stoul(argv[2]);
    }
    
    EventLoop loop;
    g_loop = &loop;
    
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    InetAddress local(port);
    TcpServer server(&loop, local);
    
    // 设置连接回调
    server.setConnectionCallback([](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            // 连接建立，静默处理（减少日志输出）
        } else {
            // 连接关闭，静默处理
        }
    });
    
    // 设置消息回调 - HTTP 服务器
    server.setMessageCallback([&](const TcpConnectionPtr& conn, Buffer& buf) {
        std::string request = buf.retrieveAllAsString();
        
        HttpRequest httpReq;
        if (httpReq.parse(request)) {
            // 处理不同的路径
            std::string response;
            
            if (httpReq.path == "/" || httpReq.path == "/index.html") {
                std::string html = generateHtmlResponse("KnetLib Performance Test", 
                    "Welcome to KnetLib HTTP Server Performance Test");
                response = generateHttpResponse(html);
            } else if (httpReq.path == "/hello") {
                std::string html = generateHtmlResponse("Hello", "Hello from KnetLib!");
                response = generateHttpResponse(html);
            } else if (httpReq.path == "/plain") {
                response = generateHttpResponse("OK", "text/plain");
            } else if (httpReq.path == "/json") {
                std::string json = "{\"status\":\"ok\",\"message\":\"KnetLib Performance Test\"}";
                response = generateHttpResponse(json, "application/json");
            } else {
                // 404 Not Found
                std::ostringstream oss;
                oss << "HTTP/1.1 404 Not Found\r\n";
                oss << "Content-Type: text/html\r\n";
                oss << "Content-Length: 0\r\n";
                oss << "Connection: keep-alive\r\n";
                oss << "\r\n";
                response = oss.str();
            }
            
            conn->send(response);
        } else {
            // 解析失败，返回 400 Bad Request
            std::ostringstream oss;
            oss << "HTTP/1.1 400 Bad Request\r\n";
            oss << "Content-Type: text/html\r\n";
            oss << "Content-Length: 0\r\n";
            oss << "Connection: close\r\n";
            oss << "\r\n";
            conn->send(oss.str());
        }
    });
    
    // 设置线程数（必须在 start() 之前，且必须在 EventLoop 线程中）
    if (numThreads > 1) {
        server.setNumThread(numThreads);
    }
    
    // 启动服务器（这会通过 runInLoop 异步执行，但我们在 EventLoop 线程中，所以会立即执行）
    server.start();
    
    // 确保服务器真正启动（通过运行一次 EventLoop 迭代来处理 pending tasks）
    // 由于我们在 EventLoop 线程中，runInLoop 会立即执行，但我们需要确保它完成
    loop.runInLoop([](){}); // 触发一次任务处理，确保 startInLoop 执行
    
    std::cout << "HTTP 性能测试服务器启动，监听端口 " << port;
    if (numThreads > 1) {
        std::cout << "，使用 " << numThreads << " 个工作线程";
    }
    std::cout << std::endl;
    std::cout << "可用端点:" << std::endl;
    std::cout << "  GET /              - HTML 响应" << std::endl;
    std::cout << "  GET /hello         - Hello 页面" << std::endl;
    std::cout << "  GET /plain         - 纯文本响应" << std::endl;
    std::cout << "  GET /json          - JSON 响应" << std::endl;
    
    // 运行事件循环
    loop.loop();
    
    std::cout << "服务器退出" << std::endl;
    return 0;
}


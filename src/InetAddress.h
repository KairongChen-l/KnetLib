#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, bool loopback = false);
    InetAddress(const std::string& ip, uint16_t port);
    InetAddress();  // 默认构造，用于 accept

    void setAddress(const sockaddr_in& addr);
    const sockaddr* getSockaddr() const;
    socklen_t getSocklen() const;

    std::string toIp() const;
    uint16_t toPort() const;
    std::string toIpPort() const;

    // 兼容旧 API
    void setInetAddr(sockaddr_in _addr, socklen_t _addr_len);
    sockaddr_in getAddr() const;
    socklen_t getAddr_len() const;

private:
    struct sockaddr_in addr_;
};

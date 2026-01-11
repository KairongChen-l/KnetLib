#include "knetlib/InetAddress.h"
#include "knetlib/utils.h"
#include <arpa/inet.h>
#include <strings.h>
#include <cstring>

InetAddress::InetAddress(uint16_t port, bool loopback) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopback ? INADDR_LOOPBACK : INADDR_ANY;
    addr_.sin_addr.s_addr = htonl(ip);
    addr_.sin_port = htons(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    int ret = inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    if (ret != 1) {
        errif(true, "InetAddress::inet_pton");
    }
    addr_.sin_port = htons(port);
}

InetAddress::InetAddress() {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
}

void InetAddress::setAddress(const sockaddr_in& addr) {
    addr_ = addr;
}

const sockaddr* InetAddress::getSockaddr() const {
    return reinterpret_cast<const sockaddr*>(&addr_);
}

socklen_t InetAddress::getSocklen() const {
    return sizeof(addr_);
}

std::string InetAddress::toIp() const {
    char buf[INET_ADDRSTRLEN];
    const char* ret = inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    if (ret == nullptr) {
        buf[0] = '\0';
        // 错误处理：使用 utils 中的 errif
    }
    return std::string(buf);
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

std::string InetAddress::toIpPort() const {
    std::string ret = toIp();
    ret.push_back(':');
    ret += std::to_string(toPort());
    return ret;
}

// 兼容旧 API
void InetAddress::setInetAddr(sockaddr_in _addr, socklen_t _addr_len) {
    (void)_addr_len;  // 忽略，因为大小固定
    addr_ = _addr;
}

sockaddr_in InetAddress::getAddr() const {
    return addr_;
}

socklen_t InetAddress::getAddr_len() const {
    return sizeof(addr_);
}

#include <gtest/gtest.h>
#include "knetlib/InetAddress.h"
#include <arpa/inet.h>

class InetAddressTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// 测试端口构造
TEST_F(InetAddressTest, PortConstructor) {
    InetAddress addr(8888);
    
    EXPECT_EQ(addr.toPort(), 8888);
    EXPECT_EQ(addr.toIp(), "0.0.0.0");  // INADDR_ANY
}

// 测试 IP 和端口构造
TEST_F(InetAddressTest, IpPortConstructor) {
    InetAddress addr("127.0.0.1", 1234);
    
    EXPECT_EQ(addr.toPort(), 1234);
    EXPECT_EQ(addr.toIp(), "127.0.0.1");
}

// 测试 toIpPort
TEST_F(InetAddressTest, ToIpPort) {
    InetAddress addr("192.168.1.1", 8080);
    std::string ipPort = addr.toIpPort();
    
    EXPECT_NE(ipPort.find("192.168.1.1"), std::string::npos);
    EXPECT_NE(ipPort.find("8080"), std::string::npos);
}

// 测试 setAddress
TEST_F(InetAddressTest, SetAddress) {
    InetAddress addr;
    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, "10.0.0.1", &sa.sin_addr);
    sa.sin_port = htons(9999);
    
    addr.setAddress(sa);
    
    EXPECT_EQ(addr.toPort(), 9999);
    EXPECT_EQ(addr.toIp(), "10.0.0.1");
}

// 测试 getSockaddr 和 getSocklen
TEST_F(InetAddressTest, GetSockaddr) {
    InetAddress addr("127.0.0.1", 8888);
    
    const sockaddr* sa = addr.getSockaddr();
    socklen_t len = addr.getSocklen();
    
    EXPECT_NE(sa, nullptr);
    EXPECT_EQ(len, sizeof(sockaddr_in));
}


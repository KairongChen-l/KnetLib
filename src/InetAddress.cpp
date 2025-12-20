#include "InetAddress.h"
#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

InetAddress::InetAddress():addr_len(sizeof(addr)){
    bzero(&addr, addr_len);
}

InetAddress::~InetAddress(){

}

InetAddress::InetAddress(const char* ip,uint16_t port):addr_len(sizeof(addr)){
    bzero(&addr, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    addr_len = sizeof(addr);
}

void InetAddress::setInetAddr(sockaddr_in _addr, socklen_t _addr_len){
    addr = _addr;
    addr_len = _addr_len;
}

sockaddr_in InetAddress::getAddr(){
    return addr;
}

socklen_t InetAddress::getAddr_len(){
    return addr_len;
}
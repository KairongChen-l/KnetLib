#include "knetlib/Connection.h"
#include "knetlib/Socket.h"
#include "knetlib/Channel.h"
#include "knetlib/utils.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <functional>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>

#define READ_BUFFER_SIZE 1024


Connection::Connection(EventLoop* _loop,Socket* _sock):loop(_loop),sock(_sock), chan(nullptr), inBuffer(new std::string()), readBuffer(nullptr){
    chan = new Channel(loop,sock->getFd());
    chan->enableRead();
    chan->useET();
    std::function<void()> cb = std::bind(&Connection::echo,this,sock->getFd());
    chan->setReadCallback(cb);
    chan->setUseThreadPool(true);
    readBuffer = new Buffer();
}

Connection::~Connection(){
    delete chan;
    delete sock;
    delete readBuffer;
    delete inBuffer;  
}

void Connection::echo(int sockfd){
    char buf[READ_BUFFER_SIZE];
    while (true) {    //使用非阻塞IO需要一次性读完所有数据，因为内核只会通知一次
        bzero(&buf, sizeof(buf));
        ssize_t bytes_read = read(sockfd,buf,sizeof(buf));
        if(bytes_read > 0){
            readBuffer->append(buf, bytes_read);
        }else if(bytes_read == -1 && errno == EINTR){//客户端正常中断，继续读取
            std::cout<<"continue reading\n";
            continue;
        }else if(bytes_read == -1 &&  ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
            std::cout<<"finish reading once, errno:"<<errno<<"\n";
            printf("message from client fd %d: %s\n", sockfd, readBuffer->c_str());
            errif(write(sockfd, readBuffer->c_str(), readBuffer->size()) == -1, "socket write error");
            break;
        }else if (bytes_read == 0) {
            std::cout<<"EOF,client fd "<<sockfd << "\n";
            deleteConnectionCallBack(sockfd);          //多线程有bug
            break;
        }else{
            printf("Connection reset by peer\n");
            deleteConnectionCallBack(sockfd);          //会有bug，注释后单线程无bug
            break;
        }
    }
}

void Connection::setDeleteConnectionCallBack(std::function<void(int)> _cb){
    deleteConnectionCallBack = _cb;
}

void Connection::send(int sockfd){
    // 修复：使用动态分配或 std::vector 替代 VLA（变长数组），VLA 不是 C++ 标准
    size_t data_size = readBuffer->size();
    if(data_size == 0) return;
    std::vector<char> buf(data_size);
    memcpy(buf.data(), readBuffer->c_str(), data_size);
    size_t data_left = data_size; 
    while (data_left > 0) 
    { 
        ssize_t bytes_write = write(sockfd, buf.data() + data_size - data_left, data_left); 
        if (bytes_write == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            // 修复：处理其他错误情况
            return;
        }
        data_left -= bytes_write; 
    }
}
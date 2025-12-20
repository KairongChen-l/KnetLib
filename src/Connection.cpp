#include "Connection.h"
#include "Socket.h"
#include "Channel.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <functional>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#define READ_BUFFER_SIZE 1024


Connection::Connection(EventLoop* _loop,Socket* _sock):loop(_loop),sock(_sock){
    chan = new Channel(loop,sock->getFd());
    std::function<void()> cb = std::bind(&Connection::echo,this,sock->getFd());
    chan->setCallback(cb);
    chan->enableReading();
}

Connection::~Connection(){
    delete chan;
    delete sock;
}

void Connection::echo(int sockfd){
    char buf[READ_BUFFER_SIZE];
    //使用非阻塞IO需要一次性读完所有数据，
    while (true) {
        bzero(&buf, sizeof(buf));
        ssize_t bytes_read = read(sockfd,buf,sizeof(buf));
        if(bytes_read > 0){
            std::cout<<"message from client fd"<< sockfd <<":"<< buf<<"\n";
            write(sockfd, buf, sizeof(buf));
        }else if(bytes_read == -1 && errno == EINTR){//客户端正常中断，继续读取
            std::cout<<"continue reading\n";
            continue;
        }else if(bytes_read == -1 &&  ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
            std::cout<<"finish reading once, errno:"<<errno<<"\n";
            break;
        }else if (bytes_read == 0) {
            std::cout<<"EOF,client fd "<<sockfd << "\n";
            deleteConnectionCallBack(sock);
            break;
        }
    }
}

void Connection::setDeleteConnectionCallBack(std::function<void (Socket *)> _cb){
    deleteConnectionCallBack = _cb;
}

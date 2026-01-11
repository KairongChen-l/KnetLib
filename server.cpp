#include "src/EventLoop.h"
#include "src/Server.h"
#include "src/InetAddress.h"

int main(){
    EventLoop *loop = new EventLoop();
    InetAddress local(8888);  // 监听端口 8888
    [[maybe_unused]]Server *server = new Server(loop, local);
    loop->loop();
    delete server;
    delete loop;
    return 0;
}
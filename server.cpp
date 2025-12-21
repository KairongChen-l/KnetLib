#include "src/EventLoop.h"
#include "src/Server.h"

int main(){
    EventLoop *loop = new EventLoop();
    [[maybe_unused]]Server *server = new Server(loop);
    loop->loop();
    delete server;
    delete loop;
    return 0;
}
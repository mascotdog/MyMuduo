#include "Poller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop * loop){
    if(::getenv("MUDUO_USE_POLL")){
        // TODO 生成POLLPOLL的实例
        return nullptr;
    }else{
        // TODO 生成EPOLLPOLL的实例
        return nullptr;
    }
}
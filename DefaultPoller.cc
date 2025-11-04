#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop * loop){
    if(::getenv("MUDUO_USE_POLL")){
        // TODO 生成POLLPOLL的实例
        return nullptr;
    }else{
        return new EPollPoller(loop);
    }
}
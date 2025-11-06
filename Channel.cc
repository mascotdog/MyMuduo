#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

Channel::~Channel() {}

// Channel的tie方法什么时候调用过？ 一个TcpConnection新链接创建的时候 TcpConnection => channel
void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}

/**
 * 当改变channel所表示的fd的events事件后，updatea负责在poller里面更改fd相应的事件
 * epoll_ctl EventLoop => ChannelList Poller
 */
void Channel::update() {
    // 通过Channel所属的EventLoop, 调用Poller的相应方法，注册fd的events事件

    loop_->updateChannel(this);
}

/**
 * 在Channel所属的EventLoop中吧当前的Channel删除掉
 */
void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp recieveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(recieveTime);
        }
    } else {
        handleEventWithGuard(recieveTime);
    }
}

// 根据poller通知的Channel发生的具体事件，由Channel负责调用对应的回调事件
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        }
    }
    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
    if (revents_ &(EPOLLIN | EPOLLPRI)) {
        if (readCallBack_) {
            readCallBack_(receiveTime);
        }
    }
    if (revents_ & EPOLLOUT) {
        if (writeCallBack_) {
            writeCallBack_();
        }
    }
}

#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

// channel未添加到poller中
const int kNew = -1; // channel的成员index_ = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel 从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() { ::close(epollfd_); }

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    // 并发环境下请求数量脚都，该函数被频繁调用，实际使用LOG_DEBUG较好
    LOG_INFO("func=%s => fd total count:%d\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    // 保证可重入性
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0) {
        LOG_INFO("%d events happened \n ", numEvents);
        fillActiceChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller
// updateChannel removeChannel
/**
 *                  EventLoop => poller.poll
 *  ChannelList                     Poller
 *                          ChannelMap <fd,channel*>
 *  ChannelList 里包含所有的事件
 *  ChannelMap 里包含注册过的事件
 *  ChannelList >= ChannelMap
 */
void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__,
             channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else { // channel已经在poller上处注册过了
        int fd = channel->fd();
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除Channel
void EPollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
    // 注册过的需要在三个地方都执行对应的逻辑
}

void EPollPoller::fillActiceChannels(int numEvents,
                                     ChannelList *activeChannels) const {
    for (int i = 0; i < numEvents; i++) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了它的poller给他返回的所有发生时间的channel列表
    } 
}

// epoll_ctl mod/add/del 的具体操作
void EPollPoller::update(int operation, Channel *channel) {
    epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    int fd = channel->fd();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_INFO("epoll_ctl del error:%d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

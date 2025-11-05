#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/eventfd.h>
#include <unistd.h>

// 防止一个线程创建多个EventLoop thrad_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创造wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventFd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}
// 主线程是可以拿到子线程的loop的，进而进行跨线程的调用
EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventFd()), wakeupChannel_(new Channel(this, wakeupFd_)) {
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",
                  t_loopInThisThread, threadId_);

    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeupFd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::Loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_) {
        activeChannels_.clear();
        // 监听两类fd 一种是client的fd 一种是wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_) {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程 mainLoop accept fd <= channel 分发给 subLoop
         * mainLoop 事先注册一个回调cb （需要subLoop执行） wakeup
         * wakeup subloop后，执行之前mainLoop注册的cb操作
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出当前 EventLoop 的事件循环
// quit() 可能在任意线程被调用：
//  如果在当前 EventLoop 所属线程中调用：
//     - 直接设置 quit_ = true
//     - 下一次事件循环检测到后自然退出
//  如果在其他线程中调用：
//     - 仅设置 quit_ 不够，因为可能阻塞在 epoll_wait()
//     - wakeup() 用来唤醒 EventLoop，使其及时跳出阻塞并退出
void EventLoop::quit() {
    quit_ = true;

    // 如果当前线程不是 EventLoop 所属线程
    // 必须唤醒 EventLoop 使其从 poll/epoll_wait() 中返回
    if (!isInLoopThread()) {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) // 在当前的loop线程中执行cb
    {
        cb();
    } else { // 在非当前loop线程中执行cb，就需要唤醒loop所在线程执行cb
        queueInLoop(cb);
    }
}
// 把cb放入队列当中，唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面的回调操作的loop线程了
    // callingPendingFcnctors_的意思是当前loop正在执行回调，但是loop又有了新的回调，
    if (!isInLoopThread() || callingPendingFcnctors_) {
        wakeup(); // 唤醒loop所在线程
    }
}

// 用来唤醒loop所在的线程的，向wakeupFd——写一个数据
// wakeup Channel发生了读事件
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() write %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

// 此处由于可能有多个其他线程调用，导致访问pendingFunctors可能会阻塞
// 因此通过临时变量快速跳过阻塞部分，大大提高了并发效率
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFcnctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors) {
        functor();
    }

    callingPendingFcnctors_ = false;
}

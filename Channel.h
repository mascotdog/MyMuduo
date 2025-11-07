#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <functional>
#include <memory>

// 前向声明
// 减少头文件之间的依赖 加快编译速度 避免循环依赖 降低耦合性
// 适用于指针或引用成员 因为只需要知道对应的类是一个类型
// 不适用于定义对象 因为结构大小未知
class EventLoop;

/**
 * 理清楚 EventLoop,Channel,Poller之间的关系，前者包含后两者 Reactor模型上对应Demultiplex
 * Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN，EPOLLOUT时间
 * 还绑定了poller返回的具体事件
 */

class Channel : noncopyable {
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理时间的方法
    void handleEvent(Timestamp recieveTime);

    // 设置回调函数对象
    void setReadCallBack(ReadEventCallBack cb) {readCallBack_ = std::move(cb);}
    void setWriteCallBack(EventCallBack cb) { writeCallBack_ = std::move(cb); }
    void setCloseCallBack(EventCallBack cb) { closeCallback_ = std::move(cb); }
    void setErrorCallBack(EventCallBack cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove后，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd()const {return fd_;}
    int events()const {return events_;}
    void set_revents(int revt){revents_ = revt;}

    // 设置fd相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    void disableAll() { events_ = kNoneEvent; update();}

    bool isNoneEvent() const{return events_ == kNoneEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}
    bool isReading() const {return events_ & kReadEvent;}

    int index() {return index_;}
    void set_index(int idx) { index_ = idx;}

    // one loop per thread
    EventLoop*  onwerLoop(){return loop_;}
    void remove();
    
private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // 状态
    static const int kNoneEvent;
    static const int kWriteEvent;
    static const int kReadEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    //  poller监听的对象
    int events_;       // 注册fd感兴趣的事件
    int revents_;      // poller返回的具体发生的事件
    int index_;

    // 用于进行回调监听，例如连接断开时回调被触发，但对象可能已经销毁，通过lock检测
    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道李米娜能够后置fd最终发生的具体的事件revents，素以他负责调用具体事件的回调操作
    ReadEventCallBack readCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack closeCallback_;
    EventCallBack errorCallback_;
};
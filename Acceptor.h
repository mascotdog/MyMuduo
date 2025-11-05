#pragma once
#include <functional>

#include "Channel.h"
#include "Socket.h"
#include "noncopyable.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
    using NewConnectionCallBack =
        std::function<void(int sockfd, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallBack &cb) {
        newConnectionCallBack_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();
    EventLoop *loop_; // Acceptor 用的就是用户定义的baseLoop，也称作mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallBack newConnectionCallBack_;
    bool listenning_;
};
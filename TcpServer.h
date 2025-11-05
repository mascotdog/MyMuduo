#pragma once

/**
 * 用户使用muduo库编写服务器程序
 */
#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

// 对外的服务器编程使用的类
class TcpServer : noncopyable {
public:
    using ThreadInitCallBack = std::function<void(EventLoop *)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr,
              const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallBack &cb) {
        threadInitCallBack_ = cb;
    }
    void setConnectionCallback(const ConnectionCallback &cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb) {
        messageCallback_ = cb;
    }
    void setWrtteCompleteCallback(const WriteCompleteCallback &cb) {
        writeCompleteCallback_ = cb;
    }

    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnetionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    EventLoop *loop_; // baseLoop 用户定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor>
        acceptor_; // 运行在mainloop，任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成后的回调

    ThreadInitCallBack threadInitCallBack_; // loop线程初始化的回调
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接
};
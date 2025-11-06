#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

// 网络库底层的缓冲区定义
class Buffer {
public:
    static const size_t kCheapPreend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPreend + initialSize), readIndex_(kCheapPreend),
          writeIndex_(kCheapPreend) {}

    size_t readableBytes() const { return writeIndex_ - readIndex_; }

    size_t writableBytes() const { return buffer_.size() - writeIndex_; }

    size_t prependableBytes() const { return readIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readIndex_; }

    // onMessage string <- Buffer
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readIndex_ += len; //  应用只读取了可读缓冲区的数据一部分
        } else {               // len = readableByte()
            retrieveAll();
        }
    }

    void retrieveAll() { readIndex_ = writeIndex_ = kCheapPreend; }

    // 把onMessage函数上报的Buffer数据，转成string类型数据返回
    std::string retrieveeAllAsString() {
        return retrieveAsString(readableBytes()); // 应用可读取的数据
    }

    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len);
        // 上面一句把缓冲区中可读的数据已经读取出俩，这里要对应对缓冲区进行复位操作
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len); // 扩容
        }
    }

    // 把[data,data + len] 内存上的数据，添加到缓冲区当中
    void append(const char *data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    char *beginWrite() { return begin() + writeIndex_; }

    const char *beginWrite() const { return begin() + writeIndex_; }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);

    // 通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);
private:
    char *begin() {
        // it.operator*().operator&()
        return &*buffer_.begin(); // 底层数组首元素的地址
    }

    const char *begin() const {
        // it.operator*().operator&()
        return &*buffer_.begin(); // 底层数组首元素的地址
    }

    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPreend) {
            buffer_.resize(writeIndex_ + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readIndex_, begin() + writeIndex_,
                      begin() + kCheapPreend);
            readIndex_ = kCheapPreend;
            writeIndex_ = readIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};
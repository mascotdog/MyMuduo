#include "InetAddress.h"

#include <string.h>
#include <strings.h>

InetAddress::InetAddress(uint16_t port, std::string ip) {
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const {
    char buf[64] = {0};
    // 将接口类型 AF（通常为 AF_INET 或 AF_INET6）的网络字节序二进制 IP
    // 地址（位于 CP
    // 指针指向的位置）转换为人类可阅读的呈现格式（字符串），并写入 BUF
    // 指向的缓冲区，缓冲区长度为 LEN。
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

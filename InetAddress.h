#pragma once

#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
class InetAddress
{
public:
    explicit InetAddress() {}
    explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr) : _addr(addr) {}

    std::string toIP() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    const sockaddr_in *getSockAddr() const { return &_addr; }
    void setSockAddr(const sockaddr_in &addr) { _addr = addr; }

private:
    sockaddr_in _addr;
};
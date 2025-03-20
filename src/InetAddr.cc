#include "InetAddr.h"
#include <string.h>

InetAddr::InetAddr(const sockaddr_in &addr) : _addr(addr) {
    memset(&_addr, 0, sizeof(_addr)); // initialize all fields in struct _addr to zero
    _addr = addr; // set the address to the provided address
}

InetAddr::InetAddr(const char *ip, uint16_t port) {
    memset(&_addr, 0, sizeof(_addr)); // initialize all fields in struct _addr to zero
    _addr.sin_family = AF_INET; // AF_INET is the address family for IPv4
    // set the port number to "port"
    // htons converts the port number from host byte order to network byte order
    // network byte order is big-endian, while host byte order can be either little-endian or big-endian
    // htons stands for "host to network short"
    _addr.sin_port = htons(port);
     // set the server address to ip, ip must be local address
    // inet_addr convert internet host address from numbers-and-dots notation into binary data in network byte order
    _addr.sin_addr.s_addr = inet_addr(ip);
}

InetAddr::InetAddr(uint16_t port) : InetAddr::InetAddr("127.0.0.1", port) {}

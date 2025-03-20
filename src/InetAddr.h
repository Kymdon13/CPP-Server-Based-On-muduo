#ifndef INETADDR_H_
#define INETADDR_H_

#include <arpa/inet.h>
#include <sys/socket.h>

class InetAddr {
public:
    struct sockaddr_in _addr; // address struct
    socklen_t _addr_len = sizeof(_addr); // address length

    /// @brief build with struct sockaddr_in
    /// @param addr address struct, should be memset with 0 first
    /// @details This constructor initializes the _addr member variable with the provided sockaddr_in structure
    InetAddr(const struct sockaddr_in &addr);

    /// @brief build with ip and port
    /// @param port port number
    /// @param ip ip address
    InetAddr(const char *ip = "127.0.0.1", uint16_t port = 80);

    /// @brief build with port only
    /// @param port port number
    InetAddr(uint16_t port);

    ~InetAddr() = default;
};

#endif // INETADDR_H_

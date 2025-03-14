#ifndef INETADDR_H_
#define INETADDR_H_

#include <arpa/inet.h>
#include <sys/socket.h>

class InetAddr {
public:
    struct sockaddr_in _addr; // address struct
    socklen_t _addr_len = sizeof(_addr); // address length

    /// @brief default ctor
    /// @param addr address struct, should be init with 0 first
    /// @details This constructor initializes the _addr member variable with the provided sockaddr_in structure
    InetAddr();

    /// @brief build with struct sockaddr_in
    /// @param addr address struct, should be memset with 0 first
    /// @details This constructor initializes the _addr member variable with the provided sockaddr_in structure
    InetAddr(const struct sockaddr_in& addr);

    /// @brief build with ip and port
    /// @param port port number
    /// @param ip ip address
    InetAddr(const char *ip, uint16_t port);

    ~InetAddr();
};

#endif // INETADDR_H_

#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/socket.h>

class InetAddr; // forward declaration

class Socket {
private:
    int _fd; // socket file descriptor
public:
    /// @brief default ctor
    /// @details constructs a new socket and assigns it to _fd
    Socket();

    /// @brief ctor with a given file descriptor
    /// @param fd assigns fd to _fd
    Socket(int fd);

    ~Socket();

    /// @brief bind the socket to a given InetAddr instance
    /// @param addr the InetAddr instance to bind to
    void bind(InetAddr* addr);

    /// @brief listen for incoming connections
    /// @param backlog the maximum length of the queue for pending connections
    void listen(int backlog = SOMAXCONN);

    /// @brief accept a connection from the pending queue
    /// @param addr the InetAddr instance to store the address of the accepted connection
    /// @return the file descriptor of the accepted connection
    int accept(InetAddr* addr);

    /// @brief connect to a given InetAddr instance
    /// @param addr the InetAddr instance to connect to
    void connect(InetAddr* addr);

    /// @brief close the socket
    void close();

    /// @brief set the socket to non-blocking mode
    void setNonBlocking();

    /// @brief getter of _fd
    /// @return _fd
    int getFd();
};

#endif // SOCKET_H_

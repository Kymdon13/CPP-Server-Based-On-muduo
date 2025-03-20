#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/socket.h>

class InetAddr; // forward declaration

class Socket {
private:
    int _fd; // socket file descriptor
    bool _isClosed = false;
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

    /// @brief async read,
    /// @param buf buffer used to store data
    /// @param n number of bytes you want to read
    /// @param done if there is no data left in socket, done = true
    /// @return actual number of bytes read into buf
    size_t read(void *buf, size_t n, bool &done);

    /// @brief sync read,
    /// @param buf buffer used to store data
    /// @param n number of bytes you want to read
    /// @return actual number of bytes read into buf
    size_t readSync(void *buf, size_t n);

    size_t writeSync(const void *buf, size_t nbytes);

    /// @brief set the socket to non-blocking mode
    void setNonBlocking();

    /// @brief allow the socket to bind to the same addr:port while the last one is in TIME_WAIT stat
    void setREUSEADDR();

    /// @brief getter of _fd
    /// @return _fd
    int getFd();

    bool getIsClosed();

    /// @brief close the socket
    void close();
};

#endif // SOCKET_H_

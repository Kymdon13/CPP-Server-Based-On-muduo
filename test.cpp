#include "Socket.h"

int main() {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET 或 AF_INET6 来指定版本
    hints.ai_socktype = SOCK_STREAM; //TCP

    if ((status = getaddrinfo("www.baidu.com", "80", &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return 1;
    }

    std::cout << "IP addresses for www.example.com:" << std::endl;

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        std::string ipver;

        // 获取指向地址本身的指针，不同的协议有不同的字段。
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // 转换IP地址为可读字符串
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        std::cout << " " << ipver << ": " << ipstr << std::endl;
    }

    // socket used to talk to server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // store server address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("223.109.82.212");
    serv_addr.sin_port = htons(80);
    errif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect failed");

    char req[1024] = "GET / HTTP/1.1\r\nContent-Type: \r\nHost: baidu.com\r\nConnection: close\r\n\r\n";
    ssize_t ret = write(sockfd, req, sizeof(req));

    /* communicate with client */
    char buffer[64] = {0}; // buffer to store received data
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // clear the buffer

        // receives data from the socket and stores it in the buffer
        // then handle the received data
        ssize_t read_number = read(sockfd, buffer, sizeof(buffer));
        if (read_number > 0) { // success
            std::cout << strlen(buffer) << " bytes received:" << buffer << std::endl;
        } else if (read_number == 0) {
            std::cout << "client has closed socket , exiting..." << std::endl;
            close(sockfd);
            break;
        } else if (read_number == -1) { // error
            close(sockfd);
            errif(true, "socket receive failed");
        }
    }

    freeaddrinfo(res); // 释放 addrinfo 链表
    return 0;
}

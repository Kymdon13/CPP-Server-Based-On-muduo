#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

int main() {
    // socket used to talk to server
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    // store server address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);
    errif(connect(serv_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "server socket connect failed");

    /* buffer */
    char buffer[64];
    while (true) {
        // clear the buffer
        memset(buffer, 0, sizeof(buffer));

        // get user input
        std::cout << "input: ";
        std::cin >> buffer;
        if (std::cin.eof()) {
            close(serv_sock);
            std::cout << "EOF detected, exiting..." << std::endl;
            break;
        }

        // write data to server
        ssize_t write_number = write(serv_sock, buffer, sizeof(buffer));
        if (write_number == -1) { // error
            close(serv_sock);
            errif(true, "socket send failed");
        } else if (write_number == 0) { // no data sent
            std::cout << "no data sent" << std::endl;
        } else {
            std::cout << write_number << " bytes sent to server" << std::endl;
        }

        // clear the buffer
        memset(buffer, 0, sizeof(buffer));

        // read and handle data from server
        ssize_t read_number = read(serv_sock, buffer, sizeof(buffer));
        if (read_number > 0) {
            std::cout << strlen(buffer) << " bytes received:" << buffer << std::endl;
        } else if (read_number == 0) {
            std::cout << "server has closed socket , exiting..." << std::endl;
            close(serv_sock);
            break;
        } else if (read_number == -1) { // error
            close(serv_sock);
            errif(true, "socket receive failed");
        }
    }
}

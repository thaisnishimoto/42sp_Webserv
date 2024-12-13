//
// Created by angomes- on 11/19/24.
//
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void sendRequest(const std::string& host, int port, const std::string& request) {
    int sock;
    struct sockaddr_in server;
    char buffer[4096];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return;
    }

    server.sin_addr.s_addr = inet_addr(host.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    // Send HTTP request
    if (send(sock, request.c_str(), request.size(), 0) < 0) {
        perror("Send failed");
        close(sock);
        return;
    }

    // Receive server response
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        std::cout << "Server Response:\n" << buffer << std::endl;
    } else {
        perror("Receive failed");
    }
    std::cout << "----------------------" << std::endl;

    close(sock);
}

int main() {
    std::string host = "127.0.0.1"; // Localhost
    int port = 8081; // Default HTTP port

    // Example 1: Valid request
    std::string Request =
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Type:\r\n"
        "Content-Length: 10\r\n"
        "Connection: close\r\n\r\n";

    std::cout << "----------------------" << std::endl;
    std::cout << Request << std::endl;
    sendRequest(host, port, Request);;

    return 0;
}

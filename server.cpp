// server.cpp

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
 
#define PORT 8080
 
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    const char* hello = "Hello from server";
 
    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // 设置套接字选项
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // 绑定地址
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // 接受客户端连接
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // 读取客户端数据
    while (true)
    {
        read(new_socket, buffer, 1024);
        printf("%d\n", buffer[0]);
        const char* t = "TODO 99:99:99";
        const char* n = "Anya";
        const char* l = "TODO CLIENT LIST";
        const char* sm = "TODO SEND MESSAGE";
        const char* bye = "OK, bye~";

        switch (buffer[0])
        {
        case 1:
            send(new_socket, t, strlen(t), 0);
            break;
        case 2:
            send(new_socket, n, strlen(n), 0);
            break;
        case 3:
            send(new_socket, l, strlen(l), 0);
            break;
        case 4:
            send(new_socket, sm, strlen(sm), 0);
            printf("Target: %s\n", buffer+1);
            printf("Message: %s\n", buffer+21);
            break;
        case 5:
            send(new_socket, bye, strlen(bye), 0);
            printf("%d disconnected\n",PORT);
            close(new_socket);
            return 0;
            break;
        default:
            break;
        }
    }
    // 关闭套接字
    close(server_fd);

    return 0;
}
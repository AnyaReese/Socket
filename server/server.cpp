// server.cpp

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <map>
using namespace std;

#define PORT 8080
#define MAX_CLIENT 10


// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"
#define CYAN "\033[36m"

// std::vector<thread> threads;
std::map<int, thread*> threads;
int sockets[MAX_CLIENT];


void do_server(int socket)
{
    char buffer[1024] = {0};
    // 读取客户端数据
    while (true)
    {
        read(socket, buffer, 1024);
        const char* t = "TODO 99:99:99";
        const char* n = "Tcat";
        const char* l = "TODO CLIENT LIST";
        const char* sm = "TODO SEND MESSAGE";
        const char* bye = "OK, bye~";
        int id;

        switch (buffer[0])
        {
        case 1:
            send(socket, t, strlen(t), 0);
            break;
        case 2:
            send(socket, n, strlen(n), 0);
            break;
        case 3:
            for ( id = 0; id<MAX_CLIENT; id++)
            {
                if(sockets[id] > 0)
                {
                    cout <<"\tID: "<<id<<" socket:"<<sockets[id]<<endl; 
                }
            }
            send(socket, l, strlen(l), 0);
            break;
        case 4:
            send(socket, sm, strlen(sm), 0);
            printf("Target: %s\n", buffer+1);
            printf("Message: %s\n", buffer+21);
            break;
        case 5:
            send(socket, bye, strlen(bye), 0);
            printf("%d disconnected\n",PORT);
            close(socket);
            for ( id = 0; id<MAX_CLIENT; id++)
            {
                if(sockets[id] == socket)break; 
            }
            sockets[id] = -2;
            return;
            break;
        default:
            break;
        }
    }
    return;
}

int main() {
    for(int i=0;i<MAX_CLIENT;i++)
        sockets[i] = -1;
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

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
    
    if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    

    // 监听连接
    if (listen(server_fd, MAX_CLIENT) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    cout<<"Listening at "<< PORT << " Max Clients: "<< MAX_CLIENT <<endl;
    while (true)
    {
    // 接受客户端连接
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        int id = 0;
        for ( id = 0; id<MAX_CLIENT; id++)
        {
            if(sockets[id]<0)break; 
        }
        
        sockets[id] = new_socket;
        if(sockets[id]==-2)
        {
            threads[id]->join();
            delete threads[id];
            cout<<"deleted"<<endl;
        }
        threads[id] = new thread(do_server, new_socket);
        cout<<"PORT: "<< PORT <<" connected to new_socket: "<< new_socket <<" with local ID: "<< id <<endl;
        // threads.emplace_back(do_server, new_socket);
        cout<<"Current connecting: "<<endl;
        for ( id = 0; id<MAX_CLIENT; id++)
        {
            cout <<"\tID: "<<id<<" socket:"<<sockets[id]<<endl; 
        }
    }

    
    // for (auto& th : threads) {
    //     th.join();
    // }
    // 关闭套接字
    close(server_fd);

    return 0;
}

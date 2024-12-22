#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <map>
#include <string>
#include <time.h>
#include <signal.h>
#include <atomic>

using namespace std;

#define PORT 8080
#define MAX_CLIENT 10

// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"
#define CYAN "\033[36m"

std::map<int, thread*> threads;
int sockets[MAX_CLIENT];
int server_fd;  // 全局server socket描述符

// 添加全局原子标志用于优雅退出
std::atomic<bool> should_exit(false);

// 信号处理函数
void signal_handler(int signum) {
    printf("\n%s[INFO]%s Caught signal %d, cleaning up...\n", GREEN, RESET, signum);
    should_exit = true;
    
    // 关闭所有客户端连接
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(sockets[i] >= 0) {
            close(sockets[i]);
            sockets[i] = -1;
        }
    }
    
    // 关闭服务器socket
    if(server_fd >= 0) {
        close(server_fd);
    }
}

void do_server(int socket, int id) {
    char buffer[1024] = {0};
    string ring_ = "You have a message from ID: "+to_string(id)+"\n";
    const char* n = "Tcat";
    const char* bye = "OK, bye~";

    // 发送欢迎消息
    const char* hello = "Welcome to the server!\n";
    send(socket, hello, strlen(hello), 0);

    // 读取客户端数据
    while (!should_exit) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(socket, buffer, 1024);
        
        if(bytes_read <= 0) {
            if(bytes_read == 0) {
                printf("Client ID %d disconnected\n", id);
            } else {
                printf("Read error for client ID %d\n", id);
            }
            break;
        }

        time_t rawtime;
        time(&rawtime);
        const char* t = asctime(localtime(&rawtime));
        int target_ = -1;
        string client_list_ = "Connecting Client List:\n";

        switch (buffer[0]) {
            case 1: // get time
                send(socket, t, strlen(t), 0);
                break;
            case 2: // get name
                send(socket, n, strlen(n), 0);
                break;
            case 3: // client list
                for(int i = 0; i < MAX_CLIENT; i++) {
                    if(sockets[i] >= 0)
                        client_list_ += "\tID: " + to_string(i) + " socket:" + to_string(sockets[i]) + "\n";
                }
                send(socket, client_list_.c_str(), client_list_.length(), 0);
                break;
            case 4: // send message
                target_ = int(*(buffer+1)) - int('0');
                if(target_ < 0 || target_ >= MAX_CLIENT || sockets[target_] < 0) {
                    send(socket, "[ERROR]: Target ID does not exist", 32, 0);
                    break;
                }
                printf("Sending Message from ID: %d to target ID: %s\n", id, buffer+1);
                send(sockets[target_], ring_.c_str(), ring_.length(), 0);
                send(sockets[target_], buffer+21, 256, 0);
                printf("Message: %s\n", buffer+21);
                break;
            case 5: // disconnect
                send(socket, bye, strlen(bye), 0);
                close(socket);
                sockets[id] = -2;
                printf("ID %d disconnected\n", id);
                return;
            default:
                break;
        }
    }

    // 清理连接
    close(socket);
    sockets[id] = -2;
    return;
}

int main() {
    // 初始化socket数组
    for(int i = 0; i < MAX_CLIENT; i++)
        sockets[i] = -1;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
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
    
    cout << "Listening at " << PORT << " Max Clients: " << MAX_CLIENT << endl;
    
    while (!should_exit) {
        // 使用带超时的accept
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        tv.tv_sec = 1;  // 1秒超时
        tv.tv_usec = 0;

        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            int new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }

            int id = 0;
            for(id = 0; id < MAX_CLIENT; id++) {
                if(sockets[id] < 0) break;
            }

            if(id >= MAX_CLIENT) {
                printf("%s[ERROR]%s Maximum clients reached\n", RED, RESET);
                close(new_socket);
                continue;
            }

            sockets[id] = new_socket;
            if(sockets[id] == -2) {
                threads[id]->join();
                delete threads[id];
                cout << "Cleaned up thread for ID " << id << endl;
            }
            
            threads[id] = new thread(do_server, new_socket, id);
            cout << "PORT: " << PORT << " connected to new_socket: " << new_socket << " with local ID: " << id << endl;
        }
    }

    // 等待所有子线程结束
    printf("%s[INFO]%s Waiting for all threads to finish...\n", GREEN, RESET);
    for(auto& pair : threads) {
        if(pair.second && pair.second->joinable()) {
            pair.second->join();
            delete pair.second;
        }
    }
    threads.clear();

    // 关闭服务器socket
    close(server_fd);
    printf("%s[INFO]%s Server shutdown complete\n", GREEN, RESET);

    return 0;
}
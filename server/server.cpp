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
int server_fd;

std::atomic<bool> should_exit(false);

// 添加一个辅助函数来处理send操作的结果
bool send_with_check(int socket, const char* message, size_t length, int source_id = -1, int target_id = -1) {
    ssize_t bytes_sent = send(socket, message, length, 0);
    if (bytes_sent < 0) {
        if (target_id >= 0) {
            printf("%s[ERROR]%s Failed to send message from ID %d to ID %d: %s\n", 
                   RED, RESET, source_id, target_id, strerror(errno));
            // 通知发送方消息发送失败
            string error_msg = "Message delivery failed: Connection error";
            send(sockets[source_id], error_msg.c_str(), error_msg.length(), 0);
        } else {
            printf("%s[ERROR]%s Failed to send message to socket %d: %s\n", 
                   RED, RESET, socket, strerror(errno));
        }
        return false;
    } else if (bytes_sent < (ssize_t)length) {
        printf("%s[WARNING]%s Partial send: only %zd of %zu bytes sent\n", 
               YELLOW, RESET, bytes_sent, length);
        return false;
    }
    return true;
}

void signal_handler(int signum) {
    printf("\n%s[INFO]%s Caught signal %d, cleaning up...\n", GREEN, RESET, signum);
    should_exit = true;
    
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(sockets[i] >= 0) {
            const char* goodbye = "Server shutting down...";
            send_with_check(sockets[i], goodbye, strlen(goodbye));
            close(sockets[i]);
            sockets[i] = -1;
        }
    }
    
    if(server_fd >= 0) {
        close(server_fd);
    }
}

void do_server(int socket, int id) {
    char buffer[1024] = {0};
    const char* n = "Tcat";
    const char* bye = "OK, bye~";

    // 发送欢迎消息
    const char* hello = "Welcome to the server!\n";
    if (!send_with_check(socket, hello, strlen(hello))) {
        printf("%s[ERROR]%s Failed to send welcome message to client ID %d\n", RED, RESET, id);
        return;
    }

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

        // 在switch之前声明所有需要的变量
        string ring_;
        string success_msg;
        const char* error_msg;

        switch (buffer[0]) {
            case 1: // get time
                if (send_with_check(socket, t, strlen(t))) {
                    printf("%s[INFO]%s Time sent successfully to client ID %d\n", GREEN, RESET, id);
                }
                break;
                
            case 2: // get name
                if (send_with_check(socket, n, strlen(n))) {
                    printf("%s[INFO]%s Name sent successfully to client ID %d\n", GREEN, RESET, id);
                }
                break;
                
            case 3: // client list
                for(int i = 0; i < MAX_CLIENT; i++) {
                    if(sockets[i] >= 0)
                        client_list_ += "\tID: " + to_string(i) + " socket:" + to_string(sockets[i]) + "\n";
                }
                if (send_with_check(socket, client_list_.c_str(), client_list_.length())) {
                    printf("%s[INFO]%s Client list sent successfully to client ID %d\n", GREEN, RESET, id);
                }
                break;
                
            case 4: // send message
                target_ = int(*(buffer+1)) - int('0');
                if(target_ < 0 || target_ >= MAX_CLIENT || sockets[target_] < 0) {
                    error_msg = "[ERROR]: Target ID does not exist";
                    send_with_check(socket, error_msg, strlen(error_msg));
                    break;
                }
                
                ring_ = "You have a message from ID: " + to_string(id) + "\n";
                printf("Attempting to send message from ID %d to target ID %d\n", id, target_);
                
                // 首先发送通知消息
                if (send_with_check(sockets[target_], ring_.c_str(), ring_.length(), id, target_)) {
                    // 然后发送实际消息内容
                    if (send_with_check(sockets[target_], buffer+21, strlen(buffer+21), id, target_)) {
                        printf("%s[INFO]%s Message successfully delivered from ID %d to ID %d\n", 
                               GREEN, RESET, id, target_);
                        // 通知发送方消息发送成功
                        success_msg = "Message successfully delivered to ID " + to_string(target_);
                        send_with_check(socket, success_msg.c_str(), success_msg.length());
                    }
                }
                break;
                
            case 5: // disconnect
                if (send_with_check(socket, bye, strlen(bye))) {
                    printf("%s[INFO]%s Goodbye message sent successfully to client ID %d\n", GREEN, RESET, id);
                }
                close(socket);
                sockets[id] = -2;
                printf("ID %d disconnected\n", id);
                return;
                
            default:
                break;
        }
    }

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

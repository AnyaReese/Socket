#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <queue>
#include <mutex>

#define DEFUALT_PORT 8080

// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"
#define CYAN "\033[36m"

using namespace std;

string menu1 = "+--------------------+\n"
               "|    " GREEN "Menu Options" RESET "    |\n"
               "+--------------------+\n"
               "| " YELLOW "1." RESET " connect         |\n"
               "| " YELLOW "2." RESET " exit            |\n"
               "+--------------------+\n";

string menu2 = "+-------------------+\n"
               "|    " GREEN "Menu Options" RESET "   |\n"
               "+-------------------+\n"
               "| " YELLOW "1." RESET " get time       |\n"
               "| " YELLOW "2." RESET " get name       |\n"
               "| " YELLOW "3." RESET " client list    |\n"
               "| " YELLOW "4." RESET " send message   |\n"
               "| " YELLOW "5." RESET " disconnect     |\n"
               "| " YELLOW "6." RESET " exit           |\n"
               "| " YELLOW "7." RESET " refresh        |\n"
               "+-------------------+\n";

struct packet {
    char request; //1.get time 2.get name 3.client list 4.send message 5.disconnect
    char target_addr[20];
    char message[256];
};

// 定义消息队列和同步机制
struct ThreadMessage {
    enum Type { RESPONSE, NOTIFICATION } type;
    string content;
    string sender_ip;
    int sender_id;
};

queue<ThreadMessage> messageQueue;
mutex queueMutex;
bool threadRunning = false;
bool clientListReceived = false;
pthread_t recvThread;
int global_sock;

// 处理消息队列
void processMessageQueue() {
    queueMutex.lock();
    while(!messageQueue.empty()) {
        ThreadMessage msg = messageQueue.front();
        messageQueue.pop();
        
        if(msg.type == ThreadMessage::RESPONSE) {
            printf("%s[Server Response]:%s %s\n", CYAN, RESET, msg.content.c_str());
        } else {
            printf("\n%s[Message from %s(ID:%d)]:%s %s\n", 
                   CYAN, msg.sender_ip.c_str(), msg.sender_id, RESET, msg.content.c_str());
            printf("%s[User]%s ", YELLOW, RESET);
            fflush(stdout);
        }
    }
    queueMutex.unlock();
}

// 接收数据的线程函数
void* receiveThread(void* arg) {
    int sock = *(int*)arg;
    char buffer[1024];
    threadRunning = true;
    
    while(threadRunning) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer)-1, 0);
        if(bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            
            ThreadMessage msg;
            // 简单解析消息类型和内容
            if(strstr(buffer, "[NOTIFICATION]") != NULL) {
                msg.type = ThreadMessage::NOTIFICATION;
                // 解析发送者信息
                sscanf(buffer, "[NOTIFICATION]From:%[^(](ID:%d):%s", 
                       msg.sender_ip.c_str(), &msg.sender_id, msg.content.c_str());
            } else {
                msg.type = ThreadMessage::RESPONSE;
                msg.content = string(buffer);
            }
            
            queueMutex.lock();
            messageQueue.push(msg);
            queueMutex.unlock();
        }
        processMessageQueue();
    }
    return NULL;
}

// 处理断开连接
void disconnect(int& sock) {
    printf("%s[INFO]%s Disconnecting...\n", GREEN, RESET);
    threadRunning = false;
    shutdown(sock, SHUT_RDWR);
    pthread_join(recvThread, NULL);
    close(sock);
    clientListReceived = false;
    printf("%s[INFO]%s Disconnected successfully!\n", GREEN, RESET);
}

// 发送消息到服务器
void send_to_server(int &sock, struct packet pack) {
    send(sock, &pack, 277, 0);
}

// 获取客户端列表
void get_client_list(int& sock) {
    struct packet pack = {3, "", ""};
    send_to_server(sock, pack);
    clientListReceived = true;
}

// 发送消息
void send_message(int& sock) {
    if(!clientListReceived) {
        printf("%s[INFO]%s Getting client list first...\n", GREEN, RESET);
        get_client_list(sock);
        sleep(1); // 等待接收客户端列表
    }
    
    struct packet pack = {4, "", ""};
    printf("%s[INFO]%s Please enter target IP address:\n", GREEN, RESET);
    printf("%s[User]%s ", YELLOW, RESET);
    cin >> pack.target_addr;
    printf("%s[INFO]%s Please enter your message:\n", GREEN, RESET);
    printf("%s[User]%s ", YELLOW, RESET);
    cin.ignore();
    cin.getline(pack.message, 256);
    
    send_to_server(sock, pack);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_sockaddr;
    bool connected = false;
    
    while (true) {
        int order = 0;
        if(connected) {
            cout << menu2;
            printf("%s[User]%s ", YELLOW, RESET);
            cin >> order;
            
            struct packet pack = {-1,"",""};
            pack.request = order;
            bool flag = true;
            
            switch (order) {
                case 1: // get time
                    printf("%s[INFO]%s Getting time...\n", GREEN, RESET);
                    break;
                    
                case 2: // get name
                    printf("%s[INFO]%s Getting name...\n", GREEN, RESET);
                    break;
                    
                case 3: // client list
                    printf("%s[INFO]%s Getting client list...\n", GREEN, RESET);
                    clientListReceived = true;
                    break;
                    
                case 4: // send message
                    flag = false;
                    send_message(sock);
                    break;
                    
                case 5: // disconnect
                    flag = false;
                    disconnect(sock);
                    connected = false;
                    break;
                    
                case 6: // exit
                    flag = false;
                    if(connected) {
                        disconnect(sock);
                    }
                    printf("%s[INFO]%s Goodbye!\n", GREEN, RESET);
                    return 0;
                    
                case 7: // refresh
                    printf("%s[INFO]%s Refreshing...\n", GREEN, RESET);
                    break;
                    
                default:
                    printf("%s[ERROR]%s Unknown command!\n", RED, RESET);
                    flag = false;
                    break;
            }
            if(flag) send_to_server(sock, pack);
            processMessageQueue();
        }
        else {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                printf("%s[ERROR]%s Socket creation failed\n", RED, RESET);
                return -1;
            }
            
            global_sock = sock;
            memset(&serv_sockaddr, 0, sizeof(serv_sockaddr));
            serv_sockaddr.sin_family = AF_INET;
            serv_sockaddr.sin_port = htons(DEFUALT_PORT);
            
            cout << menu1;
            printf("%s[User]%s ", YELLOW, RESET);
            cin >> order;
            
            switch (order) {
                case 1: {
                    char ip[20];
                    int port;
                    printf("%s[INFO]%s Please enter server IP address:\n", GREEN, RESET);
                    printf("%s[User]%s ", YELLOW, RESET);
                    cin >> ip;
                    printf("%s[INFO]%s Please enter server port:\n", GREEN, RESET);
                    printf("%s[User]%s ", YELLOW, RESET);
                    cin >> port;
                    
                    serv_sockaddr.sin_port = htons(port);
                    
                    if (inet_pton(AF_INET, ip, &serv_sockaddr.sin_addr) <= 0) {
                        printf("%s[ERROR]%s Invalid address/Address not supported\n", RED, RESET);
                        break;
                    }
                    
                    if (connect(sock, (struct sockaddr*)&serv_sockaddr, sizeof(serv_sockaddr)) < 0) {
                        printf("%s[ERROR]%s Connection failed\n", RED, RESET);
                        break;
                    }
                    
                    printf("%s[INFO]%s Connected successfully!\n", GREEN, RESET);
                    connected = true;
                    
                    // 创建接收线程
                    pthread_create(&recvThread, NULL, receiveThread, (void*)&global_sock);
                    break;
                }
                
                case 2:
                    printf("%s[INFO]%s Exiting...\n", GREEN, RESET);
                    close(sock);
                    printf("%s[INFO]%s Goodbye!\n", GREEN, RESET);
                    return 0;
                
                default:
                    printf("%s[ERROR]%s Unknown command!\n", RED, RESET);
                    break;
            }
        }
    }
    
    close(sock);
    return 0;
}
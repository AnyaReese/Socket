#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>

#define DEFUALT_PORT 3784

// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"
#define CYAN "\033[36m"

using namespace std;

// 添加响应计数器
atomic<int> responseCounter(0);

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
               "| " YELLOW "8." RESET " time test      |\n"  // 新增选项
               "+-------------------+\n";

struct packet {
    unsigned char request; //1.get time 2.get name 3.client list 4.send message 5.disconnect
    unsigned char target_addr;
    unsigned char size;
    char message[256];
};

// 定义消息队列和同步机制
struct ThreadMessage {
    enum Type { RESPONSE, INFO } type;
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
bool server_shutdown = false;

// 发送消息到服务器
void send_to_server(int &sock, struct packet pack) {
    send(sock, &pack, sizeof(pack), 0);
}

// 新增的时间请求测试函数
void perform_time_test(int& sock) {
    printf("%s[INFO]%s 开始自动化时间请求测试 (100次请求)...\n", GREEN, RESET);
    
    responseCounter = 0;
    struct packet pack = {1, 0, 0, ""};  // 请求类型1是获取时间
    
    auto start_time = chrono::high_resolution_clock::now();
    
    // 发送100次请求
    for(int i = 0; i < 100; i++) {
        send(sock, &pack, sizeof(pack), 0);
        usleep(10000);  // 请求间隔10ms
    }
    
    // 等待响应（带超时）
    int timeout_seconds = 10;
    auto start_wait = chrono::high_resolution_clock::now();
    while(responseCounter < 100) {
        auto current = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(current - start_wait).count();
        if(duration >= timeout_seconds) {
            printf("%s[ERROR]%s 等待响应超时！只收到 %d/100 个响应\n", 
                   RED, RESET, responseCounter.load());
            break;
        }
        usleep(100000);  // 等待时休眠100ms
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
    
    printf("%s[INFO]%s 测试完成:\n", GREEN, RESET);
    printf("- 发送请求数: 100\n");
    printf("- 接收响应数: %d\n", responseCounter.load());
    printf("- 总耗时: %ld ms\n", duration);
    printf("- 平均响应时间: %.2f ms\n", duration/100.0);
}

// 处理断开连接
void disconnect(int& sock) {
    printf("%s[INFO]%s Disconnecting...\n", GREEN, RESET);
    struct packet pack = {5, 0, 0, ""};
    send_to_server(sock, pack);
    threadRunning = false;
    shutdown(sock, SHUT_RDWR);
    pthread_join(recvThread, NULL);
    close(sock);
    clientListReceived = false;
    printf("%s[INFO]%s Disconnected successfully!\n", GREEN, RESET);
}

// 处理消息队列
void processMessageQueue() {
    queueMutex.lock();
    while(!messageQueue.empty()) {
        ThreadMessage msg = messageQueue.front();
        messageQueue.pop();
        
        if(msg.type == ThreadMessage::RESPONSE) {
            printf("\n%s[Server Response]:%s %s\n", CYAN, RESET, msg.content.c_str());
            printf("%s[User]%s ", YELLOW, RESET);
            if(msg.sender_id == -1)
            {
                server_shutdown = true;
                disconnect(global_sock);
            }
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
            if(strstr(buffer, "[INFO]") != NULL) {
                msg.type = ThreadMessage::INFO;
                sscanf(buffer, "[INFO]From:%[^(](ID:%d):%s", 
                       msg.sender_ip.c_str(), &msg.sender_id, msg.content.c_str());
            } else {
                if(strstr(buffer, "[SHUTDOWN]") != NULL) {
                    msg.sender_id = -1;
                }
                
                msg.type = ThreadMessage::RESPONSE;
                msg.content = string(buffer);
                
                // 如果是时间响应则增加计数
                if(msg.type == ThreadMessage::RESPONSE && 
                    (strstr(buffer, "202") != NULL || // 匹配年份
                        strstr(buffer, "Mon ") != NULL || // 匹配星期
                        strstr(buffer, "Tue ") != NULL ||
                        strstr(buffer, "Wed ") != NULL ||
                        strstr(buffer, "Thu ") != NULL ||
                        strstr(buffer, "Fri ") != NULL ||
                        strstr(buffer, "Sat ") != NULL ||
                        strstr(buffer, "Sun ") != NULL)) {
                        responseCounter++;
}
            }
            
            queueMutex.lock();
            messageQueue.push(msg);
            queueMutex.unlock();
        }
        processMessageQueue();
    }
    return NULL;
}

// 获取客户端列表
void get_client_list(int& sock) {
    struct packet pack = {3, 0, 0, ""};
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
    
    struct packet pack = {4, 0, 0, ""};
    printf("%s[INFO]%s Please enter target ID\n", GREEN, RESET);
    printf("%s[User]%s ", YELLOW, RESET);
    cin >> pack.target_addr;
    printf("%s[INFO]%s Please enter your message:\n", GREEN, RESET);
    printf("%s[User]%s ", YELLOW, RESET);
    cin.ignore();
    cin.getline(pack.message, 256);
    pack.size = strlen(pack.message);
    send_to_server(sock, pack);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_sockaddr;
    bool connected = false;
    
    while (true) {
        int order = 0;
        if(server_shutdown)
        {
            connected = false;
            server_shutdown = false;
        }

        if(connected) {
            cout << menu2;
            printf("%s[User]%s ", YELLOW, RESET);
            cin >> order;
            
            struct packet pack = {0,0,0,""};
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
                    
                case 8: // time test
                    flag = false;
                    perform_time_test(sock);
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
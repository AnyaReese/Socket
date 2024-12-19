// client.cpp
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define DEFUALT_PORT 8080

// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"
#define CYAN "\033[36m"

using namespace std;

string menu1 = "+-------------------+\n"
               "|    " GREEN "Menu Options" RESET "    |\n"
               "+-------------------+\n"
               "| " YELLOW "1." RESET " connect        |\n"
               "| " YELLOW "2." RESET " exit           |\n"
               "+-------------------+\n";

string menu2 = "+-------------------+\n"
               "|    " GREEN "Menu Options" RESET "    |\n"
               "+-------------------+\n"
               "| " YELLOW "1." RESET " get time       |\n"
               "| " YELLOW "2." RESET " get name       |\n"
               "| " YELLOW "3." RESET " client list    |\n"
               "| " YELLOW "4." RESET " send message   |\n"
               "| " YELLOW "5." RESET " disconnect     |\n"
               "| " YELLOW "6." RESET " exit           |\n"
               "+-------------------+\n";

struct packet
{
    char request; //1.get time 2.get name 3.client list 4.send message 5.disconnect
    char target_addr[20];
    char message[256];
};

void send_to_server(int &sock, struct packet pack)
{
    char buffer[1024] = {0};
    send(sock, &pack, 277, 0);
    read(sock, buffer, 1024);
    printf("%s[Server]:%s %s\n", CYAN, RESET, buffer); 
    return;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_sockaddr;
    // 创建套接字
    bool connected = false;
    
    while (true)
    {
        int order = 0;
        if(connected)
        {
            cout << menu2;
            printf("%s[User]%s ", YELLOW, RESET);
            cin >> order;
            
            struct packet pack = {-1,"",""};
            pack.request = order;
            bool flag = true;
            
            switch (order)
            {
            case 1:
                printf("%s[INFO]%s Getting time...\n", GREEN, RESET);
                break;
            case 2:
                printf("%s[INFO]%s Getting name...\n", GREEN, RESET);
                break;
            case 3:
                printf("%s[INFO]%s Getting client list...\n", GREEN, RESET);
                break;
            case 4:
                printf("%s[INFO]%s Please enter target IP address:\n", GREEN, RESET);
                printf("%s[User]%s ", YELLOW, RESET);
                cin >> pack.target_addr;
                printf("%s[INFO]%s Please enter your message:\n", GREEN, RESET);
                printf("%s[User]%s ", YELLOW, RESET);
                cin >> pack.message;
                printf("%s[INFO]%s Sending message...\n", GREEN, RESET);
                break;
            case 5:
                printf("%s[INFO]%s Disconnecting...\n", GREEN, RESET);
                flag = false;
                send_to_server(sock,pack);
                close(sock);
                connected = false;
                printf("%s[INFO]%s Disconnected successfully!\n", GREEN, RESET);
                break;
            case 6:
                printf("%s[INFO]%s Exiting...\n", GREEN, RESET);
                flag = false;
                send_to_server(sock,pack);
                close(sock);
                connected = false;
                printf("%s[INFO]%s Goodbye!\n", GREEN, RESET);
                return 0;
            default:
                printf("%s[ERROR]%s Unknown command!\n", RED, RESET);
                flag = false;
                break;
            }
            if(flag) send_to_server(sock,pack);
        }
        else
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                printf("%s[ERROR]%s Socket creation failed\n", RED, RESET);
                return -1;
            }
            
            memset(&serv_sockaddr, 0, sizeof(serv_sockaddr));
            serv_sockaddr.sin_family = AF_INET;
            serv_sockaddr.sin_port = htons(DEFUALT_PORT);
            
            cout << menu1;
            printf("%s[User]%s ", YELLOW, RESET);
            cin >> order;
            
            switch (order)
            {
            case 1:
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
                break;

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
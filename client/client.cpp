// client.cpp

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define DEFUALT_PORT 8080

using namespace std;
string menu1 = "=======Menu=======\n| 1. connect     |\n| 2. exit        |\n==================\n";
string menu2 = "=======Menu=======\n| 1.get time     |\n| 2.get name     |\n| 3.client list  |\n| 4.send message |\n| 5.disconnect   |\n| 6.exit         |\n==================\n";

struct packet
{
    char request; //1.get time 2.get name 3.client list 4.send message 5.disconnect
    char target_addr[20];
    char message[256];
};

void send_to_server(int &sock,struct packet pack)
{
    char buffer[1024] = {0};
    send(sock, &pack, 277, 0);
    printf("message sent\n");
    read(sock, buffer, 1024);
    printf("[From Server]: %s\n", buffer); 
    return;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_sockaddr;
    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    memset(&serv_sockaddr, 0, sizeof(serv_sockaddr));
    serv_sockaddr.sin_family = AF_INET;
    serv_sockaddr.sin_port = htons(DEFUALT_PORT);
    bool connected = false;
    while (true)
    {
        int order = 0;
        if(connected)
        {
            cout<<menu2;
            cin>>order;
            struct packet pack = {-1,"",""};
            pack.request = order;
            bool flag = true;
            switch (order)
            {
            case 1:
                cout <<"Geting time ..."<<endl;
                break;
            case 2:
                cout <<"Geting name ..."<<endl;
                break;
            case 3:
                cout <<"Geting client list ..."<<endl;
                break;
            case 4:
                cout<<"input IP addr"<<endl;
                cin>>pack.target_addr;
                cout<<"input message"<<endl;
                cin>>pack.message;
                cout <<"Sending Messsage ..."<<endl;
                break;
            case 5:
                cout <<"Disconnecting ..."<<endl;
                flag = false;
                send_to_server(sock,pack);
                close(sock);
                cout <<"done"<<endl;
                break;
            case 6:
                cout <<"Exiting ..."<<endl;
                close(sock);
                cout <<"done"<<endl;
                return 0;
                break;
            default:
                cout <<"[INFO]: unknown command"<<endl;
                flag = false;
                break;
            }
            if(flag)send_to_server(sock,pack);
        }
        else
        {
            cout<<menu1;
            cin>>order;
            switch (order)
            {
            case 1:
                char ip[20];
                int port;
                cout<<"input IP addr"<<endl;
                cin>>ip;
                cout<<"input Port"<<endl;
                cin>>port;
                serv_sockaddr.sin_port = htons(port);
                // 将IP地址转换成二进制形式
                if (inet_pton(AF_INET,ip, &serv_sockaddr.sin_addr) <= 0) {
                    perror("Invalid address/ Address not supported");
                    break;
                }
                // 连接服务器
                if (connect(sock, (struct sockaddr*)&serv_sockaddr, sizeof(serv_sockaddr)) < 0) {
                    perror("Connection Failed");
                    break;;
                }
                connected = true;
                break;

            case 2:
                cout <<"Exiting ..."<<endl;
                close(sock);
                cout <<"done"<<endl;
                return 0;
                break;
            
            default:
                cout <<"[INFO]: unknown command"<<endl;
                break;
            }
        }
    }
    
    close(sock);
    return 0;
}
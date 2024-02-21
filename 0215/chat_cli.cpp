#include "lib.h"

#define DEFAULT_BUFLEN 1024

void RecvThread(SOCKET sock){
    char recvBuf[DEFAULT_BUFLEN + 1];
    while(true) {
        int recvlen = recv(sock, recvBuf, DEFAULT_BUFLEN, 0);
        if (recvlen > 0){
            recvBuf[recvlen] = '\0';
            cout << "Received: " << recvBuf << endl;

            send(sock, recvBuf, recvlen, 0);
        }
        else if (recvlen == 0){
            cout << "Server disconnected" <<endl;
            break;
        }
        else{
            int error = WSAGetLastError();
            if(error != WSAEWOULDBLOCK && error != WSAECONNRESET){
                cout << "recv() error" << endl;
                break;
            }
        }
        std::cout << "클라이언트: " << recvBuf << std::endl;
    }
}

void SendThread(SOCKET sock){
    char sendBuf[DEFAULT_BUFLEN];
    char recipient[100];

    cout << "대화 상대를 입력하세요: ";
    cin.getline(recipient, 100);

    std::cout << "메시지를 입력하세요: ";
    std::string message;
    std::getline(std::cin, message);
    while(true) {
        cin.getline(sendBuf, DEFAULT_BUFLEN);
        send(sock, recipient, strlen(recipient), 0);
        send(sock, sendBuf, strlen(sendBuf), 0);

        int sendlen = send(sock, sendBuf, strlen(sendBuf) + 1, 0);
        if (sendlen == SOCKET_ERROR){
            int error = WSAGetLastError();
            if(error != WSAEWOULDBLOCK){
                cout << "send() error" << endl;
                break;
            }
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
        return 1;
    }

    SOCKADDR_IN servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("100.106.146.50"); 
    servaddr.sin_port = htons(12345); 

    if (connect(sock, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR){
        cout << "connect() error" << endl;
        return 1;
    }

    thread recvThread(RecvThread, sock);

    thread sendThread(SendThread, sock);

    recvThread.join();
    sendThread.join();

    closesocket(sock);
    WSACleanup();
    return 0;
}
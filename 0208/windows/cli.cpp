#include "lib.h"

int main() {
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clisock = socket(AF_INET, SOCK_STREAM, 0);
    if (clisock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
        return 0;
    }

    char ip[16] = "";
    cout << "Input server IP: "; cin >> ip;

    SOCKADDR_IN servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ip);
    servAddr.sin_port = htons(12345);

    if (connect(clisock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cout << "connect() error" << endl;
        closesocket(clisock);
        WSACleanup();
        return 0;
    }
    
    // 경로 요청
    string http_request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    const char* request_buffer = http_request.c_str();
    int request_length = http_request.size();
    int sendlen = send(clisock, request_buffer, request_length, 0);
    if (sendlen == SOCKET_ERROR) {
        cout << "send() error" << endl;
        closesocket(clisock);
        WSACleanup();
        return 0;
    }

    // 서버로부터 응답 받기
    char buf[1024];
    string html_content;
    int recvlen;
    do {
        recvlen = recv(clisock, buf, sizeof(buf), 0);
        if (recvlen > 0) {
            html_content.append(buf, recvlen);
        } else if (recvlen == 0) {
            cout << "Connection closed by server" << endl;
        } else {
            int error_code = WSAGetLastError();
            if (error_code != WSAEWOULDBLOCK) {
                cout << "recv() error: " << error_code << endl;
            }
        }
    } while (recvlen > 0);

    cout << "Received HTML:" << endl;
    cout << html_content << endl;

    closesocket(clisock);
    WSACleanup();
    return 0;
}
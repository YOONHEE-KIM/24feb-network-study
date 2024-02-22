#include "lib.h"

#define DEFAULT_BUFLEN 1024

void RecvThread(SOCKET sock){
    char recvBuf[DEFAULT_BUFLEN];
    WSABUF wsabuf_R = {};
    DWORD flags = 0;
    OVERLAPPED overlapped = {};
    overlapped.hEvent = WSACreateEvent();

    while(true) {
        int recvlen = WSARecv(sock, &wsabuf_R, 1, NULL, &flags, &overlapped, NULL);
        if (recvlen == SOCKET_ERROR){
            int error = WSAGetLastError();
            if (error != WSA_IO_PENDING) {
                cout << "WSARecv() error" << endl;
                break;
            }
            // 비동기 작업 완료 대기
            DWORD dwRet = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, WSA_INFINITE, TRUE);
            if (dwRet == WSA_WAIT_FAILED) {
                cout << "WSAWaitForMultipleEvents() error" << endl;
                break;
            }
            // 이벤트 신호 확인
            WSAResetEvent(overlapped.hEvent);
        }
        else {
            // 비동기 수신 완료
            recvBuf[recvlen] = '\0';
            cout << "Received: " << recvBuf << endl;
        }
    }
    WSACloseEvent(overlapped.hEvent);
}

void SendThread(SOCKET sock){
    char sendBuf[DEFAULT_BUFLEN];
    while(true) {
        cout << "message: ";
        cin.getline(sendBuf, DEFAULT_BUFLEN);
        WSABUF wsabuf_S = {};
        wsabuf_S.buf = sendBuf;
        wsabuf_S.len = strlen(sendBuf);

        OVERLAPPED overlapped = {};
        overlapped.hEvent = WSACreateEvent();

        int sendlen = WSASend(sock, &wsabuf_S, 1, NULL, 0, &overlapped, NULL);
        if (sendlen == SOCKET_ERROR){
            int error = WSAGetLastError();
            if (error != WSA_IO_PENDING) {
                cout << "WSASend() error" << endl;
                break;
            }
            // 비동기 작업 완료 대기
            DWORD dwRet = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, WSA_INFINITE, TRUE);
            if (dwRet == WSA_WAIT_FAILED) {
                cout << "WSAWaitForMultipleEvents() error" << endl;
                break;
            }
            // 이벤트 신호 확인
            WSAResetEvent(overlapped.hEvent);
        }
        else {
            // 비동기 송신 완료
        }

        WSACloseEvent(overlapped.hEvent);
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
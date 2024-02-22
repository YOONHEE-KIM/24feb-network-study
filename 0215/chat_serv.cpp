#include "lib.h"

#define DEFAULT_BUFLEN 1024
#define MAX_THREAD 8

struct Session {
    SOCKET sock = INVALID_SOCKET;
    char recvBuf[DEFAULT_BUFLEN] = {};
    char sendBuf[DEFAULT_BUFLEN] = {};
    int bytesToSend = 0; //보낼 바이트 수
    int bytesSent = 0; //보낸 바이트 수
    WSAOVERLAPPED readOverLapped = {};
    WSAOVERLAPPED writeOverLapped = {};

    Session() {}
    Session(SOCKET sock) : sock(sock) {}
};

std::vector<Session*> sessions;
std::mutex sessionsMutex;

atomic<bool> TPoolRunning = true;

void SendDataToClient(Session* session);
void WorkerThread(HANDLE iocpHd);

MemoryPool* MemPool = new MemoryPool(sizeof(Session), 1000);

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET servsock = WSASocket(
        AF_INET, SOCK_STREAM, 0,
        NULL, 0, WSA_FLAG_OVERLAPPED
    );
    if (servsock == INVALID_SOCKET) {
        cout << "WSASocket() error" << endl;
        return 1;
    }

    u_long on = 1;
    if (ioctlsocket(servsock, FIONBIO, &on) == SOCKET_ERROR) {
        cout << "ioctlsocket() error" << endl;
        return 1;
    }

    SOCKADDR_IN servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(12345);

    if (bind(servsock, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR) {
        cout << "bind() error" << endl;
        return 1;
    }

    if (listen(servsock, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen() error" << endl;
        return 1;
    }

    HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    for (int i = 0; i < MAX_THREAD; i++) {
        HANDLE hThread = CreateThread(
            NULL, 0,
            (LPTHREAD_START_ROUTINE)WorkerThread,
            iocp, 0, NULL
        );

        CloseHandle(hThread);
    }

    while (TPoolRunning) {
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);

        SOCKET clisock = accept(servsock, NULL, NULL);
        if (clisock == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                cout << "accept() error" << endl;
                return 1;
            }

            continue;
        }

        u_long on = 1;
        if (ioctlsocket(clisock, FIONBIO, &on) == SOCKET_ERROR) {
            cout << "ioctlsocket() error" << endl;
            return 1;
        }

        Session* session = MemPool_new<Session>(*MemPool, clisock);

        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions.push_back(session);
        }

        cout << "Client Connected" << endl;

        CreateIoCompletionPort((HANDLE)session->sock, iocp, (ULONG_PTR)session, 0);

        // 초기화하고 다시 호출
        memset(session->recvBuf, 0, DEFAULT_BUFLEN);
        WSABUF wsabuf_R = {};
        wsabuf_R.buf = session->recvBuf;
        wsabuf_R.len = DEFAULT_BUFLEN;

        DWORD flags = 0;

        WSARecv(
            clisock, &wsabuf_R, 1,
            NULL, &flags, &session->readOverLapped, NULL
        );
    }

    for (auto session : sessions) {
        closesocket(session->sock);
        MemPool_delete(*MemPool, session);
    }

    CloseHandle(iocp);

    closesocket(servsock);
    WSACleanup();
    return 0;
}

void SendDataToClient(Session* session) {
    WSABUF wsabuf_S = {};
    wsabuf_S.buf = session->sendBuf + session->bytesSent;
    wsabuf_S.len = session->bytesToSend - session->bytesSent;

    DWORD flags = 0;

    WSASend(
        session->sock, &wsabuf_S, 1,
        NULL, 0, &session->writeOverLapped, NULL
    );
}

void WorkerThread(HANDLE iocpHd) {
    DWORD bytesTransfered;
    Session* session;
    LPOVERLAPPED lpOverlapped;
    WSABUF wsabuf_S = {}, wsabuf_R = {};

    while (TPoolRunning) {
        {
            std::unique_lock<std::mutex> lock(sessionsMutex);
            if (sessions.empty()) {
                continue;
            }

            session = sessions.back();
            sessions.pop_back();
        }

        while(true){
            bool ret = GetQueuedCompletionStatus(
            iocpHd, &bytesTransfered,
            (ULONG_PTR*)&session, &lpOverlapped, INFINITE
            );
            if (!ret || bytesTransfered == 0) {
                cout << "Client Disconnected" << endl;
                closesocket(session->sock);
                MemPool_delete(*MemPool, session);

                break;
            }
            if (lpOverlapped == &session->readOverLapped) {

                cout << "Received: " << session->recvBuf << endl;

                std::lock_guard<std::mutex> lock(sessionsMutex);
                for (auto& otherSession : sessions) {
                    if (otherSession != session) {
                        memcpy(otherSession->sendBuf, session->recvBuf, bytesTransfered);
                        otherSession->bytesToSend = bytesTransfered;
                        SendDataToClient(otherSession);
                    }
                }
                //초기화하고 다시 호출
                memset(session->recvBuf, 0, DEFAULT_BUFLEN);
                WSABUF wsabuf_R = {};
                wsabuf_R.buf = session->recvBuf;
                wsabuf_R.len = DEFAULT_BUFLEN;
                DWORD flags = 0;
                WSARecv(
                    session->sock, &wsabuf_R, 1,
                    NULL, &flags, &session->readOverLapped, NULL
                );
            }
            else if (lpOverlapped == &session->writeOverLapped) {
                session->bytesSent += bytesTransfered;

                if (session->bytesSent < session->bytesToSend) {
                    wsabuf_S.buf = session->sendBuf + session->bytesSent;
                    wsabuf_S.len = session->bytesToSend - session->bytesSent;

                    SendDataToClient(session);
                }
                else {
                    wsabuf_R.buf = session->recvBuf;
                    wsabuf_R.len = DEFAULT_BUFLEN;

                    DWORD flags = 0;

                    WSARecv(
                        session->sock, &wsabuf_R, 1,
                        NULL, &flags, &session->readOverLapped, NULL
                    );
                }
            }

        
        }
    }
}
#include "lib.h"

int main() {
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET servsock = socket(AF_INET, SOCK_STREAM, 0);
    if (servsock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
        return 0;
    }

    u_long on = 1;
    if (ioctlsocket(servsock, FIONBIO, &on) == SOCKET_ERROR) {
        cout << "ioctlsocket() error" << endl;
        return 0;
    }

    SOCKADDR_IN servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(12345);

    if (bind(servsock, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR) {
        cout << "bind() error" << endl;
        return 0;
    }

    if (listen(servsock, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen() error" << endl;
        return 0;
    }

    while (true) {
        SOCKADDR_IN cliaddr;
        int addrlen = sizeof(cliaddr);
        SOCKET clisock = accept(servsock, (SOCKADDR*)&cliaddr, &addrlen);
        if (clisock == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                continue;
            }
            else {
                cout << "accept() error" << endl;
                return 0;
            }
        }

        cout << "Client Connected" << endl;

        // 클라이언트가 요청한 경로에 따라 HTML 내용을 설정
        string requested_path;
        string html_content;

        if (requested_path == "/") {
            // 홈 페이지에 대한 HTML 내용
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>Home</title>
                </head>
                <body>
                    <h1>아무거나 끄적</h1>
                    <button onclick=\"navigateTo('/albamorning')\">오픈</button>
                    <button onclick=\"navigateTo('/albaclose')\">마감</button>
                    <button onclick=\"navigateTo('/timetable')\">24-1 시간표</button>
                    <button onclick=\"navigateTo('/memo')\">메모</button>
                    <button onclick=\"navigateTo('scheduler/')\">스케줄러</button>

                    <script>
                        function navigateTo(path) {
                            window.location.href = \"http://100.106.146.50:12345\" + path;
                        }
                    </script>
                </body>
                </html>
            )";
        } else if (requested_path == "/albamorning") {
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>오픈</title>
                </head>
                <body>
                    <h1>카페1</h1>
                    <p><strong>오픈 근무 시간:</strong></p>
                    <ul>
                        <li>토요일: 9:00 - 17:00</li>
                        <li>일요일: 10:00 - 17:00</li>
                    </ul>
                    <p><strong>해야하는 일:</strong></p>
                    <ul>
                        <li>a</li>
                        <li>b</li>
                        <li>c</li>
                    </ul>
                </body>
                </html>
            )";
        } else if (requested_path == "/albaclose") {
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>알바 마감</title>
                </head>
                <body>
                    <h1>주말 알바 마감</h1>
                </body>
                </html>
            )";
        } else if (requested_path == "/timetable") {
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>알바 마감</title>
                </head>
                <body>
                    <h1>주말 알바 마감</h1>
                </body>
                </html>
            )"; 
        } else if (requested_path == "/memo") {
            // 알바 마감 페이지에 대한 HTML 내용
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>알바 마감</title>
                </head>
                <body>
                    <h1>주말 알바 마감</h1>
                </body>
                </html>
            )";
        } else if (requested_path == "/scheduler") {
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>알바 마감</title>
                </head>
                <body>
                    <h1>주말 알바 마감</h1>
                </body>
                </html>
            )";
        } else {
            // 알 수 없는 경로에 대한 응답 설정
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>Error</title>
                </head>
                <body>
                    <h1>누구세요404</h1>
                    <p>찾을 수 없으세여..</p>
                </body>
                </html>
            )";
        }

        // HTML 내용을 클라이언트에 전송
        string http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + to_string(html_content.size()) + "\r\n\r\n" + html_content;
        const char* response_buffer = http_response.c_str();
        int response_length = http_response.size();

        int sendlen = send(clisock, response_buffer, response_length, 0);
        if (sendlen == SOCKET_ERROR) {
            cout << "send() error" << endl;
            closesocket(clisock);
            continue;
        }

        cout << "HTML Sent" << endl;

        closesocket(clisock);
        cout << "Client Disconnected" << endl;
    }

    closesocket(servsock);
    WSACleanup();
    return 0;
}

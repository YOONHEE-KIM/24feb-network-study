#include "lib.h"

int main() {
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET servsock = socket(AF_INET, SOCK_STREAM, 0);
    if (servsock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
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

    u_long on = 1;
    if (ioctlsocket(servsock, FIONBIO, &on) == SOCKET_ERROR) {
        cout << "ioctlsocket() error" << endl;
        return 0;
    }

    while (true) {
        SOCKADDR_IN cliaddr;
        int addrlen = sizeof(cliaddr);
        SOCKET clisock = accept(servsock, (SOCKADDR*)&cliaddr, &addrlen);
        if (clisock == INVALID_SOCKET) {
            int error_code = WSAGetLastError();
            if (error_code == WSAEWOULDBLOCK || error_code == WSAECONNABORTED) {
                continue;
            }
            else {
                cout << "accept() error" << endl;
                return 0;
            }
        }

        cout << "Client Connected" << endl;

        while (true) {
            char buf[1024];
            int recvlen = recv(clisock, buf, sizeof(buf), 0);
            if (recvlen == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                if (error_code == WSAEWOULDBLOCK) {
                    continue;
                }
                else {
                    cout << "recv() error" << endl;
                    closesocket(clisock);
                    break;
                }
            } else if (recvlen == 0) {
                cout << "Connection closed by client" << endl;
                closesocket(clisock);
                break;
            }


        string request_message(buf, recvlen);

        // 요청 메시지에서 첫 번째 줄을 추출
        istringstream iss(request_message);
        string first_line;
        getline(iss, first_line);

        // 첫 번째 줄에서 경로를 추출
        size_t start_pos = first_line.find(' ');
        size_t end_pos = first_line.find(' ', start_pos + 1);
        string requested_path = first_line.substr(start_pos + 1, end_pos - start_pos - 1);

        // HTTP/1.1 이후의 문자열이 포함되어 있으면 제거
        size_t http_pos = requested_path.find(" HTTP/1.1");
        if (http_pos != string::npos) {
            requested_path = requested_path.substr(0, http_pos);
        }

        // 클라이언트가 요청한 경로에 따라 HTML 내용을 설정
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
                    <button id="button1" onclick=\"navigateTo('/albamorning')\">오픈</button>
                    <button id="button2" onclick=\"navigateTo('/albaclose')\">마감</button>
                    <button id="button3" onclick=\"navigateTo('/timetable')\">24-1 시간표</button>
                    <button id="button4" onclick=\"navigateTo('/memo')\">메모</button>
                    <button id="button5" onclick=\"navigateTo('scheduler/')\">스케줄러</button>
                    <hr>
                    버튼 이벤트 처리가 아직이라 경로를 직접 입력해야 이동 가능
                    <br>
                    각각 /albamorning, /albaclose, /timetable, /memo, /scheduler 로 이동

                    <script>
                        document.getElementById("button1").addEventListener("click", function() {
                            navigateTo('/albamorning');
                        });
                        document.getElementById("button2").addEventListener("click", function() {
                            navigateTo('/albaclose');
                        });
                        document.getElementById("button3").addEventListener("click", function() {
                            navigateTo('/timetable');
                        });
                        document.getElementById("button4").addEventListener("click", function() {
                            navigateTo('/memo');
                        });
                        document.getElementById("button5").addEventListener("click", function() {
                            navigateTo('/scheduler');
                        });

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
                    <title>24-1 시간표</title>
                </head>
                <body>
                    <h1>수기 신청 성공 기원</h1>
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
                    <title>메모장</title>
                </head>
                <body>
                    <h1>메모장</h1>
                    <hr>
                    <h2>메모장</h2>
                    <h3>메모장</h3>
                    <h4>메모장</h4>
                    <h5>메모장</h5>
                </body>
                </html>
            )";
        } else if (requested_path == "/scheduler") {
            html_content = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="UTF-8">
                    <title>스케줄러</title>
                </head>
                <body>
                    <h1>오늘 할 일 : 숨쉬기</h1>
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
        }else {
                cout << "HTML Sent" << endl;
         } 
    }

        cout << "Client Disconnected" << endl;
        closesocket(clisock);
    }

    closesocket(servsock);
    WSACleanup();
    return 0;
}

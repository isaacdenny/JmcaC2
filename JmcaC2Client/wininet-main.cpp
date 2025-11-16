/**
 * Windows WinINet C2 Client Example
 * WinINet API to HTTP GET and POST requests
 * Compile: g++ wininet-main.cpp -lwininet -o wininet-client.exe
 */
#include <windows.h>
#include <wininet.h>

#include <iostream>

#pragma comment(lib, "Wininet.lib")

int getTasks();
int postResults();

int main(int argc, char* argv[]) {
    getTasks();
    // postResults();

    return 0;
}

int getTasks() {
    HINTERNET hSession =
        InternetOpenA("Mozilla/5.0",  // agent
                      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    HINTERNET hConnect = InternetOpenUrlA(hSession, "http://localhost:27015",
                                          NULL, 0, INTERNET_FLAG_RELOAD, 0);

    if (!hConnect) return 1;

    char buffer[4096];
    DWORD bytesRead;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) &&
           bytesRead) {
        buffer[bytesRead] = 0;
        std::cout << buffer;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);

    return 0;
}

int postResults() {
    HINTERNET hSession =
        InternetOpenA("Mozilla/5.0",  // agent
                      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    HINTERNET hConnect =
        InternetConnectA(hSession, "http://localhost:27015", 80, NULL, NULL,
                         INTERNET_SERVICE_HTTP, 0, 0);

    if (!hConnect) return 1;

    const char* headers = "Content-Type: text/plain";
    const char* data = "Hello, sir!";

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/post", NULL, NULL,
                                          NULL, INTERNET_FLAG_RELOAD, 0);

    if (!hRequest) return 1;

    HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data,
                     strlen(data));

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    return 0;
}
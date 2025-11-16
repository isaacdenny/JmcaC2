// consolidated than everything in windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winhttp.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
// #pragma comment(lib, "Mswsock.lib")
// #pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_TCP_PORT \
    "27015"  // TODO should be configureable but for dev hardcode to MSDN
#define HTTP_SERVER_IP L"10.240.244.195"
#define RESOURCE_NAME L"task.txt"
#define HTTP_VERB L"GET"
#define HTTP_SERVER_PORT 27015  // We coult just use INTERNET_DEFAULT_PORT

const wchar_t* ACCEPTED_MIME_TYPES[] = {
    L"text/plain", L"application/octet-stream", L"text/html", NULL};

// TODO: Setup server

int CreateTCPConn();
int getHTTPTask();

int getHTTPTask() {
    // TODO: This should be HTTPS but I'm using HTTP as a starting point
    // WinHTTP > WinINet

    /*WinHttpOpen -> WinHttpConnect (example.com) -> WinHttpOpenRequest   ->
    WinHttpAddRequestHeaders (user-agent/content-type) ->
    WinHttpSendRequest -> WinHttpReadData -> TASK -> Close / cleanup*/

    // https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/WinHttp/winhttp-sessions-overview.md#using-the-winhttp-api-to-access-the-web

    // returns a WinHTTP-session handle.
    HINTERNET hHTTPSession = {}, hHTTPConnection = {}, hHTTPRequest = {};
    bool isRequestSuccessful{}, didReceiveResponse{};

    hHTTPSession =
        WinHttpOpen(L"JmcaC2 Task Fetching",
                    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS, 0);  // SYNCHRONOUS (blocking)

    /*does not result in an actual connection to the HTTP server until a
    request is made for a specific resource*/

    if (hHTTPSession)
        hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                         (INTERNET_PORT)HTTP_SERVER_PORT, 0);

    if (hHTTPConnection)
        hHTTPRequest = WinHttpOpenRequest(
            hHTTPConnection, HTTP_VERB, RESOURCE_NAME, NULL, WINHTTP_NO_REFERER,
            ACCEPTED_MIME_TYPES,
            WINHTTP_FLAG_REFRESH);  // WINHTTP_FLAG_SECURE for HTTPS

    if (hHTTPRequest)
        isRequestSuccessful =
            WinHttpSendRequest(hHTTPRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0,
                               0);  // WINHTTP_NO_REQUEST_DATA
                                    // should be replaced if POST

    didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL);

    DWORD dwSize = 0;

    while (true) {
        if (!WinHttpQueryDataAvailable(hHTTPRequest, &dwSize) || !dwSize) break;

        char* outBuffer = new char[dwSize + 1];
        ZeroMemory(outBuffer, dwSize + 1);

        DWORD dwOut = 0;

        if (!WinHttpReadData(hHTTPRequest, outBuffer, dwSize, &dwOut)) {
            delete[] outBuffer;
            break;
        }

        printf("TASK: %.*s\n", dwOut, outBuffer);
        delete[] outBuffer;
    }

    if (hHTTPRequest) WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection) WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession) WinHttpCloseHandle(hHTTPSession);

    return (int)didReceiveResponse;
}

int CreateTCPConn(const char** argv) {
    const char* sendbuf = "Hello fr0m JMCAC2";

    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (res) {
        printf("WSA Startup Failed %d\n", res);
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL,
                    hints;  // conatiners for IP and to tell getaddrinfo what
                            // sockettype I want
    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo(argv[1], DEFAULT_TCP_PORT, &hints,
                              &result);  // Put get address info into result

    if (iResult) {
        printf("Getting Addr info failed! Did you pass an address?\n Error: %d",
               iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;

    ConnectSocket =
        socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Initializaing Socket Failed! Error %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, result->ai_addr, result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        printf("Failed to connect to server %s:%s, Error:  %d\n", argv[1],
               DEFAULT_TCP_PORT, WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);
    return 0;
}

int main(int argc, const char** argv) {
    /*TODO: Client should be able to send receive (does that mean the HTTP
     * Request needs to be async?) */
    getHTTPTask();
    return 0;
}
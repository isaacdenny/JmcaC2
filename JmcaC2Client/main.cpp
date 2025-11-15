// consolidated than everything in windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
// #pragma comment(lib, "Mswsock.lib")
// #pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT \
    "27015"  // TODO should be configureable but for dev hardcode to MSDN
// TODO: Setup server

int main(int argc, const char** argv) {
    // INIT SOCKET

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

    int iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints,
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
               DEFAULT_PORT, WSAGetLastError());
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
    // MAIN LOOP
    //  send & recv
}
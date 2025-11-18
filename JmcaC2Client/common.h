#ifndef H_COMMON
#define H_COMMON

#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <string>
#include <winioctl.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
// #pragma comment(lib, "Mswsock.lib")
// #pragma comment(lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_TCP_PORT \
    "27015"  // TODO should be configureable but for dev hardcode to MSDN
#define HTTP_SERVER_IP L"localhost"
#define RESOURCE_NAME L"index.html"
#define HTTP_SERVER_PORT 27015  // We coult just use INTERNET_DEFAULT_PORT

#endif
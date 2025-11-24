#ifndef H_COMMON
#define H_COMMON

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tlhelp32.h>
#include <winhttp.h>
#include <winioctl.h>
#include <ws2tcpip.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include <string>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "User32.lib")

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
// #pragma comment(lib, "Mswsock.lib")
// #pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PS_BUFLEN 4096
#define DEFAULT_TCP_PORT "27016"  // Should be configureable
#define HTTP_SERVER_IP L"localhost"
#define RESOURCE_NAME L"/"
#define HTTP_SERVER_PORT 27015  // Should be configureable
#endif
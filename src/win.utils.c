#include "win.utils.h"
#include <tlhelp32.h>
#include <math.h>

#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next

// 获取进程的 pid
DWORD Win_GetPid(const char* processName) {
    HANDLE snapshot;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 pe;
    DWORD pid = 0;

    pe.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot, &pe)) {
        CloseHandle(snapshot);
        return 0;
    }

    do {
        if (strcmp(pe.szExeFile, processName) == 0) {
            pid = pe.th32ProcessID;
            CloseHandle(snapshot);
            return pid;
        }
    } while (Process32Next(snapshot, &pe));

    CloseHandle(snapshot);
    return 0;
}



// 遍历所有进程的hwnd，通过判断pid来找到目标进程的hwnd
DWORD __targetPid;
BOOL CALLBACK Win_EnumWndsProc(HWND hwnd, LPARAM lParam) {
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == __targetPid) {
        *(HWND*)lParam = hwnd;
        return FALSE;
    }
    return TRUE;
}



// 获得目标进程的 hwnd
HWND Win_GetHwnd(DWORD pid) {
    __targetPid = pid;
    HWND hwnd = NULL;
    EnumWindows(Win_EnumWndsProc, (LPARAM)&hwnd);
    return hwnd;
}



// 获取窗口大小
SIZE Win_GetWndSize(HWND hwnd) {
    ShowWindow(hwnd, SW_RESTORE);
    unsigned dpi = GetDpiForWindow(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);

    int width = round((rect.right - rect.left) * (dpi / 96.0));
    int height = round((rect.bottom - rect.top) * (dpi / 96.0));

    return (SIZE){ .cx = width, .cy = height };
}



// 检查多个按键是否按下
BOOL Win_IsKeysDownEx(const int *vKeys) {
    for(; *vKeys; vKeys++)
        if(!(GetAsyncKeyState(*vKeys) & 0x8000))
            return FALSE;
    return TRUE;
}
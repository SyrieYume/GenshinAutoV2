// 基于 win32 API 的一些工具函数
#pragma once
#include <windows.h>

// 获取目标进程的 pid
DWORD Win_GetPid(const char* processName);

// 获得目标进程的 hwnd
HWND Win_GetHwnd(DWORD pid);

// 获取窗口大小
SIZE Win_GetWndSize(HWND hwnd);

// 检查多个按键是否按下
BOOL Win_IsKeysDownEx(const int *vKeys);

// 检查多个按键是否按下
#define Win_IsKeysDown(...)  Win_IsKeysDownEx((int[]){ __VA_ARGS__, 0 })


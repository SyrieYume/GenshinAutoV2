#include "console.utils.h"

// 初始化控制台
BOOL Con_Init(const char* title, int cols, int lines) {
    // 分配一个新的控制台，gcc编译时需要加上 -mwindows 才能生效
    if(!AllocConsole()) 
        return FALSE;
       
    // 重定向标准输入和标准输出到新的控制台
    FILE* newStream;
    freopen_s(&newStream, "CONIN$", "r", stdin);
    freopen_s(&newStream, "CONOUT$", "w", stdout);

    // 获取控制台窗口的句柄
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if(hOut == INVALID_HANDLE_VALUE)
        return FALSE;

    // 开启 Windows 虚拟终端序列
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return FALSE;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
        return FALSE;

    // 设置控制台编码方式为utf-8
    SetConsoleOutputCP(CP_UTF8);

    // 设置控制台字体为黑体，防止出现乱码
    CONSOLE_FONT_INFOEX cfi = {
        .cbSize = sizeof(CONSOLE_FONT_INFOEX),
        .dwFontSize.X = 0,
        .dwFontSize.Y = 16,
        .FaceName = L"SimHei"
    };
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

    // 设置控制台标题
    SetConsoleTitleA(title);

    // 设置控制台大小
    char command[64];
    sprintf_s(command, 64, "mode con cols=%d lines=%d", cols, lines);
    system(command);
    SetConsoleScreenBufferSize(hOut, (COORD){cols, lines * 10});

    system("cls");

    // 设置窗口透明度
    HWND hwnd = GetConsoleWindow();
    SetLayeredWindowAttributes(hwnd, 0, 225, LWA_ALPHA);
    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, GetWindowLongPtrA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    
    // 让窗口位于最前
    // SetWindowLongPtrA(hwnd, GWL_EXSTYLE, GetWindowLongPtrA(hwnd, GWL_EXSTYLE) | WS_EX_TOPMOST);
    // SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // 隐藏控制台光标
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    return TRUE;
}
module;

#include <string>
#include <cstdio>
#include <windows.h>

export module console;

export namespace console {
    auto init(const std::string& title, short cols, short lines) -> bool;

    void print(const char* text);

    void info(const std::string& text);
    
    void error(const std::string& text);
}


export namespace console::ansi {
    constexpr char bold[] = "\x1b[1m";
    constexpr char reset[] = "\033[0m";
    constexpr char cleanLine[] = "\r\033[K";
    
    #define rgb(R, G, B) "\x1b[38;2;" #R ";" #G ";" #B "m"
    constexpr char green[] = rgb(96, 200, 135);
    constexpr char blue[] = rgb(76, 186, 250);
    constexpr char orange[] = rgb(241, 149, 88);
    constexpr char red[] = rgb(247, 101, 104);
    #undef rgb
}


auto console::init(const std::string& title, short cols, short lines) -> bool {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if(hOutput == INVALID_HANDLE_VALUE)
        return false;

    // 开启 Windows 虚拟终端序列
    // https://learn.microsoft.com/zh-cn/windows/console/console-virtual-terminal-sequences
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOutput, &dwMode))
        return false;

    if (!SetConsoleMode(hOutput, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return false;

    // 设置控制台编码方式为utf-8
    SetConsoleOutputCP(CP_UTF8);

    // 设置控制台字体为黑体，防止出现乱码
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX),
    cfi.dwFontSize.X = 0,
    cfi.dwFontSize.Y = 16,
    wcscpy_s(cfi.FaceName, 32, L"SimHei");
    
    SetCurrentConsoleFontEx(hOutput, FALSE, &cfi);

    // 设置控制台标题
    SetConsoleTitleA(title.c_str());

    // 设置控制台大小
    char command[64];
    sprintf_s(command, 64, "mode con cols=%d lines=%d", cols, lines);
    system(command);
    SetConsoleScreenBufferSize(hOutput, (COORD){ cols, static_cast<short>(lines * 10) });

    system("cls");

    if(HWND hwnd = GetConsoleWindow()) {
        // 设置窗口透明度
        SetLayeredWindowAttributes(hwnd, 0, 225, LWA_ALPHA);
        SetWindowLongPtrA(hwnd, GWL_EXSTYLE, GetWindowLongPtrA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        
        // 让窗口位于最前
        // SetWindowLongPtrA(hwnd, GWL_EXSTYLE, GetWindowLongPtrA(hwnd, GWL_EXSTYLE) | WS_EX_TOPMOST);
        // SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    // 隐藏控制台光标
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOutput, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOutput, &cursorInfo);
    return true;
}


void console::print(const char* text) {
    printf("%s", text);
}

void console::info(const std::string& text) {
    printf("%s[Info]%s %s\n", ansi::green, ansi::reset, text.c_str());
}

void console::error(const std::string& text) {
    printf("%s[Error]%s %s\n", ansi::red, ansi::reset, text.c_str());
}
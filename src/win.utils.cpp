module;

#define WINVER 0x0A00
#define NOMINMAX

#include <filesystem>
#include <tuple>
#include <utility>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <Windows.h>
#include <tlhelp32.h>

export module win;


export namespace win {
    auto loadResourceToFile(WORD resourceId, std::filesystem::path filepath) -> bool;
    auto getPid(const char* processName) -> DWORD;
    auto getHwnd(DWORD pid) -> HWND;
    auto getBaseDir() -> std::filesystem::path;
    auto getWndSize(HWND hwnd);
    auto captureWindow(HWND hwnd, std::tuple<int, int, int, int> area);
    auto saveBitmapImage(const char* savepath, std::byte* data, int width, int height, int step) -> bool;
}


// 从程序资源中加载数据，并且写入文件
bool win::loadResourceToFile(WORD resourceId, std::filesystem::path filepath) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA); 
    if(!hResource)
        return false;

    HGLOBAL hGlobal = LoadResource(NULL, hResource);
    DWORD dwSize = SizeofResource(NULL, hResource);
    std::byte* data = reinterpret_cast<std::byte*>(LockResource(hGlobal));

    UnlockResource(hGlobal);

    HANDLE hFile = CreateFileW(filepath.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;
    
    DWORD dwWrite = 0;
    if (!WriteFile(hFile, data, dwSize, &dwWrite, NULL) || dwWrite != dwSize) {
        CloseHandle(hFile);
        return false;
    }
    
    CloseHandle(hFile);
    return true;
}


auto win::getPid(const char* processName) -> DWORD {
    wchar_t wszProcessName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, processName, -1, wszProcessName, MAX_PATH);

    HANDLE snapshot;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe;
    DWORD pid = 0;

    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32FirstW(snapshot, &pe)) {
        CloseHandle(snapshot);
        return 0;
    }

    do {
        if (wcscmp(pe.szExeFile, wszProcessName) == 0) {
            pid = pe.th32ProcessID;
            CloseHandle(snapshot);
            return pid;
        }
    } while (Process32NextW(snapshot, &pe));

    CloseHandle(snapshot);
    return 0;
}


auto win::getHwnd(DWORD pid) -> HWND {
    using EnumWindowParam = std::pair<HWND, DWORD>;

    EnumWindowParam param = { nullptr, pid };
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumWindowParam* param = reinterpret_cast<EnumWindowParam*>(lParam);
        auto& [result, targetPid] = *param;
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == targetPid) {
            result = hwnd;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&param));

    return param.first;
}


auto win::getWndSize(HWND hwnd) {
    unsigned dpi = GetDpiForWindow(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = std::lround((rect.right - rect.left) * (dpi / 96.0));
    int height = std::lround((rect.bottom - rect.top) * (dpi / 96.0));

    return std::make_tuple(
        std::make_pair("width", width),
        std::make_pair("height", height)
    );
}

auto win::getBaseDir() -> std::filesystem::path {
    wchar_t wszPath[MAX_PATH + 1];
    if (GetModuleFileNameW(nullptr, wszPath, MAX_PATH) == 0)
        return std::filesystem::current_path();
    return std::filesystem::path(wszPath).parent_path();
}


auto win::captureWindow(HWND hwnd, std::tuple<int, int, int, int> area) {
    auto image = std::make_tuple(
        std::make_pair("width", 0),
        std::make_pair("height", 0),
        std::make_pair("channels", 0),
        std::make_pair("step", 0),
        std::make_pair("data", std::pair<std::byte*, size_t>(nullptr, 0))
    );

    auto& [left, top, right, bottom] = area;
    int width = right - left;
    int height = bottom - top;

    if (!IsWindow(hwnd) || width <= 0 || height <= 0)
        return image;

    HDC hdc_window = GetDC(hwnd);
    if (!hdc_window)
        return image;

    auto wndSize = win::getWndSize(hwnd);
    auto& wndWidth = std::get<0>(wndSize).second;
    auto& wndHeight = std::get<1>(wndSize).second;

    int capture_x = std::max(left, 0);
    int capture_y = std::max(top, 0);
    int capture_width = (left + width > wndWidth) ? (wndWidth - capture_x) : width;
    int capture_height = (top + height > wndHeight) ? (wndHeight - capture_y) : height;
    
    if (capture_width <= 0 || capture_height <= 0) {
        ReleaseDC(hwnd, hdc_window);
        return image;
    }
    
    // 创建一个与窗口DC兼容的内存DC
    HDC hdc_mem = CreateCompatibleDC(hdc_window);
    if (!hdc_mem) {
        ReleaseDC(hwnd, hdc_window);
        return image;
    }

    // 创建一个兼容的位图 (DIB Section) 以便直接访问像素数据
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = capture_width;
    bi.bmiHeader.biHeight = -capture_height; // 负值表示顶-底DIB，数据从左上角开始，与OpenCV布局一致
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32; // 32位，BGRA
    bi.bmiHeader.biCompression = BI_RGB;

    void* pixels_data = NULL;
    HBITMAP hbmp = CreateDIBSection(hdc_mem, &bi, DIB_RGB_COLORS, &pixels_data, NULL, 0);
    if (!hbmp || !pixels_data) {
        DeleteDC(hdc_mem);
        ReleaseDC(hwnd, hdc_window);
        return image;
    }

    // 将位图选入内存DC，并保存旧的位图
    HGDIOBJ hbmp_old = SelectObject(hdc_mem, hbmp);

    // 使用 BitBlt 将窗口内容复制到内存DC（进而复制到位图）
    // 从窗口客户区的 (capture_x, capture_y) 开始，复制 capture_width x capture_height 的内容到内存DC的 (0, 0) 位置
    BitBlt(hdc_mem, 0, 0, capture_width, capture_height, hdc_window, capture_x, capture_y, SRCCOPY);

    std::get<0>(image).second = capture_width;
    std::get<1>(image).second = capture_height;
    std::get<2>(image).second = 4; // BGRA
    std::get<3>(image).second = capture_width * 4;
    std::get<4>(image).second.first = new std::byte[capture_width * capture_height * 4];
    std::get<4>(image).second.second = capture_width * capture_height * 4;

    memcpy(std::get<4>(image).second.first, pixels_data, capture_width * capture_height * 4);

    // 清理GDI资源
    SelectObject(hdc_mem, hbmp_old);
    DeleteObject(hbmp);
    DeleteDC(hdc_mem);
    ReleaseDC(hwnd, hdc_window);

    return image;
}


bool win::saveBitmapImage(const char* savepath, std::byte* data, int width, int height, int step) {
    if (!data || width <= 0 || height <= 0)
        return false;

    // 设置BMP文件头 (BITMAPFILEHEADER)
    BITMAPFILEHEADER bmfh = { };
    bmfh.bfType = 0x4D42; // 'BM'
    bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (step * height);
    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // 2. 设置BMP信息头 (BITMAPINFOHEADER)
    BITMAPINFOHEADER bmih = { };
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = width;
    bmih.biHeight = -height; // 正值表示底-顶DIB，保存时需翻转或按底-顶格式保存
    bmih.biPlanes = 1;
    bmih.biBitCount = 32; // 32位
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = 0; // BI_RGB时可以为0
    
    // 创建文件
    wchar_t wczFilepath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, savepath, -1, wczFilepath, MAX_PATH);
    HANDLE hFile = CreateFileW(wczFilepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD bytesWritten;

    // 写入文件头和信息头
    WriteFile(hFile, &bmfh, sizeof(bmfh), &bytesWritten, NULL);
    WriteFile(hFile, &bmih, sizeof(bmih), &bytesWritten, NULL);
    WriteFile(hFile, data, step * height, &bytesWritten, NULL);

    CloseHandle(hFile);
    return true;
}
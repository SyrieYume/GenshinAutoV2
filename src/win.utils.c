#include "win.utils.h"
#include <tlhelp32.h>
#include <math.h>

// 从程序资源中加载数据，并且写入文件
BOOL Win_LoadResourceToFile(int resourceId, wchar_t* filepath) {   
    HRSRC hResource = FindResourceA(NULL, MAKEINTRESOURCEA(resourceId), MAKEINTRESOURCEA(RT_RCDATA)); 
    if(!hResource)
        return FALSE;

    HGLOBAL hGlobal = LoadResource(NULL, hResource);
    DWORD dwSize = SizeofResource(NULL, hResource);
    BYTE *data = (BYTE*)malloc(dwSize);
    memcpy(data, (BYTE*)LockResource(hGlobal), dwSize);

    UnlockResource(hGlobal);

    HANDLE hFile = CreateFileW(filepath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        free(data);
        return FALSE;
    }
    
    DWORD dwWrite = 0;
    if (!WriteFile(hFile, data, dwSize, &dwWrite, NULL) || dwWrite != dwSize) {
        free(data);
        CloseHandle(hFile);
        return FALSE;
    }
    
    CloseHandle(hFile);
    free(data);
    return TRUE;
}

// 获取进程的 pid
DWORD Win_GetPid(const wchar_t* wszProcessName) {
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
    ShowWindow(hwnd, SW_SHOW);
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



// 截取窗口指定区域
Image Win_CaptureWindow(HWND hwnd, int left, int top, int right, int bottom) {
    Image image = NULL;
    int width = right - left;
    int height = bottom - top;

    if (!IsWindow(hwnd) || width <= 0 || height <= 0)
        return image;

    HDC hdc_window = GetDC(hwnd);
    if (!hdc_window)
        return image;

    SIZE client_size = Win_GetWndSize(hwnd);

    int capture_x = (left < 0) ? 0 : left;
    int capture_y = (top < 0) ? 0 : top;
    int capture_width = (left + width > client_size.cx) ? (client_size.cx - capture_x) : width;
    int capture_height = (top + height > client_size.cy) ? (client_size.cy - capture_y) : height;
    
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

    void* pixel_data = NULL;
    HBITMAP hbmp = CreateDIBSection(hdc_mem, &bi, DIB_RGB_COLORS, &pixel_data, NULL, 0);
    if (!hbmp || !pixel_data) {
        DeleteDC(hdc_mem);
        ReleaseDC(hwnd, hdc_window);
        return image;
    }

    // 将位图选入内存DC，并保存旧的位图
    HGDIOBJ hbmp_old = SelectObject(hdc_mem, hbmp);

    // 使用 BitBlt 将窗口内容复制到内存DC（进而复制到位图）
    // 从窗口客户区的 (capture_x, capture_y) 开始，复制 capture_width x capture_height 的内容到内存DC的 (0, 0) 位置
    BitBlt(hdc_mem, 0, 0, capture_width, capture_height, hdc_window, capture_x, capture_y, SRCCOPY);

    // 填充Image结构体
    image = (Image)malloc(sizeof(*image));
    image->width = capture_width;
    image->height = capture_height;
    image->channels = 4; // BGRA
    image->step = capture_width * 4;

    // 分配内存并复制像素数据，使Image对象独立于GDI对象
    image->data = malloc(image->step * image->height);
    memcpy(image->data, pixel_data, image->step * image->height);


    // 9. 清理GDI资源
    SelectObject(hdc_mem, hbmp_old);
    DeleteObject(hbmp);
    DeleteDC(hdc_mem);
    ReleaseDC(hwnd, hdc_window);

    return image;
}

WINBOOL Win_SaveImage(Image image, wchar_t* filepath) {
    if (!image || !image->data || image->width <= 0 || image->height <= 0)
        return FALSE;

    // 设置BMP文件头 (BITMAPFILEHEADER)
    BITMAPFILEHEADER bmfh = { 0 };
    bmfh.bfType = 0x4D42; // 'BM'
    bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (image->step * image->height);
    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // 2. 设置BMP信息头 (BITMAPINFOHEADER)
    BITMAPINFOHEADER bmih = { 0 };
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = image->width;
    bmih.biHeight = image->height; // 正值表示底-顶DIB，保存时需翻转或按底-顶格式保存
    bmih.biPlanes = 1;
    bmih.biBitCount = 32; // 32位
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = 0; // BI_RGB时可以为0
    
    // 创建文件
    HANDLE hFile = CreateFileW(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD bytesWritten;

    // 写入文件头和信息头
    WriteFile(hFile, &bmfh, sizeof(bmfh), &bytesWritten, NULL);
    WriteFile(hFile, &bmih, sizeof(bmih), &bytesWritten, NULL);

    // 写入像素数据
    // BMP格式要求数据是 bottom-up 的，而我们的截图是 top-down 的
    // 因此需要逐行反向写入文件
    for (int i = image->height - 1; i >= 0; i--) {
        char* row = (char*)image->data + i * image->step;
        WriteFile(hFile, row, image->step, &bytesWritten, NULL);
    }

    CloseHandle(hFile);
    return TRUE;
}


void Win_FreeImage(Image image) {
    if(image) {
        free(image->data);
        free(image);
    }
}
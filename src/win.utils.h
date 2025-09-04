// 基于 win32 API 的一些工具函数
#pragma once
#include <windows.h>

// 定义一个与OpenCV的Mat对象二进制兼容的图像结构体
// OpenCV的Mat可以由这些数据直接构造:
// cv::Mat mat(image->height, image->width, CV_8UC4, image->data, image->step);
typedef struct {
    int width;      // 图像宽度（像素）
    int height;     // 图像高度（像素）
    int channels;   // 通道数 (这里将是4, BGRA)
    size_t step;    // 每一行的字节数, 通常是 width * channels
    void* data;     // 指向像素数据的指针
} *Image;

// 从程序资源中加载数据，并且写入文件
BOOL Win_LoadResourceToFile(WORD resourceId, wchar_t* filepath);

// 获取目标进程的 pid
DWORD Win_GetPid(const wchar_t* wszProcessName);

// 获得目标进程的 hwnd
HWND Win_GetHwnd(DWORD pid);

// 获取窗口大小
SIZE Win_GetWndSize(HWND hwnd);

// 检查多个按键是否按下
BOOL Win_IsKeysDownEx(const int *vKeys);

// 检查多个按键是否按下
#define Win_IsKeysDown(...)  Win_IsKeysDownEx((int[]){ __VA_ARGS__, 0 })

// 截取窗口指定区域
Image Win_CaptureWindow(HWND hwnd, int x, int y, int width, int height);

// 将图片以bmp格式保存
BOOL Win_SaveImage(Image image, wchar_t* filepath);

// 释放图片数据的内存
void Win_FreeImage(Image image);
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "win.utils.h"
#include "console.utils.h"

typedef struct {
    int x, y; 
} Point2D;

// 1920 x 1080 分辨率下的像素点坐标
Point2D points[] = {
    { 290, 37 },  // 隐藏对话按钮的白色部分
    { 289, 48 },  // 隐藏对话按钮的黑色部分 
    { 279, 28 },  // 隐藏对话按钮左上角
    { 319, 68 }   // 隐藏对话按钮右下角
};



// 根据实际的分辨率大小调整上面的像素点坐标
void initPoints(SIZE wndSize) {
    double scale;
    if((wndSize.cx * 9) > (wndSize.cy * 16))
        scale = wndSize.cy / 1080.0;
    else
        scale = wndSize.cx / 1920.0;

    int offset_y = wndSize.cy - 1080 * scale;

    for(int i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
        points[i].x = round(points[i].x * scale);
        points[i].y = round(points[i].y * scale);
    }
}



int main() {
    DWORD pid;
    HWND hwnd;
    SIZE wndSize;
    HDC hdc;

    if(!Con_Init("GenshinAuto v2", 80, 24))
        Con_Error("初始化控制台窗口时出错");

    Con_Waiting("正在等待原神进程 " ANSI_BLUE("YuanShen.exe") " / " ANSI_BLUE("GenshinImpact.exe"));

    while(!((pid = Win_GetPid("YuanShen.exe")) || (pid = Win_GetPid("GenshinImpact.exe")))) 
        Sleep(200);

    printf(ANSI_CLEAN_LINE);

    Con_Info(ANSI_BLUE("Pid") " = %lld", pid);


    Con_Waiting("正在等待原神窗口");

    while (TRUE) {
        hwnd = Win_GetHwnd(pid);
        wndSize = Win_GetWndSize(hwnd);
        if(wndSize.cx > 400) 
            break;
        Sleep(200);
    } 
    
    printf(ANSI_CLEAN_LINE);

    Con_Info(ANSI_BLUE("Hwnd") " = 0x%llx", hwnd);
    Con_Info(ANSI_BLUE("窗口大小") ": %d" ANSI_BLUE(" X ") "%d", wndSize.cx, wndSize.cy);

    initPoints(wndSize);

    Con_Info("开始自动点击剧情中(当检测到进入剧情时会自动点击)...");
    Con_Info("按" ANSI_BLUE("Ctrl+p") "键暂停.");

    hdc = GetDC(hwnd);

    BOOL isActivate = TRUE;
    
    // afterDialog：值为0表示正在剧情对话中，值为1表示不在剧情对话中，大于1表示剧情对话刚刚结束（会在几秒内递减到1）
    int afterDialog = 1;
    
    while(TRUE) {
        if (Win_IsKeysDown(VK_CONTROL, 'P')) {
            isActivate = !isActivate;
            if (isActivate) 
                Con_Info("程序继续" ANSI_GREEN("执行") "中");
            else 
                Con_Info("程序" ANSI_ORANGE("暂停") "中");
            Sleep(400);
        }

        if (isActivate) {
            // 判断左上角的隐藏对话按钮
            if (GetPixel(hdc, points[0].x, points[0].y) == RGB(236,229,216) && GetPixel(hdc, points[1].x, points[1].y) == RGB(59, 67, 84)) {
                if(afterDialog > 0) {
                    afterDialog = 0;
                    Con_Info("检测到进入剧情对话");
                }
                SetForegroundWindow(hwnd);
                // 发送点击 F 键的消息
                PostMessageW(hwnd, WM_KEYDOWN, 0x46, 0x210001);
                Sleep(75);
                PostMessageW(hwnd, WM_KEYUP, 0x46, 0xC0210001);
            }
            else if(afterDialog == 0) {
                afterDialog = 16;
                Con_Info("剧情对话结束");
            }
        }

        // 对话结束之后，原神会将鼠标强制锁到窗口中央，这里在剧情对话结束后几秒内解除鼠标锁定
        if(afterDialog > 1) {
            afterDialog--;
            ClipCursor(NULL);
        }

        Sleep(125);
    }

    ReleaseDC(hwnd, hdc);

    return 0;
}
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "win.utils.h"
#include "console.utils.h"

#define MAX_POINTS 3

typedef struct {
    int x, y;
} Point2D;

typedef struct {
    Point2D point;
    const char *description;
} LabeledPoint;

// 1920 x 1080 分辨率下的像素点坐标
LabeledPoint points[] = {
    {{290, 37}, "隐藏对话按钮的白色部分"},
    {{289, 48}, "隐藏对话按钮的黑色部分"},
    {{1280, 1286}, "中间底部点击后继续橙黄色部分"},
    // {{319, 68}, "隐藏对话按钮右下角"}
};

int point_count = sizeof(points) / sizeof(points[0]);

int load_points_from_args(const int argc, char *argv[], Point2D *out_points, const int max_points) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--points") == 0 && i + 1 < argc) {
            int j = 0;
            for (int k = i + 1; k < argc && j < max_points; ++k) {
                int x, y;
                if (sscanf(argv[k], "%d,%d", &x, &y) == 2) {
                    out_points[j++] = (Point2D){x, y};
                } else {
                    break;
                }
            }
            return j;
        }
    }
    return 0;
}

int load_points_from_file(Point2D *out_points, const int max_points) {
    char path[MAX_PATH];
    FILE *fp = NULL;

    // 1. 尝试打开当前工作目录下的配置文件
    snprintf(path, MAX_PATH, ".genshin_points.conf");
    fp = fopen(path, "r");
    if (fp == NULL) {
        Con_Info("工作目录下没有.genshin_points.conf, 尝试从HOME目录读取");
    }

    // 2. 如果当前路径没有，再尝试用户目录
    if (!fp) {
        const char *home = getenv("USERPROFILE");
        if (!home) return 0;

        snprintf(path, MAX_PATH, "%s\\.genshin_points.conf", home);
        fp = fopen(path, "r");
        if (!fp) return 0;
    }

    int i = 0;
    while (i < max_points && fscanf(fp, "%d %d", &out_points[i].x, &out_points[i].y) == 2) {
        i++;
    }
    fclose(fp);
    return i;
}

// 返回值：1 表示使用默认点（需要缩放和偏移），0 表示从外部读取点（无需调整）
int initialize_points(const int argc, char *argv[]) {
    Point2D temp_points[MAX_POINTS];
    int count = load_points_from_args(argc, argv, temp_points, MAX_POINTS);

    if (count == 0) {
        count = load_points_from_file(temp_points, MAX_POINTS);
    }

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            points[i].point.x = temp_points[i].x;
            points[i].point.y = temp_points[i].y;
        }
        point_count = count;
        printf("已加载 %d 个点（来自参数或配置）。\n", point_count);
        return 0;
    }
    printf("使用默认点坐标, 1080P下全屏为基准。\n");
    return 1;
}

// 根据实际的分辨率大小调整上面的像素点坐标
void initPoints(const SIZE wndSize) {
    double scale;
    if ((wndSize.cx * 9) > (wndSize.cy * 16))
        scale = wndSize.cy / 1080.0;
    else
        scale = wndSize.cx / 1920.0;

    // int offset_y = wndSize.cy - 1080 * scale;
    printf("点位偏移因子: %.4f\n", scale);

    for (int i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
        points[i].point.x = round(points[i].point.x * scale);
        points[i].point.y = round(points[i].point.y * scale);

        printf("Scaled point[%d]: (%d, %d) - %s\n",
               i,
               points[i].point.x,
               points[i].point.y,
               points[i].description);
    }
}

BOOL inNormal(const HDC hdc) {
    const COLORREF LEFT_TOP_COLOR = GetPixel(hdc, points[0].point.x, points[0].point.y);
    const COLORREF LEFT_TOP_COLOR2 = GetPixel(hdc, points[1].point.x, points[1].point.y);
    if (LEFT_TOP_COLOR == RGB(236, 229, 216) && LEFT_TOP_COLOR2 == RGB(59, 67, 84)) {
        return TRUE;
    }
    // 黑屏也算
    return LEFT_TOP_COLOR == RGB(0, 0, 0) && LEFT_TOP_COLOR2 == RGB(0, 0, 0);
}

BOOL inOthers(const HDC hdc) {
    const COLORREF COLOR = GetPixel(hdc, points[2].point.x, points[2].point.y);
    return  COLOR == RGB(255, 195, 0);
}

int main(const int argc, char *argv[]) {
    if (!Con_Init("GenshinAuto v2", 80, 24))
        Con_Error("初始化控制台窗口时出错");

    DWORD pid = 0;
    HWND hwnd = NULL;
    SIZE wndSize;
    HDC hdc = NULL;

wait_process:
    pid = 0;
    hwnd = NULL;
    hdc = NULL;

    Con_Waiting("正在等待原神进程 " ANSI_BLUE("YuanShen.exe") " / " ANSI_BLUE("GenshinImpact.exe"));

    // 等待原神进程启动
    while (!((pid = Win_GetPid("YuanShen.exe")) || (pid = Win_GetPid("GenshinImpact.exe"))))
        Sleep(2000);

    printf(ANSI_CLEAN_LINE);
    Con_Info(ANSI_BLUE("Pid") " = %lu", pid);
    Con_Waiting("正在等待原神窗口");

    // 等待窗口初始化完成
    while (TRUE) {
        // 检查进程是否仍然存在
        if (!Win_GetPid("YuanShen.exe") && !Win_GetPid("GenshinImpact.exe")) {
            Con_Info("进程已关闭，重新等待进程启动");
            goto wait_process;
        }

        hwnd = Win_GetHwnd(pid);
        wndSize = Win_GetWndSize(hwnd);
        if (wndSize.cx > 400)
            break;

        Sleep(200);
    }

    printf(ANSI_CLEAN_LINE);
    Con_Info(ANSI_BLUE("Hwnd") " = 0x%p", hwnd);
    Con_Info(ANSI_BLUE("窗口大小") ": %d" ANSI_BLUE(" X ") "%d", wndSize.cx, wndSize.cy);

    const int need_scale_adjust = initialize_points(argc, argv);
    for (int i = 0; i < point_count; ++i) {
        printf("点[%d]: (%d, %d) - %s\n",
               i,
               points[i].point.x,
               points[i].point.y,
               points[i].description);
    }
    if (need_scale_adjust) {
        initPoints(wndSize);
    }

    Con_Info("开始自动点击剧情中(当检测到进入剧情时会自动点击)...");
    Con_Info("按" ANSI_BLUE("Ctrl+p") "键暂停，按" ANSI_BLUE("Ctrl+q") "键退出。");

    hdc = GetDC(hwnd);

    BOOL isActivate = TRUE;

    // afterDialog：值为0表示正在剧情对话中，值为1表示不在剧情对话中，大于1表示剧情对话刚刚结束（会在几秒内递减到1）
    int afterDialog = 1;

    while (TRUE) {
        // 添加退出快捷键
        if (Win_IsKeysDown(VK_CONTROL, 'Q')) {
            Con_Info("用户按下退出快捷键，程序退出");
            ReleaseDC(hwnd, hdc);
            return 0;
        }

        if (Win_IsKeysDown(VK_CONTROL, 'P')) {
            isActivate = !isActivate;
            if (isActivate)
                Con_Info("程序继续" ANSI_GREEN("执行") "中");
            else
                Con_Info("程序" ANSI_ORANGE("暂停") "中");
            Sleep(400);
        }

        // 检查进程是否仍然存在
        if (!IsProcessAlive(pid)) {
            Con_Info("进程已关闭，重新等待进程启动");
            if (hdc != NULL) {
                ReleaseDC(hwnd, hdc);
            }

            // 等待进程退出
            const HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
            if (hProcess != NULL) {
                WaitForSingleObject(hProcess, INFINITE);
                CloseHandle(hProcess);
            }
            goto wait_process;
        }

        if (isActivate) {
            // 判断左上角的隐藏对话按钮
            if (inNormal(hdc) || inOthers(hdc)) {
                if (afterDialog > 0) {
                    afterDialog = 0;
                    Con_Info("检测到进入剧情对话");
                }
                SetForegroundWindow(hwnd);
                // 发送点击 F 键的消息
                PostMessageW(hwnd, WM_KEYDOWN, 0x46, 0x210001);
                Sleep(75);
                PostMessageW(hwnd, WM_KEYUP, 0x46, 0xC0210001);
            } else if (afterDialog == 0) {
                afterDialog = 16;
                Con_Info("剧情对话结束");
            }
        }

        // 对话结束之后，原神会将鼠标强制锁到窗口中央，这里在剧情对话结束后几秒内解除鼠标锁定
        if (afterDialog > 1) {
            afterDialog--;
            ClipCursor(NULL);
        }

        Sleep(125);
    }
}

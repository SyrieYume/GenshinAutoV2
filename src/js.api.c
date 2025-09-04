// 供JavaScript调用的函数
// 函数格式统一为 Js_Api_xxxxx(ctx, thisVal, argc, argv)

#include "js.api.h"
#include <windows.h>
#include <stdio.h>
#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>
#include "win.utils.h"
#include "js.api.utils.h"

static struct { char* name; void* func; } jsApis[100];
static int jsApisCount = 0;

// 将数组传递给js时需要一个回调函数，用来自动释放内存
static void JS_FreeArrayBuffer(JSRuntime *rt, void *opaque, void *ptr) { free(ptr); }

Js_Api(print, CString text) 
    printf("%s", text);
    JS_FreeCString(ctx, text);
Js_Api_End

Js_Api(input)
    char buffer[512];
    fgets(buffer, 512, stdin);
    return JS_NewString(ctx, buffer);
Js_Api_End

Js_Api(sleep, Uint32 millsecond) 
    Sleep(millsecond);
Js_Api_End

Js_Api(getPid, CString processName) 
    wchar_t wszProcessName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, processName, -1, wszProcessName, MAX_PATH);

    JS_FreeCString(ctx, processName);

    return JS_NewUint32(ctx, Win_GetPid(wszProcessName));
Js_Api_End

Js_Api(getHwnd, Uint32 pid) 
    return JS_NewBigUint64(ctx, (uint64_t)Win_GetHwnd(pid));
Js_Api_End

Js_Api(getWndSize, Uint64 hwnd) 
    SIZE size = Win_GetWndSize((HWND)hwnd);

    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "width", JS_NewInt32(ctx, size.cx));
    JS_SetPropertyStr(ctx, result, "height", JS_NewInt32(ctx, size.cy));

    return result;
Js_Api_End

Js_Api(getDC, Uint64 hwnd)
    return JS_NewBigUint64(ctx, (uint64_t)GetDC((HWND)hwnd));
Js_Api_End

Js_Api(getPixel, Uint64 hdc, Int32 x, Int32 y) 
    return JS_NewUint32(ctx, (uint32_t)GetPixel((HDC)hdc, x, y));
Js_Api_End

Js_Api(isKeyDown, Int32 vKey)
    return JS_NewBool(ctx, GetAsyncKeyState(vKey) & 0x8000);
Js_Api_End

Js_Api(setForegroundWindow, Uint64 hwnd)
    return JS_NewBool(ctx, SetForegroundWindow((HWND)hwnd));
Js_Api_End

Js_Api(postMessageW, Uint64 hwnd, Uint32 msg, Uint64 wParam, Uint64 lParam)
    return JS_NewBool(ctx, PostMessageW((HWND)hwnd, msg, (WPARAM)wParam, (LPARAM)lParam));
Js_Api_End

// releaseDC(hwnd, hdc) 函数
Js_Api(releaseDC, Uint64 hwnd, Uint64 hdc)
    return JS_NewInt32(ctx, ReleaseDC((HWND)hwnd, (HDC)hdc));
Js_Api_End

// ClipCursor(NULL) 函数
Js_Api(releaseCursorClip)
    return JS_NewBool(ctx, ClipCursor(NULL));
Js_Api_End

Js_Api(captureWindow, Uint64 hwnd, Int32 left, Int32 top, Int32 right, Int32 bottom)
    return JS_NewBigUint64(ctx, (uint64_t)Win_CaptureWindow((HWND)hwnd, left, top, right, bottom));
Js_Api_End

// saveImage(image, filepath) 函数
Js_Api(saveImage, Uint64 image, CString filepath)
    wchar_t wczFilepath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wczFilepath, MAX_PATH);
    JS_FreeCString(ctx, filepath);
    return JS_NewBool(ctx, Win_SaveImage((Image)image, wczFilepath));
Js_Api_End

// freeImage(image) 函数
Js_Api(freeImage, Uint64 image)
    Win_FreeImage((Image)image);
Js_Api_End

// mkdir(dirName) 函数
Js_Api(mkdir, CString dirName)
    wchar_t wczDirName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, dirName, -1, wczDirName, MAX_PATH);
    JS_FreeCString(ctx, dirName);
    return JS_NewBool(ctx, CreateDirectoryW(wczDirName, NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
Js_Api_End


// 将以上所有C语言函数绑定到一个JavaScript对象上
void Js_Api_Bind(JSContext* ctx, JSValue obj) {
    for (int i = 0; i < jsApisCount; i++)
        JS_SetPropertyStr(ctx, obj, jsApis[i].name, JS_NewCFunction(ctx, jsApis[i].func, jsApis[i].name, 0));
}


// 执行JavaScript脚本文件
JSValue JS_EvalFile(JSContext* ctx, wchar_t* wszFilepath, int evalFlags) {
    FILE* file; 
    errno_t err = _wfopen_s(&file, wszFilepath, L"rb");

    if(err != 0) {
        char errorMsg[256];
        strerror_s(errorMsg, sizeof(errorMsg), err);
        return JS_ThrowSyntaxError(ctx, "Faild to open '%ls', %s", wszFilepath, errorMsg);
    }
        
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* jsCode = (char*)malloc(fileSize + 1);
    fread(jsCode, fileSize, 1, file);
    jsCode[fileSize] = '\0';

    fclose(file);

    char szFilepath[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, wszFilepath, -1, szFilepath, MAX_PATH, NULL, NULL);

    JSValue result = JS_Eval(ctx, jsCode, fileSize, szFilepath, evalFlags);

    if((evalFlags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        JSValue promiseResult = js_std_await(ctx, result);
        JS_FreeValue(ctx, result);
        result = promiseResult;
    }
    
    free(jsCode);
    return result;
}

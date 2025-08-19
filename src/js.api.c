// 供JavaScript调用的函数
// 函数格式统一为 Js_Api_xxxxx(ctx, thisVal, argc, argv)

#include "js.api.h"
#include <windows.h>
#include <stdio.h>
#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>
#include "win.utils.h"

// 魔法宏：ARG_NUM(...) 展开为传入参数个数, 例如 ARG_NUM(1, 2, 3) -> 3
#define ARG_NUM(...)  ARG_X("ignored", ##__VA_ARGS__, 9,8,7,6,5,4,3,2,1,0)
#define ARG_X(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9, X, ...) X

// CONCAT(a,b) 将a和b拼合起来， 例如 CONCAT(abc, def) -> abcdef
#define CONCAT(a,b)  CONCAT1(a,b)
#define CONCAT1(a,b)  a##b

#define Js_Api_Type_CString const char*
#define Js_Api_Type_Int32 int32_t
#define Js_Api_Type_Uint32 uint32_t
#define Js_Api_Type_Uint64 uint64_t

#define Js_Api_Type_Convert_CString(argIndex) (arg##argIndex = JS_ToCString(ctx, argv[argIndex])) == 0
#define Js_Api_Type_Convert_Int32(argIndex) JS_ToInt32(ctx, &arg##argIndex, argv[argIndex]) != 0
#define Js_Api_Type_Convert_Uint32(argIndex) JS_ToUint32(ctx, &arg##argIndex, argv[argIndex]) != 0
#define Js_Api_Type_Convert_Uint64(argIndex) JS_ToBigInt64(ctx, &arg##argIndex, argv[argIndex]) != 0

#define Js_Api_Name (&(__func__[7]))

#define Js_Api_CheckArgsCount(requiredCount) \
    if (argc < (requiredCount)) \
        return JS_ThrowSyntaxError(ctx, "%s(): Expected %d argument, but received %d", Js_Api_Name, requiredCount, argc);

#define Js_Api_GetArg(argIndex, type) \
    Js_Api_Type_##type arg##argIndex; \
    if(Js_Api_Type_Convert_##type(argIndex)) \
        return JS_ThrowTypeError(ctx, "%s(): The argument%d cannot be converted to " #type, Js_Api_Name, argIndex);

#define Js_Api_GetArgs_0(...)
#define Js_Api_GetArgs_1(t1) Js_Api_GetArg(0,t1)
#define Js_Api_GetArgs_2(t1,t2) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2)
#define Js_Api_GetArgs_3(t1,t2,t3) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2) Js_Api_GetArg(2,t3)
#define Js_Api_GetArgs_4(t1,t2,t3,t4) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2) Js_Api_GetArg(2,t3) Js_Api_GetArg(3,t4)
#define Js_Api_GetArgs_5(t1,t2,t3,t4,t5) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2) Js_Api_GetArg(2,t3) Js_Api_GetArg(3,t4) Js_Api_GetArg(4,t5)

#define Js_Api(apiName, ...) \
    JSValue Js_Api_##apiName(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) { \
        Js_Api_CheckArgsCount(ARG_NUM(__VA_ARGS__)) \
        CONCAT(Js_Api_GetArgs_, ARG_NUM(__VA_ARGS__))(__VA_ARGS__)
        
#define Js_Api_End return JS_NULL;}




// print(text) 函数
Js_Api(print, CString) 
    printf(arg0);
    JS_FreeCString(ctx, arg0);
Js_Api_End

// input() 函数
Js_Api(input)
    char buffer[512];
    fgets(buffer, 512, stdin);
    return JS_NewString(ctx, buffer);
Js_Api_End

// sleep(ms) 函数
Js_Api(sleep, Uint32) 
    Sleep(arg0);
Js_Api_End

// getPid(processName) 函数
Js_Api(getPid, CString) 
    wchar_t wszProcessName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, arg0, -1, wszProcessName, MAX_PATH);

    JS_FreeCString(ctx, arg0);

    return JS_NewUint32(ctx, Win_GetPid(wszProcessName));
Js_Api_End

// getHwnd(pid) 函数
Js_Api(getHwnd, Uint32) 
    return JS_NewBigUint64(ctx, (uint64_t)Win_GetHwnd(arg0));
Js_Api_End

// getWndSize(hwnd) 函数
Js_Api(getWndSize, Uint64) 
    SIZE size = Win_GetWndSize((HWND)arg0);

    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "width", JS_NewInt32(ctx, size.cx));
    JS_SetPropertyStr(ctx, result, "height", JS_NewInt32(ctx, size.cy));

    return result;
Js_Api_End

// getDC(hwnd) 函数
Js_Api(getDC, Uint64)
    return JS_NewBigUint64(ctx, (uint64_t)GetDC((HWND)arg0));
Js_Api_End

// getPixel(hdc, x, y) 函数
Js_Api(getPixel, Uint64, Int32, Int32) 
    return JS_NewUint32(ctx, (uint32_t)GetPixel((HDC)arg0, arg1, arg2));
Js_Api_End

// isKeyDown(vKey) 函数
Js_Api(isKeyDown, Int32)
    return JS_NewBool(ctx, GetAsyncKeyState(arg0) & 0x8000);
Js_Api_End

// setForegroundWindow(hwnd) 函数
Js_Api(setForegroundWindow, Uint64)
    return JS_NewBool(ctx, SetForegroundWindow((HWND)arg0));
Js_Api_End

// postMessageW(hwnd, msg, wParam, lParam) 函数
Js_Api(postMessageW, Uint64, Uint32, Uint64, Uint64)
    return JS_NewBool(ctx, PostMessageW((HWND)arg0, arg1, (WPARAM)arg2, (LPARAM)arg3));
Js_Api_End

// releaseDC(hwnd, hdc) 函数
Js_Api(releaseDC, Uint64, Uint64)
    return JS_NewInt32(ctx, ReleaseDC((HWND)arg0, (HDC)arg1));
Js_Api_End

// ClipCursor(NULL) 函数
Js_Api(releaseCursorClip)
    return JS_NewBool(ctx, ClipCursor(NULL));
Js_Api_End

// captureWindow(hwnd, left, top, right, bottom) 函数
Js_Api(captureWindow, Uint64, Int32, Int32, Int32, Int32)
    return JS_NewBigUint64(ctx, (uint64_t)Win_CaptureWindow((HWND)arg0, arg1, arg2, arg3, arg4));
Js_Api_End

// saveImage(image, filepath) 函数
Js_Api(saveImage, Uint64, CString)
    wchar_t filepath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, arg1, -1, filepath, MAX_PATH);
    JS_FreeCString(ctx, arg1);
    return JS_NewBool(ctx, Win_SaveImage((Image)arg0, filepath));
Js_Api_End

// freeImage(image) 函数
Js_Api(freeImage, Uint64)
    Win_FreeImage((Image)arg0);
Js_Api_End

// mkdir(dirName) 函数
Js_Api(mkdir, CString)
    wchar_t dirName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, arg0, -1, dirName, MAX_PATH);
    JS_FreeCString(ctx, arg0);
    return JS_NewBool(ctx, CreateDirectoryW(dirName, NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
Js_Api_End

// 将以上所有C语言函数绑定到一个JavaScript对象上
void Js_Api_Bind(JSContext* ctx, JSValue obj) {
#define Js_Api_BindFunc(ctx, obj, func, argsCount) \
    JS_SetPropertyStr(ctx, obj, &(#func[6]), JS_NewCFunction(ctx, func, &(#func[6]), argsCount))

    Js_Api_BindFunc(ctx, obj, Js_Api_print, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_input, 0);
    Js_Api_BindFunc(ctx, obj, Js_Api_sleep, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_getPid, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_getHwnd, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_getWndSize, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_getDC, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_getPixel, 3);
    Js_Api_BindFunc(ctx, obj, Js_Api_isKeyDown, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_setForegroundWindow, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_postMessageW, 4);
    Js_Api_BindFunc(ctx, obj, Js_Api_releaseDC, 2);
    Js_Api_BindFunc(ctx, obj, Js_Api_releaseCursorClip, 0);
    Js_Api_BindFunc(ctx, obj, Js_Api_captureWindow, 5);
    Js_Api_BindFunc(ctx, obj, Js_Api_saveImage, 2);
    Js_Api_BindFunc(ctx, obj, Js_Api_freeImage, 1);
    Js_Api_BindFunc(ctx, obj, Js_Api_mkdir, 1);
}


// 执行JavaScript脚本文件
JSValue inline JS_EvalFile(JSContext* ctx, wchar_t* wszFilepath, int evalFlags) {
    FILE* file = _wfopen(wszFilepath, L"rb");
    if(file == NULL)
        return JS_ThrowSyntaxError(ctx, "Faild to open '%ls'.", wszFilepath);
        
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

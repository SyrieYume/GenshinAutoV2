#include <windows.h>
#include <quickjs/quickjs.h>
#include "console.utils.h"
#include "win.utils.h"
#include "js.api.h"

int main(int argc, char **argv) {
    if(!Con_Init("GenshinAuto v2", 100, 30)) {
        MessageBoxW(NULL, L"初始化控制台窗口时出错", L"Error", MB_OK | MB_TOPMOST);
        return 0;
    }

    Con_Info("GenshinAuto " ANSI_BLUE("v2.3"));

    // 创建js运行时和context
    JSRuntime *jsRuntime = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(jsRuntime);

    // 获取全局对象
    JSValue jsGlobal = JS_GetGlobalObject(ctx);

    // 将 js.api.c 中的所有C语言函数注册到JavaScript的全局对象上
    Js_Api_Bind(ctx, jsGlobal);

    wchar_t* jsFilePaths[2] = { L"./utils.js", L"./script.js" };

    for (int i = 0; i < 2; i++) {
        wchar_t* jsFilePath = jsFilePaths[i];

        Win_LoadResourceToFile(101 + i, jsFilePath);

        Con_Info("加载脚本：" ANSI_BLUE("%ls"), jsFilePath);
        
        JSValue result = JS_EvalFile(ctx, jsFilePath, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_STRICT);

        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(ctx);
            const char* exceptionText = JS_ToCString(ctx, exception);

            Con_Error("%s", exceptionText);

            JS_FreeCString(ctx, exceptionText);
            JS_FreeValue(ctx, exception);
            JS_FreeValue(ctx, result);
            break;
        }

        JS_FreeValue(ctx, result);
    }

    JS_FreeContext(ctx);
    JS_FreeRuntime(jsRuntime);

    system("pause");
    return 0;
}
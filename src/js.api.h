#pragma once
#include <quickjs/quickjs.h>

// 将 js.api.c 中的所有C语言函数绑定到一个JavaScript对象上
void Js_Api_Bind(JSContext* ctx, JSValue obj);

// 执行JavaScript脚本文件
JSValue JS_EvalFile(JSContext* ctx, wchar_t* wszFilepath, int evalFlags);
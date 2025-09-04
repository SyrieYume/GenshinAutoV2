// 用来方便定义JS API的工具宏
#pragma once
#include <quickjs/quickjs.h>

// 魔法宏：ARG_NUM(...) 展开为传入参数个数, 例如 ARG_NUM(1, 2, 3) -> 3
#define ARG_NUM(...)  ARG_X("ignored", ##__VA_ARGS__, 9,8,7,6,5,4,3,2,1,0)
#define ARG_X(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9, X, ...) X

// CONCAT(a,b) 将a和b拼合起来， 例如 CONCAT(abc, def) -> abcdef
#define CONCAT(a,b)  CONCAT1(a,b)
#define CONCAT1(a,b)  a##b

#define Js_Api_Type_CString CString, const char*,
#define Js_Api_Type_Int32 Int32, int32_t,
#define Js_Api_Type_Uint32 Uint32, uint32_t,
#define Js_Api_Type_Uint64 Uint64, uint64_t,

#define Js_Api_Type_Convert_CString(argIndex, name) (name = JS_ToCString(ctx, argv[argIndex])) == 0
#define Js_Api_Type_Convert_Int32(argIndex, name) JS_ToInt32(ctx, &name, argv[argIndex]) != 0
#define Js_Api_Type_Convert_Uint32(argIndex, name) JS_ToUint32(ctx, &name, argv[argIndex]) != 0
#define Js_Api_Type_Convert_Uint64(argIndex, name) JS_ToBigInt64(ctx, (int64_t*)&name, argv[argIndex]) != 0

#define Js_Api_Name (&(__func__[7]))

#define Js_Api_Register(apiName) \
    static  __attribute__((constructor)) void register_##apiName() { \
        jsApis[jsApisCount].name = "_" #apiName; \
        jsApis[jsApisCount++].func = Js_Api_##apiName; \
    }

#define Js_Api_CheckArgsCount(requiredCount) \
    if (argc < (requiredCount)) \
        return JS_ThrowSyntaxError(ctx, "%s(): Expected %d argument, but received %d", Js_Api_Name, requiredCount, argc);

#define Js_Api_GetArg(argIndex, type_name) Js_Api_GetArg1(argIndex, Js_Api_Type_##type_name)
#define Js_Api_GetArg1(...)  Js_Api_GetArg4(__VA_ARGS__)
#define Js_Api_GetArg4(argIndex, typeName, targetType, name) \
    targetType name; \
    if(Js_Api_Type_Convert_##typeName(argIndex, name)) \
        return JS_ThrowTypeError(ctx, "%s(): The argument%d cannot be converted to " #typeName, Js_Api_Name, argIndex);

#define Js_Api_GetArgs_0(...)
#define Js_Api_GetArgs_1(t1) Js_Api_GetArg(0,t1)
#define Js_Api_GetArgs_2(t1,t2) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2)
#define Js_Api_GetArgs_3(t1,t2,t3) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2) Js_Api_GetArg(2,t3)
#define Js_Api_GetArgs_4(t1,t2,t3,t4) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2) Js_Api_GetArg(2,t3) Js_Api_GetArg(3,t4)
#define Js_Api_GetArgs_5(t1,t2,t3,t4,t5) Js_Api_GetArg(0,t1) Js_Api_GetArg(1,t2) Js_Api_GetArg(2,t3) Js_Api_GetArg(3,t4) Js_Api_GetArg(4,t5)

#define Js_Api(apiName, ...) \
    JSValue Js_Api_##apiName(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv); \
    Js_Api_Register(apiName) \
    JSValue Js_Api_##apiName(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) { \
        Js_Api_CheckArgsCount(ARG_NUM(__VA_ARGS__)) \
        CONCAT(Js_Api_GetArgs_, ARG_NUM(__VA_ARGS__))(__VA_ARGS__)
        
#define Js_Api_End return JS_NULL;}
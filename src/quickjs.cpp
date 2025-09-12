module;

#include <type_traits>
#include <filesystem>
#include <functional>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <thread>
#include <chrono>
#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>
#include <windows.h>

export module quickjs;

export namespace qjs {
    class Runtime;
    class Context;
    class Shared_Value;
    class Value;
}

struct Utilities { 
    // 计时器数据类型，用于实现setTimeout
    struct TimeoutTimer {
        unsigned int id;
        std::chrono::steady_clock::time_point timeout;
        JSValue func;

        bool operator < (const TimeoutTimer& other) const { return timeout > other.timeout; } 
    };

    static JSValue setTimeout(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv);

    // 将 JS列表 转换为 std::tuple
    template <typename Tuple, std::size_t... I>
    static Tuple jsList_to_tuple(JSContext* ctx, const JSValue& val, std::index_sequence<I...>);

    // 将 std::tuple 转换为 JSObject
    template <typename Tuple, std::size_t... I>
    static JSValue tuple_to_jsObject(JSContext* ctx, const Tuple& val, std::index_sequence<I...>);

    // 将JSValue转换为任意类型
    template <typename T>
    static T convert_from_js(JSContext* ctx, const JSValue& value);

    // 将任意类型转换为JSValue
    template <typename T>
    static JSValue convert_to_js(JSContext* ctx, const T& value);

    // 将 JSValue[] 转换为Func函数的对应参数类型，并调用Func
    template <auto Func, size_t... I>
    static JSValue call_with_js_args(JSContext* ctx, JSValueConst* argv, std::index_sequence<I...>);


    // 通过模板获取函数的参数和返回值类型
    template <typename T>
    struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<R(*)(Args...)> {
        using return_type = R;
        using args_tuple = std::tuple<Args...>;
        using args_count = std::tuple_size<args_tuple>;

        template <size_t i>
        using arg_type = std::tuple_element_t<i, args_tuple>;
    };

    template <typename Lambda>
    struct function_traits : public function_traits<decltype(+std::declval<Lambda>())> {};


    // 通过模板判断 tuple 是否符合 std::tuple<std::pair<const char*, AnyType>> 类型
    template <typename T>
    struct is_tuple : std::false_type {};

    template <typename... Args>
    struct is_tuple<std::tuple<Args...>> : std::true_type {};

    template <typename T>
    struct is_key_value_pair : std::false_type {};

    template <typename V>
    struct is_key_value_pair<std::pair<const char*, V>> : std::true_type {};

    template <typename T>
    struct is_key_values_tuple : std::false_type {};

    template <typename FirstElement, typename... Rest>
    struct is_key_values_tuple<std::tuple<FirstElement, Rest...>> : is_key_value_pair<FirstElement> {};
};



class qjs::Shared_Value {
protected:
    JSValue value;
    JSContext* ctx;
public:
    Shared_Value(JSContext* _ctx, JSValue _value): ctx(_ctx), value(_value) {}

    void setProperty(const std::string& name, auto val) {
        JS_SetPropertyStr(ctx, value, name.c_str(), Utilities::convert_to_js(ctx, val));
    }

    template <typename T>
    T getProperty(const std::string& name) {
        return convert_from_js<T>(ctx, JS_GetPropertyStr(ctx, value, name.c_str()));
    }

    template <auto Func>
    Shared_Value& func(const std::string& name);

    std::string toString() { return Utilities::convert_from_js<std::string>(ctx, value); }
};



class qjs::Value: public qjs::Shared_Value {
protected:
    Value(JSContext* ctx, JSValue val) : Shared_Value(ctx, val) {}

public:
    ~Value() { JS_FreeValue(ctx, value); }

    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;

    Value(Value&& other) noexcept: Value(other.ctx, other.value) {
        other.ctx = nullptr;
        other.value = JS_UNDEFINED;
    }

    Value& operator=(Value&& other) noexcept {
        if (this != &other) {
            JS_FreeValue(ctx, value);
            ctx = other.ctx;
            value = other.value;
            other.ctx = nullptr;
            other.value = JS_UNDEFINED;
        }
        return *this;
    }

friend class Context;
friend class Runtime;
};



class qjs::Context {
private:
    JSContext* ctx;

    Context(JSRuntime* rt);
public:
    std::vector<std::function<void(const char*)>> onJsFileLoaded {};

    Value eval(const std::string& input, const std::string& filename="<eval>", int evalFlags=JS_EVAL_TYPE_GLOBAL);

    Value evalFile(std::filesystem::path filePath, int evalFlags=JS_EVAL_TYPE_MODULE);

    void loop();

    Value getGlobal() { return Value(ctx, JS_GetGlobalObject(ctx)); }

    std::string getException();

    ~Context() { JS_FreeContext(ctx); }
    
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

friend Runtime;
friend Utilities;
};



class qjs::Runtime {
private: 
    JSRuntime* runtime;
    std::filesystem::path baseDir;
    std::priority_queue<Utilities::TimeoutTimer> timers {};
    unsigned int nextTimerId = 0;

public: 
    Runtime(std::filesystem::path _baseDir);

    Runtime(): Runtime(std::filesystem::current_path()) {};

    Context createContext() { return Context(runtime); }

    ~Runtime() { JS_FreeRuntime(runtime); }
    
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

friend Utilities;
friend Context;
};



qjs::Runtime::Runtime(std::filesystem::path _baseDir): runtime(JS_NewRuntime()), baseDir(_baseDir) {
    if(!runtime) 
        throw std::runtime_error("Failed to create Quickjs Runtime.");
    JS_SetRuntimeOpaque(runtime, this);

    JS_SetModuleLoaderFunc2(runtime, 
        [](JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque){
            Runtime* rt = reinterpret_cast<Runtime*>(opaque);

            auto fullPath = rt->baseDir / std::filesystem::path(module_base_name).parent_path() / module_name;
            auto relativePath = std::filesystem::relative(std::filesystem::weakly_canonical(fullPath), rt->baseDir);
            auto szRelativePath = relativePath.generic_string();

            char* normalizedPath = reinterpret_cast<char*>(js_malloc(ctx,  szRelativePath.size() + 1));
            memcpy(normalizedPath, szRelativePath.c_str(), szRelativePath.size());
            normalizedPath[szRelativePath.size()] = '\0';
            return normalizedPath;
        },
        [](JSContext* ctx, const char* module_name, void* opaque, JSValueConst attributes) {
            Context* context = reinterpret_cast<Context*>(JS_GetContextOpaque(ctx));
            Runtime* rt = reinterpret_cast<Runtime*>(opaque);
            
            Value val = context->evalFile(std::filesystem::path(module_name), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
            js_module_set_import_meta(ctx, val.value, TRUE, FALSE);

            return reinterpret_cast<JSModuleDef*>(JS_VALUE_GET_PTR(val.value));
        }, js_module_check_attributes, this);
}


qjs::Context::Context(JSRuntime* rt): ctx(JS_NewContext(rt)) {
    if(!ctx) 
        throw std::runtime_error("Failed to create Quickjs Context.");
    JS_SetContextOpaque(ctx, this);
    getGlobal().setProperty("setTimeout", JS_NewCFunction(ctx, Utilities::setTimeout, "setTimeout", 2));
}


std::string qjs::Context::getException() {
    Value exception(ctx, JS_GetException(ctx));
    std::string errorMsg = exception.toString();

    Value stackVal(ctx, JS_GetPropertyStr(ctx, exception.value, "stack"));
    if (!JS_IsUndefined(stackVal.value)) {
        std::string stackStr = stackVal.toString();
        errorMsg += "\nTraceback:\n";
        errorMsg += stackStr;
    }

    return errorMsg;
}


// 执行js代码
qjs::Value qjs::Context::eval(const std::string& input, const std::string& filename, int evalFlags) {
    Value result(ctx, JS_Eval(ctx, input.c_str(), input.length(), filename.c_str(), evalFlags));

    if(JS_IsException(result.value))
        throw std::runtime_error(this->getException());

    return result;
}


// 执行js脚本文件
qjs::Value qjs::Context::evalFile(std::filesystem::path filePath, int evalFlags) {
    JSRuntime* _rt = JS_GetRuntime(ctx);
    qjs::Runtime* rt = reinterpret_cast<qjs::Runtime*>(JS_GetRuntimeOpaque(_rt));

    auto fullPath = rt->baseDir / filePath;
    filePath = std::filesystem::relative(std::filesystem::weakly_canonical(fullPath), rt->baseDir);
    std::string szFilePath = filePath.generic_string();

    for(const auto& callback: onJsFileLoaded)
        callback(szFilePath.c_str());

    std::ifstream file(fullPath, std::ios::binary);
    if(!file.is_open())
        throw std::runtime_error(("Faild to Open file '" + szFilePath + '\''));

    std::string jsCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    return eval(jsCode, szFilePath, evalFlags);
}


void qjs::Context::loop() {
    JSRuntime* _rt = JS_GetRuntime(ctx);
    Runtime* rt = reinterpret_cast<Runtime*>(JS_GetRuntimeOpaque(_rt));
    
    while(true) {
        while(true) {
            int err = JS_ExecutePendingJob(_rt, NULL);
            if(err <= 0) {
                if(err < 0)
                    throw std::runtime_error(this->getException());
                break;
            }
        }

        if(rt->timers.size() == 0)
            break;

        auto nowTime = std::chrono::steady_clock::now();
        auto& topTimer = rt->timers.top();

        if(nowTime >= topTimer.timeout) {
            Value ret(ctx, JS_Call(ctx, topTimer.func, JS_UNDEFINED, 0, NULL));

            if (JS_IsException(ret.value))
                throw std::runtime_error(this->getException());
            
            JS_FreeValue(ctx, topTimer.func);
            rt->timers.pop();
        } else {
            std::this_thread::sleep_for(topTimer.timeout - nowTime);
        }
    }
}


JSValue Utilities::setTimeout(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    JSRuntime* _rt = JS_GetRuntime(ctx);
    qjs::Runtime* rt = reinterpret_cast<qjs::Runtime*>(JS_GetRuntimeOpaque(_rt));

    if (argc != 2)
        return JS_ThrowSyntaxError(ctx, "Expected 2 argument, but received %d", argc);

    if(!JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "Argument1 is not a Function");

    int delay = 0;
    if(JS_ToInt32(ctx, &delay, argv[1]))
        return JS_ThrowTypeError(ctx, "Argument2 is not a Number");

    TimeoutTimer timer = {
        rt->nextTimerId++,
        std::chrono::steady_clock::now() + std::chrono::milliseconds(delay),
        JS_DupValue(ctx, argv[0])
    };

    rt->timers.push(timer);

    return JS_NewUint32(ctx, timer.id);
}


// quickjs的头文件中大量使用static inline函数，在导出的模板元函数中直接使用这些函数会出问题
const char* qjs_ToCString(JSContext *ctx, JSValue val1) { return JS_ToCString(ctx, val1); }
int qjs_ToUint32(JSContext *ctx, uint32_t *pres, JSValue val) { return JS_ToUint32(ctx, pres, val); }
JSValue qjs_NewBool(JSContext *ctx, int val) { return JS_NewBool(ctx, val); }
JSValue qjs_NewInt32(JSContext *ctx, int32_t val) { return JS_NewInt32(ctx, val); }
JSValue qjs_NewUint32(JSContext *ctx, uint32_t val) { return JS_NewUint32(ctx, val); }
JSValue qjs_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name, int length) {
    return JS_NewCFunction(ctx, func, name, length);
}


// 将 JS列表 转换为 std::tuple
template <typename Tuple, std::size_t... I>
Tuple Utilities::jsList_to_tuple(JSContext* ctx, const JSValue& val, std::index_sequence<I...>) {
    return std::make_tuple(convert_from_js<std::tuple_element_t<I, Tuple>>(
        ctx, JS_GetPropertyUint32(ctx, val, I)
    )...);
}


// 将 std::tuple 转换为 JSObject
template <typename Tuple, std::size_t... I>
JSValue Utilities::tuple_to_jsObject(JSContext* ctx, const Tuple& val, std::index_sequence<I...>) {
    JSValue obj = JS_NewObject(ctx);
    if constexpr (is_key_values_tuple<Tuple>::value)
        (JS_SetPropertyStr(ctx, obj, std::get<I>(val).first, convert_to_js(ctx, std::get<I>(val).second)), ...);
    else (JS_SetPropertyUint32(ctx, obj, I, convert_to_js(ctx, std::get<I>(val))), ...);
    return obj;
}


// JSValue 转换为任意类型
template <typename Type>
Type Utilities::convert_from_js(JSContext* ctx, const JSValue& val) {
    using T = std::decay_t<Type>;

    if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
        return qjs_ToCString(ctx, val);

    else if constexpr (std::is_same_v<T, std::string>) {
        const char* cstr = qjs_ToCString(ctx, val);
        if (!cstr) return std::string();
        std::string result(cstr);
        JS_FreeCString(ctx, cstr);
        return result;
    }

    else if constexpr (std::is_same_v<T, std::byte*>) {
        size_t bufferSize;
        return reinterpret_cast<T>(JS_GetArrayBuffer(ctx, &bufferSize, val));
    }

    else if constexpr (is_tuple<T>::value)
        return jsList_to_tuple<T>(ctx, val, std::make_index_sequence<std::tuple_size_v<T>>{});

    Type value;

    if constexpr (sizeof(T) == 4) {
        if constexpr (std::is_signed_v<T>)
            JS_ToInt32(ctx, reinterpret_cast<int32_t*>(&value), val);
        else qjs_ToUint32(ctx, reinterpret_cast<uint32_t*>(&value), val);
    }

    else if constexpr (std::is_pointer_v<T> || sizeof(T) == 8)
        JS_ToBigInt64(ctx, reinterpret_cast<int64_t*>(&value), val);

    else if constexpr (sizeof(T) < 4) {
        int _value;
        JS_ToInt32(ctx, &_value, val);
        value = static_cast<Type>(_value);
    }
     
    return value;
}


// 任意类型转换为 JSValue
template <typename Type>
JSValue Utilities::convert_to_js(JSContext *ctx, const Type& val) {
    using T = std::decay_t<Type>;

    if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
        return JS_NewStringLen(ctx, val, strlen(val));
    
    else if constexpr (std::is_same_v<T, std::string>)
        return JS_NewStringLen(ctx, val.c_str(), val.length());
    
    else if constexpr (std::is_same_v<T, bool>)
        return qjs_NewBool(ctx, static_cast<int>(val));
    
    else if constexpr (is_tuple<T>::value)
        return tuple_to_jsObject(ctx, val, std::make_index_sequence<std::tuple_size_v<T>>{});
    
    else if constexpr (std::is_same_v<T, std::pair<std::byte*, size_t>>) {
        return JS_NewArrayBuffer(ctx, reinterpret_cast<uint8_t*>(val.first), val.second, 
            [](JSRuntime* rt, void* opaque, void* ptr) { 
                delete[] reinterpret_cast<std::byte*>(ptr);
            }, nullptr, 1);
    }

    else if constexpr (sizeof(T) <= 4) {
        if constexpr (std::is_signed_v<T>)
            return qjs_NewInt32(ctx, val);
        else return qjs_NewUint32(ctx, val);
    }

    else if constexpr (std::is_pointer_v<T> || sizeof(T) == 8)
        return JS_NewBigUint64(ctx, reinterpret_cast<uint64_t>(val));
    
    else if constexpr (std::is_same_v<T, JSValue>)
        return val;
}


// 将 argv[] 转换为Func函数的对应参数类型，并调用Func
template <auto Func, size_t... I>
JSValue Utilities::call_with_js_args(JSContext* ctx, JSValueConst* argv, std::index_sequence<I...>) {
    using traits = function_traits<decltype(Func)>;
    
    std::tuple<typename traits::template arg_type<I>...> args_tuple =
        std::make_tuple(convert_from_js<typename traits::template arg_type<I>>(ctx, argv[I])...);

    JSValue result;
    
    if constexpr (std::is_same_v<typename traits::return_type, void>) {
        std::apply(Func, args_tuple);
        result = JS_UNDEFINED;
    } else {
        auto _result = std::apply(Func, args_tuple);
        result = convert_to_js(ctx, _result);
    }

    ([&]() {
        if constexpr (std::is_same_v<typename traits::template arg_type<I>, const char*>)
            JS_FreeCString(ctx, std::get<I>(args_tuple));
    }(), ...);

    return result;
}


// 将一个 C/C++ 函数封装为quickjs可用的函数，并且绑定到JS对象上
template <auto Func>
qjs::Shared_Value& qjs::Shared_Value::func(const std::string& name) {
    using traits = Utilities::function_traits<decltype(Func)>;

    JS_SetPropertyStr(ctx, value, name.c_str(), qjs_NewCFunction(ctx, 
        [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
            if (argc != traits::args_count::value)
                return JS_ThrowSyntaxError(ctx, "Expected %d argument, but received %d", (unsigned)traits::args_count::value, argc);
            
            return Utilities::call_with_js_args<Func>(ctx, argv, std::make_index_sequence<traits::args_count::value>{});
    }, name.c_str(), 0));

    return *this;
}


void test() {
    qjs::Runtime jsRuntime;
    qjs::Context context = jsRuntime.createContext();
    qjs::Value global = context.getGlobal();

    global.func<[](const char* savepath, std::byte* data, int width, int height, int step) {
        return std::make_tuple(
            std::make_pair("width", 1280),
            std::make_pair("height", 720)
        );
    }>("_test");

    qjs::Value result = context.eval("_test('1245')");
}
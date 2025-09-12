#include <string>
#include <format>
#include <filesystem>
#include <windows.h>

import console;
import quickjs;
import win;

auto bindGlobalFunctions(qjs::Value& globalObject) -> void;
 

auto main() -> int {
    try {
        console::init("GenshinAuto V2", 100, 30);
        console::info(std::format("GenshinAuto {}v2.4{}", console::ansi::blue, console::ansi::reset));

        std::filesystem::path baseDir = win::getBaseDir();
        win::loadResourceToFile(101, baseDir / "api.js");
        win::loadResourceToFile(102, baseDir / "script.js");

        qjs::Runtime jsRuntime(baseDir);
        qjs::Context context = jsRuntime.createContext();

        qjs::Value global = context.getGlobal();

        bindGlobalFunctions(global);
        
        context.onJsFileLoaded.push_back([](const char* filePath){
            console::info(std::format("加载脚本: {}{}{}", console::ansi::blue, filePath, console::ansi::reset));
        });

        context.evalFile("./script.js");

        context.loop();
    }
    catch(const std::exception& e) {
        console::error(e.what());
    }

    system("pause");
    return 0;
}



auto bindGlobalFunctions(qjs::Value& globalObject) -> void { 
globalObject
    .func<win::getPid>("_getPid")
    .func<win::getHwnd>("_getHwnd")
    .func<win::getWndSize>("_getWndSize")
    .func<win::captureWindow>("_captureWindow")
    .func<win::saveBitmapImage>("_saveBitmapImage")
    .func<console::print>("_print")

    .func<Sleep>("_sleep")
    .func<GetDC>("_getDC")
    .func<GetPixel>("_getPixel")
    .func<PostMessageW>("_postMessageW")
    .func<ReleaseDC>("_releaseDC")
    .func<ClipCursor>("_clipCursor")
    .func<SetForegroundWindow>("_setForegroundWindow")
    .func<keybd_event>("_keybdEvent")
    .func<mouse_event>("_mouseEvent")
    .func<SetCursorPos>("_setCursorPos")

    .func<[]() {
        static char buffer[256];
        fgets(buffer, sizeof(buffer), stdin);
        return buffer;
    }>("_input")

    .func<[](const char* dirName) {
        return std::filesystem::create_directories(dirName);
    }>("_mkdir")
    
    .func<[](int vKey) { 
        return (GetAsyncKeyState(vKey) & 0x8000) != 0; 
    }>("_isKeyDown");
}
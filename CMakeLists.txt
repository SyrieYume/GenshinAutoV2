# cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DDYNAMIC_LINK_QUICKJS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
# cmake --build build

cmake_minimum_required(VERSION 3.30.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

project(GenshinAutoV2 LANGUAGES C CXX)

# 设置输出目录
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

# 设置默认的构建类型
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No Build Type selected, Default to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 启用链接时优化
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories("${CMAKE_SOURCE_DIR}")

add_executable(GenshinAutoV2)


# MinGW gcc 和 LLVM-MinGW clang 可以编译并静态链接quickjs
if(NOT DYNAMIC_LINK_QUICKJS)
    add_library(pthread-for-win32 STATIC "./posix/pthread-for-win32.c")

    # 获取quickjs的版本号
    file(STRINGS "./quickjs/VERSION" QUICKJS_VERSION_CONTENT)
    list(GET QUICKJS_VERSION_CONTENT 0 QUICKJS_VERSION)
    message(STATUS "Quickjs Version: ${QUICKJS_VERSION}")
    
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_STATIC_LIBRARY_PREFIX "")

    # 编译 quickjs (将 STATIC 改为 SHARED 可以变为动态链接 quickjs.dll)
    add_library(quickjs STATIC
        "./quickjs/cutils.c"
        "./quickjs/libregexp.c"
        "./quickjs/libunicode.c"
        "./quickjs/dtoa.c"
        "./quickjs/quickjs.c"
        "./quickjs/quickjs-libc.c"
    )

    target_compile_definitions(quickjs PRIVATE 
        CONFIG_VERSION="${QUICKJS_VERSION}"
        CONFIG_WIN32 
        __USE_MINGW_ANSI_STDIO
    )

    target_compile_options(quickjs PRIVATE -O2)

    target_link_libraries(quickjs PRIVATE pthread-for-win32 m)

    target_link_options(quickjs PRIVATE -static-libgcc)

# LLVM-MSVC clang 只能动态链接已编译好的 quickjs.dll
else()
    target_link_directories(GenshinAutoV2 PRIVATE "${CMAKE_SOURCE_DIR}/lib")
    configure_file("./lib/quickjs.dll" "${OUTPUT_DIR}/quickjs.dll" COPYONLY)
endif()


target_sources(GenshinAutoV2 PRIVATE
    "./src/main.cpp"
    "./res/res.rc"
)

target_sources(GenshinAutoV2 PRIVATE FILE_SET CXX_MODULES FILES
    "./src/console.cpp"
    "./src/quickjs.cpp"
    "./src/win.utils.cpp"
)

target_link_libraries(GenshinAutoV2 PRIVATE quickjs gdi32)

target_compile_options(GenshinAutoV2 PRIVATE -Wno-unused-value)


# 静态链接 C++运行时库
if(NOT (CMAKE_BUILD_TYPE MATCHES "Debug"))
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_link_options(GenshinAutoV2 PRIVATE -static)
    endif()
endif()


# 将 utils.js 和 script.js 复制到release目录下
# configure_file("./src/script.js" "${OUTPUT_DIR}/script.js" COPYONLY)
# configure_file("./src/api.js" "${OUTPUT_DIR}/api.js" COPYONLY)
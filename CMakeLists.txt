cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(GenshinAutoV2 LANGUAGES C)

# 设置输出目录
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

# 设置默认的构建类型
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No Build Type selected, Default to Release")
    set(CMAKE_BUILD_TYPE Release)
endif()

# 启用链接时优化
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories("${CMAKE_SOURCE_DIR}")


# 编译程序 GenshinAutoV2.exe
add_executable(GenshinAutoV2 WIN32)

target_sources(GenshinAutoV2 PRIVATE 
    "./src/main.c"
    "./src/console.utils.c"
    "./src/win.utils.c"
    "./src/js.api.c"
    "./res/res.rc"
)

target_link_libraries(GenshinAutoV2 gdi32)

target_compile_options(GenshinAutoV2 PRIVATE -Wno-unused-value -Wno-pointer-to-int-cast)

# MinGW gcc 可以编译并静态链接quickjs
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # 编译 pthread-for-win32
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

    target_link_libraries(quickjs pthread-for-win32 m)

    target_link_options(quickjs PRIVATE -static-libgcc)

    target_link_libraries(GenshinAutoV2 quickjs)
endif()


# clang只能动态链接已编译好的 quickjs.dll
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_link_directories(GenshinAutoV2 PRIVATE lib)
    target_link_libraries(GenshinAutoV2 quickjs)
    configure_file("./lib/quickjs.dll" "${OUTPUT_DIR}/quickjs.dll" COPYONLY)
endif()


if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_link_options(GenshinAutoV2 PRIVATE
        "-Wl,/manifest:no" 
        "-Wl,/manifestuac:level='requireAdministrator' uiAccess='false'" 
    )
endif()

# 将 utils.js 和 script.js 复制到release目录下
configure_file("./src/script.js" "${OUTPUT_DIR}/script.js" COPYONLY)
configure_file("./src/utils.js" "${OUTPUT_DIR}/utils.js" COPYONLY)
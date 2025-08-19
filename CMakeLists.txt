cmake_minimum_required(VERSION 3.10)

project(GenshinAutoV2 LANGUAGES C)

# 设置编译的输出目录
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})


# 设置默认的构建类型
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No Build Type selected, Default to MinSizeRel")
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()


# 获取quickjs的版本号
file(STRINGS "./quickjs/VERSION" QUICKJS_VERSION_CONTENT)
list(GET QUICKJS_VERSION_CONTENT 0 QUICKJS_VERSION)
message(STATUS "Quickjs Version: ${QUICKJS_VERSION}")


# 编译 pthread-for-win32
add_library(pthread-for-win32 STATIC "./pthread/pthread-for-win32.c")


# 编译 quickjs
add_library(quickjs STATIC
    "./quickjs/cutils.c"
    "./quickjs/libregexp.c"
    "./quickjs/libunicode.c"
    "./quickjs/dtoa.c"
    "./quickjs/quickjs.c"
    "./quickjs/quickjs-libc.c"
)

target_link_libraries(quickjs pthread-for-win32 m)

target_compile_definitions(quickjs PRIVATE 
    CONFIG_VERSION="${QUICKJS_VERSION}"
    CONFIG_WIN32 
    __USE_MINGW_ANSI_STDIO
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_link_options(quickjs PRIVATE "-static-libgcc")
endif()


# 编译程序 GenshinAutoV2.exe
add_executable(GenshinAutoV2)

target_sources(GenshinAutoV2 PRIVATE 
    "./src/main.c"
    "./src/console.utils.c"
    "./src/win.utils.c"
    "./src/js.api.c"
)

target_include_directories(GenshinAutoV2 PRIVATE "${CMAKE_SOURCE_DIR}")

target_link_libraries(GenshinAutoV2 quickjs gdi32)

target_link_options(GenshinAutoV2 PRIVATE "-mwindows")

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # 打包资源文件
    add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/res/res.o"
        COMMAND windres ${CMAKE_SOURCE_DIR}/res/res.rc -O coff -o ${CMAKE_SOURCE_DIR}/res/res.o
        DEPENDS "${CMAKE_SOURCE_DIR}/res/res.rc" "${CMAKE_SOURCE_DIR}/src/utils.js" "${CMAKE_SOURCE_DIR}/src/script.js"
        COMMENT "Packaging resource files"
    )

    target_sources(GenshinAutoV2 PRIVATE "./res/res.o")
endif()


# 将 utils.js 和 script.js 复制到release目录下
# configure_file("./src/script.js" "${OUTPUT_DIR}/script.js" COPYONLY)
# configure_file("./src/utils.js" "${OUTPUT_DIR}/utils.js" COPYONLY)
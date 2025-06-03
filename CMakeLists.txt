cmake_minimum_required(VERSION 3.22)
project(GenshinAutoV2 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows -lgdi32")

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/res.o
        COMMAND windres ${CMAKE_SOURCE_DIR}/res/res.rc -O coff -o ${CMAKE_BINARY_DIR}/res.o
        DEPENDS ${CMAKE_SOURCE_DIR}/res/res.rc
        COMMENT "Compiling resource file"
)

file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.c)

add_executable(GenshinAutoV2 ${SRC_FILES} ${CMAKE_BINARY_DIR}/res.o)

target_link_libraries(GenshinAutoV2 gdi32)

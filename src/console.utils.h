// 一些 控制台 的工具函数
#pragma once
#include <windows.h>
#include <stdio.h>

// ANSI转义序列
#define ANSI_RGB(R, G, B) "\033[38;2;" #R ";" #G ";" #B "m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RESET "\033[0m"
#define ANSI_CLEAN_LINE "\r\033[K"

#define ANSI_GREEN(text)   ANSI_RGB(96, 200, 135)  text ANSI_RESET   // oklch(75.35% 0.1348 154.18)
#define ANSI_ORANGE(text)  ANSI_RGB(241, 149, 88)  text ANSI_RESET   // oklch(75.35% 0.1348 52.6)
#define ANSI_BLUE(text)    ANSI_RGB(76, 186, 250)  text ANSI_RESET   // oklch(75.35% 0.1348 237.83)
#define ANSI_RED(text)     ANSI_RGB(247, 101, 104) text ANSI_RESET   // oklch(75.35% 0.1348 0)


#define Con_Info(message, ...) printf(ANSI_BOLD ANSI_GREEN("[Info] ") message "\n", ##__VA_ARGS__)
#define Con_Error(message, ...) printf(ANSI_BOLD ANSI_RED("[Error] ") message "\n", ##__VA_ARGS__)
#define Con_Waiting(message, ...) printf(ANSI_BOLD ANSI_ORANGE("[Waiting] ") message, ##__VA_ARGS__)



// 初始化控制台
BOOL Con_Init(const char* title, int cols, int lines);
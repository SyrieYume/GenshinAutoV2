# GenshinAutoV2
原神后台自动点剧情工具 v2版本。

## 注意事项
1. 在启动该工具时，除了游戏进程外，不要运行其它名为 "YuanShen.exe" 或者 "GenshinImpact.exe" 的进程。
2. 原神的窗口大小需要 大于或等于 800 X 600，原神 `图像设置` 中的 `显示模式` 不能选择 `独占全屏`。
3. 原神的窗口大小改变后，需要重启该工具。
4. 在原神窗口化运行时，如果鼠标无法锁定在游戏中，只需要在游戏中按一下 `Alt` 键，就可以让鼠标重新锁定在游戏中。

## 如何编译本项目
使用的编译器为 **MinGW** (gcc version 14.2.0)
```bash
windres ./res/res.rc res.o
gcc ./src/*.c res.o -o ./GenshinAutoV2.exe -lgdi32 -mwindows
rm res.o
```


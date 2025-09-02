# GenshinAutoV2
原神后台自动点剧情工具 v2版本

## 使用方法
1. 从 [Releases](https://github.com/SyrieYume/GenshinAutoV2/releases/latest) 下载最新版的 `GenshinAutoV2.exe`，放到一个单独的文件夹中 (程序会在所在目录下生成文件)

2. 双击运行 `GenshinAutoV2.exe`

3. 当原神进入剧情对话时，会自动点击对话，原神窗口可以切到后台（窗口不能最小化）

4. 按 `Alt + P` 可以暂停或继续自动点对话

## 环境适配
- 仅适用于 **Windows 10** 及以上版本的系统 

- 仅适用于 `原神PC端`，暂未适配 `云原神PC端` 和 `云原神网页端`

- 仅适用于游戏中的 `中文` 语言 

- 原神 `图像设置` 中的 `显示模式` 不能选择 `独占全屏`

- 原神的窗口大小需要 大于或等于 800 X 600，当宽高比大于 `16:9` 时，窗口高度不要小于800

## 注意事项
- 在启动该工具时，除了游戏进程外，不要运行其它名为 `YuanShen.exe` 或者 `GenshinImpact.exe` 的进程

- 原神窗口可以通过 `Alt + Tab` 切到后台，但是不能最小化

- 剧情对话时，左上角的 **自动播放按钮** 要处于关闭状态

- 原神的窗口大小改变后，需要重启该工具

- **Windows11** 系统下，部分显卡可能需要在系统的 `设置` -> `系统` -> `屏幕` -> `显示卡` 中关闭 **窗口化游戏优化**
   
- 在原神窗口化运行时，如果鼠标无法锁定在游戏中，只需要在游戏中按一下 `Alt` 键，就可以让鼠标重新锁定在游戏中

- 不能在原神中开启N卡滤镜


## 自定义程序逻辑
- 程序在第一次运行的时候，会在程序所在的目录下生成 `utils.js` 和 `script.js` 两个文件，可以用 **文本编辑器** 打开

- `utils.js` 中包含了 **程序提供的接口** 以及一些 **工具函数**，一般不需要修改

- `script.js` 是程序的逻辑代码，可以使用 **JavaScript语法** 自定义程序的逻辑

## 如何手动编译本项目
1. 需要安装 [**CMake**](https://cmake.org) (cmake version 4.1.0)，或者根据 `CMakeLists.txt` 自己写编译指令

2. 需要安装 [**MinGW-w64**](https://www.mingw-w64.org) (gcc version 15.2.0)

3. 下载本项目源代码，在项目根目录下依次执行以下指令：
```powershell
cmake -G "MinGW Makefiles" -B build .
cmake --build build
```

4. 生成的程序在 `release` 目录下的 `GenshinAutoV2.exe`


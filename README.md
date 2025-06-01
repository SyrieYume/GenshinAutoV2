# GenshinAutoV2
原神后台自动点剧情工具 v2版本

## 使用方法
1. 从 [Releases](https://github.com/SyrieYume/GenshinAutoV2/releases/latest) 下载最新版的 `GenshinAutoV2.exe`

2. 双击运行 `GenshinAutoV2.exe`

3. 当原神进入剧情对话时，会自动点击对话，原神窗口可以切到后台（窗口不能最小化）

4. 按 `Ctrl + p` 可以暂停自动点对话

## 环境适配
1. 仅适用于 `原神PC端`，暂未适配 `云原神PC端` 和 `云原神网页端`

2. 仅适用于游戏中的 `中文` 语言 

3. 原神 `图像设置` 中的 `显示模式` 不能选择 `独占全屏`

4. 原神的窗口大小需要 大于或等于 800 X 600，当宽高比大于 `16:9` 时，窗口高度不要小于800

5. 如果点位无法正确识别，可以从启动参数或配置文件中指定点位

```powershell
#隐藏对话按钮的白色部分RGB(236, 229, 216)
#隐藏对话按钮黑色部分RGB(59, 67, 84)
#中间底部点击后继续橙黄色部分RGB(255, 195, 0)
#从启动参数指定
GenshinAutoV2.exe --points 420,47 418,63 1280,1286

#在工作目录或HOME目录创建.genshin_points.conf
420 47
418 63
1280 1286
```

## 注意事项
1. 在启动该工具时，除了游戏进程外，不要运行其它名为 `YuanShen.exe` 或者 `GenshinImpact.exe` 的进程

2. 原神窗口可以通过 `Alt + Tab` 切到后台，但是不能最小化

3. 剧情对话时，左上角的自动播放按钮要处于开启状态

4. 原神的窗口大小改变后，需要重启该工具
   
5. 在原神窗口化运行时，如果鼠标无法锁定在游戏中，只需要在游戏中按一下 `Alt` 键，就可以让鼠标重新锁定在游戏中

6. 不能在原神中开启N卡滤镜。

## 如何编译本项目
使用的编译器为 **MinGW** (gcc version 14.2.0)
```bash
windres ./res/res.rc res.o
gcc ./src/*.c res.o -o ./GenshinAutoV2.exe -lgdi32 -mwindows
rm res.o
```


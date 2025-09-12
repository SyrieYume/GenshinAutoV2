import { console, win, keyboard, ansi, os, sleep } from "./api.js"

try {
// 如果是国服，进程名为 "YuanShen.exe"，国际服为 "GenshinImpact.exe"
const processNames = ["YuanShen.exe", "GenshinImpact.exe"]

// 1920 x 1080 分辨率下的像素点坐标和颜色
const points = [
    { pos: [280, 35], color: win.rgb(236, 229, 216) },  // 隐藏对话按钮的白色部分
    { pos: [271, 49], color: win.rgb(59, 67, 84) },   // 隐藏对话按钮的黑色部分
]

// 根据实际的游戏窗口大小调整上面的像素点坐标
function initPoints(width, height) {
    const scale = width * 9 > height * 16 ? height / 1080.0 : width / 1920.0;

    for(const { pos } of points) {
        pos[0] = Math.round(pos[0] * scale);
        pos[1] = Math.round(pos[1] * scale);
    }
}

let pid, hwnd, wndSize, hdc

// 等待原神进程，并获取进程的pid
console.print(`${ansi.orange("[Waitting]")} 正在等待原神进程 ${ansi.blue(processNames[0])} / ${ansi.blue(processNames[1])}`)

while(!((pid=win.getPid(processNames[0])) || (pid=win.getPid(processNames[1]))))
    await sleep(200)

console.print(ansi.cleanLine)

console.info(`${ansi.blue("Pid")} = ${pid}`)


// 等待原神窗口，并获取窗口的 hwnd 以及窗口大小 wndSize
console.print(`${ansi.orange("[Waitting]")} 正在等待原神窗口`)

while (true) {
    hwnd = win.getHwnd(pid)
    wndSize = win.getWndSize(hwnd)

    if(wndSize.width > 400)
        break

    await sleep(200)
}

console.print(ansi.cleanLine)
console.info(`${ansi.blue("Hwnd")} = 0x${hwnd.toString(16)}`)
console.info(`${ansi.blue("窗口大小")}: ${wndSize.width} ${ansi.blue("X")} ${wndSize.height}`)

console.info("开始自动点击剧情中(当检测到进入剧情时会自动点击)...")
console.info(`按 ${ansi.blue("Alt + P")} 键暂停`)
console.info(`按 ${ansi.blue("Alt + K")} 键截图 (仅供测试)`)

initPoints(wndSize.width, wndSize.height)
hdc = win.getDC(hwnd)

let isActivate = true

// afterDialog：值为0表示正在剧情对话中，值为1表示不在剧情对话中，大于1表示剧情对话刚刚结束（会在几秒内递减到1）
let afterDialog = 1

while(true) {
    if(keyboard.isKeysDown('Alt', 'P')) {
        isActivate = !isActivate
        console.info(`程序${isActivate? ansi.green("继续执行"): ansi.orange("暂停")}中`)
        await sleep(400)
    }

    if(keyboard.isKeysDown('Alt', 'K')) {
        let image = win.captureWindow(hwnd, [0, 0, wndSize.width, wndSize.height])
        if(image.data.byteLength > 0) {
            os.mkdir("screenshots")
            const savePath = `screenshots/${Date.now()}.bmp`
            if(win.saveBitmapImage(savePath, image.data, image.width, image.height, image.step))
                console.info(`截图文件已保存至 "${ansi.blue(savePath)}"`)
        } else console.error("截屏失败")
        image = null
        await sleep(400)
    }

    if (isActivate) {
        // 判断左上角的隐藏对话按钮
        if (win.getPixel(hdc, ...points[0].pos) == points[0].color && win.getPixel(hdc, ...points[1].pos) == points[1].color) {
            if(afterDialog > 0) {
                afterDialog = 0
                console.info("检测到进入剧情对话")
            }
            win.setForegroundWindow(hwnd)
            // 发送点击 F 键的消息
            keyboard.sendKeyDown(hwnd, 'F')
            await sleep(75);
            keyboard.sendKeyUp(hwnd, 'F')
        }

        else if(afterDialog == 0) {
            afterDialog = 24
            console.info("剧情对话结束")
        }
    }

    // 对话结束之后，原神会将鼠标强制锁到窗口中央，这里在剧情对话结束后几秒内解除鼠标锁定
    if(afterDialog > 1) {
        afterDialog--
        win.releaseCursorClip()
    }

    await sleep(125)
}

} catch(e) {
    console.error(e.toString() + "\nTraceback:\n" + e.stack.toString())
}
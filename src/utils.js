export const console = {
    log: (text) => {
        const date = new Date()
        const hours = date.getHours().toString().padStart(2, '0')
        const miuntes = date.getMinutes().toString().padStart(2, '0')
        const seconds = date.getSeconds().toString().padStart(2, '0')
        
        _print(`[${hours}:${miuntes}:${seconds}]${text}\n`)
    },

    info: (text) => console.log(`${ansi.green("[Info]")} ${text}`),
    error: (text) => console.log(`${ansi.red("[Error]")} ${text}`),
    
    /**@type {function(any)} */
    print: _print,
    
    /**@type {function(): string} */
    input: _input
}

export const win = {
    /**@type {function(processName): pid} */
    getPid: _getPid,

    /**@type {function(pid): hwnd} */
    getHwnd: _getHwnd,
    
    /**@type {function(hwnd): {width:number, height: number}} */
    getWndSize: _getWndSize,

    /**@type {function(hwnd): hdc} */
    getDC: _getDC,

    /**@type {function(hwnd, x, y): number} */
    getPixel: _getPixel,

    /**@type {function(hwnd): boolean} */
    setForegroundWindow: _setForegroundWindow,

    /**@type {function(hwnd, msg, wparam, lparam): boolean} */
    postMessageW: _postMessageW,

    /**@type {function(hwnd, hdc): boolean} */
    releaseDC: _releaseDC,

    /**@type {function(): boolean} */
    releaseCursorClip: _releaseCursorClip,
    
    captureWindow: (hwnd, left, top, right, bottom) => {
        const imgPtr = _captureWindow(hwnd, left, top, right, bottom)
        return imgPtr === 0n ? null : new Image(imgPtr)
    },

    rgb: (r, g, b) => r | (g << 8) | (b << 16)
}

export const keyboard = {
    isKeyDown: (key) => _isKeyDown(keyCodes[key]),

    isKeysDown: (...keys) => keys.every(keyboard.isKeyDown),

    sendKeyDown: (hwnd, key) => _postMessageW(hwnd, 0x0100, BigInt(keyCodes[key]), makeKeyEventLparam(1, key, false, false, false)),
    
    sendKeyUp: (hwnd, key) => _postMessageW(hwnd, 0x0101, BigInt(keyCodes[key]), makeKeyEventLparam(1, key, false, true, true)),
}

export const os = {
    /**@type {function(millseconds)}*/
    sleep: _sleep,

    /**@type {function(dirName) :boolean}*/
    mkdir: _mkdir,
}

// ANSI转义序列
export const ansi = {
    // 设置控制台光标的位置 -> (x, y)
    cursorPos: (x, y) => `\x1b[${x + 1};${y + 1}H`,

    // 保存控制台光标的位置
    saveCursorPos: "\x1b[s",

    // 恢复控制台光标的位置
    restoreCursorPos: "\x1b[u",

    // 清除一行内容
    cleanLine: "\r\x1b[K",

    /** 给文本添加样式
     * @param {string} text
     * @param {{fg: string, bg: string, bold: boolean}} styles
     * @returns {string}
     */
    style(text, styles = {}) {
        const { fg, bg, bold } = styles
        let codes = []
        bold && codes.push(1)
        fg && codes.push(38, 2, ...hexToRgb(fg))
        bg && codes.push(48, 2, ...hexToRgb(bg))

        return codes.length > 0 ? `\x1b[${codes.join(';')}m${text}\x1b[0m` : text;
    },

    blue: text => ansi.style(text, {fg: "#4cbafa"}),
    green: text => ansi.style(text, {fg: "#60c887"}),
    red: text => ansi.style(text, {fg: "#f76568"}),
    orange: text => ansi.style(text, {fg: "#f19558"}), 
}


// 键盘扫描码
const scanCodes = {
    '1': 2, '2': 3, '3': 4, '4': 5, '5': 6, '6': 7, '7': 8, '8': 9, '9': 10, '0': 11, 

    'Q': 16, 'W': 17, 'E': 18, 'R': 19, 'T': 20, 'Y': 21, 'U': 22, 'I': 23, 'O': 24, 'P': 25, 
    'A': 30, 'S': 31, 'D': 32, 'F': 33, 'G': 34, 'H': 35, 'J': 36, 'K': 37, 'L': 38, 
    'Z': 44, 'X': 45, 'C': 46, 'V': 47, 'B': 48, 'N': 49, 'M': 50,
    
    'q': 16, 'w': 17, 'e': 18, 'r': 19, 't': 20, 'y': 21, 'u': 22, 'i': 23, 'o': 24, 'p': 25, 
    'a': 30, 's': 31, 'd': 32, 'f': 33, 'g': 34, 'h': 35, 'j': 36, 'k': 37, 'l': 38, 
    'z': 44, 'x': 45, 'c': 46, 'v': 47, 'b': 48, 'n': 49, 'm': 50,

    'Ctrl': 0x1d, 'Alt': 0x38
}

// 虚拟按键码
const keyCodes = {
    '0': 0x30, '1': 0x31, '2': 0x32, '3': 0x33, '4': 0x34, '5': 0x35, '6': 0x36, '7': 0x37, '8': 0x38, '9': 0x39,
    
    'A': 0x41, 'B': 0x42, 'C': 0x43, 'D': 0x44, 'E': 0x45, 'F': 0x46, 'G': 0x47, 'H': 0x48, 'I': 0x49,
    'J': 0x4A, 'K': 0x4B, 'L': 0x4C, 'M': 0x4D, 'N': 0x4E, 'O': 0x4F, 'P': 0x50, 'Q': 0x51, 'R': 0x52,
    'S': 0x53, 'T': 0x54, 'U': 0x55, 'V': 0x56, 'W': 0x57, 'X': 0x58, 'Y': 0x59, 'Z': 0x5A,    

    'a': 0x61, 'b': 0x62, 'c': 0x63, 'd': 0x64, 'e': 0x65, 'f': 0x66, 'g': 0x67, 'h': 0x68, 'i': 0x69,
    'j': 0x6A, 'k': 0x6B, 'l': 0x6C, 'm': 0x6D, 'n': 0x6E, 'o': 0x6F, 'p': 0x70, 'q': 0x71, 'r': 0x72,
    's': 0x73, 't': 0x74, 'u': 0x75, 'v': 0x76, 'w': 0x77, 'x': 0x78, 'y': 0x79, 'z': 0x7A,

    'Ctrl': 0x11, 'Alt': 0x12
};


class Image {
    constructor(imgPtr) { this.imgPtr = imgPtr }
    
    save(filepath) { return _saveImage(this.imgPtr, filepath) }

    free() { _freeImage(this.imgPtr) }
}

/** 十六进制颜色转rgb颜色
 * @param {string} 严格遵守格式的十六进制颜色字符串
 * @returns {[r: number, g: number, b: number]}
 */
function hexToRgb(hexColor) {
    return [
        parseInt(hexColor.substring(1, 3), 16),
        parseInt(hexColor.substring(3, 5), 16),
        parseInt(hexColor.substring(5, 7), 16)
    ]
}

// 构造一个 WM_KEYDOWN 或 WM_KEYUP 消息的 lParam 参数
function makeKeyEventLparam(repeatCount, key, extendedKey, previousState, transitionState) {
    let lparam = 0

    // 设置重复计数 (0-15位)
    lparam |= repeatCount & 0xFFFF

    // 设置扫描码 (16-23位)
    lparam |= (scanCodes[key] & 0xFF) << 16

    // 设置扩展键标志 (24位)
    if(extendedKey)
        lparam |= 1 << 24
    
    // 设置前一个键状态 (30位)
    if(previousState)
        lparam |= 1 << 30

    // 设置转换状态 (31位)
    if(transitionState)
        lparam |= 1 << 31
    
    return BigInt(lparam)
}
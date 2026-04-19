# WinInputInjector

<img src="./img/img.png" alt="en"> <br>

[English](#english) | [中文](#中文)

---

## English

**WinInputInjector** is a lightweight Windows application built with C++ and Win32 API that allows you to inject text (Unicode characters) into applications that block standard copy-pasting or input methods (e.g., certain games, restricted web pages, or virtual machines).

### Features
* **Unicode Text Injection:** Sends text natively via Windows Input Simulator APIs (`SendInput`).
* **Adjustable Interval:** Control the injection speed by setting a microsecond interval between each character.
* **Global Hotkey:** Press **F2** to quickly trigger the text injection.
* **Progress Tracking:** Built-in progress bar to monitor injection status in real-time.
* **Low Latency Keyboard Hook:** Catches hotkeys globally across the system without delay.

### How to Use
1. Run the application.
2. Type or paste the text you want to inject into the text box.
3. Select the input mode (currently supports **SendUnicodeInput**).
4. Set an **Interval (us)** if you need to slow down the injection to prevent dropped characters (e.g., `50000` for 50ms). leave as `0` for instant injection.
5. Switch to the target application (game or website) and select the input field.
6. Press **F2** to start the injection.

---

## 中文

**WinInputInjector** 是一个使用 C++ 和 Win32 API 编写的轻量级 Windows 应用程序，允许您将文本（Unicode 字符）直接注入到那些禁止标准复制粘贴或屏蔽输入法的应用程序中（例如：某些游戏、受限网页或虚拟机）。

### 功能特点
* **Unicode 文本注入：** 通过 Windows 原生输入模拟 API (`SendInput`) 发送文本。
* **自定义间隔时间：** 可设置每个字符注入之间的微秒级间隔，精准控制按键速度。
* **全局快捷键：** 按下 **F2** 即可快速触发注入（支持后台生效）。
* **进度追踪：** 内置进度条，实时监控当前文本的注入进度。
* **低延迟键盘钩子：** 无延迟捕获系统全局快捷键。

### 使用方法
1. 运行应用程序。
2. 在文本框中输入或粘贴您想要注入的文字。
3. 选择输入模式（目前支持 **SendUnicodeInput** 模式）。
4. 如果目标程序对输入速度有限制，可以设置 **Interval (us)（间隔微秒）** 以减慢注入速度，防止字符丢失（例如：输入 `50000` 代表 50 毫秒）。设置为 `0` 则瞬时输入。
5. 切换到目标应用程序（游戏或网页），并选中想要输入文字的输入框。
6. 按下键盘上的 **F2** 键开始注入。

---

## Build / 编译
- Requirements: **Visual Studio 2022** (or compatible) with C++ Desktop Development workload.
- C++ Standard: **C++20** (for `<format>`, `<stop_token>`, etc.)
- Platform Toolset: **v143** (or latest)

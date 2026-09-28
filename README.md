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
4. **Interval (us)** defaults to `1000` (1 ms). If the target drops or scrambles characters, try `10000` (10 ms) or `50000` (50 ms). Set `0` for one-batch, unthrottled injection.
5. Switch to the target application (game or website) and select the input field.
6. Press **F2** to start the injection.

### Input Limit
The text box accepts up to **100,000 UTF-16 code units**, including line breaks. Common Chinese characters and English letters each count as one unit; some emoji count as two or more units. Text typed or pasted beyond the limit will not be fully accepted; check the contents before submitting.

Large texts may cause delays when pasting, editing, or processing input in the target application. An interval of `0` sends all input events in one batch, with the progress bar updating only when injection finishes; this does not guarantee that the target application has finished processing the text.

Positive intervals wait after each send completes, without catch-up bursts. Valid UTF-16 surrogate pairs (such as many emoji) are sent together. The requested delay is not an exact timing guarantee; Windows scheduling can make it longer, and 1 ms may still be too fast for some editors. Newline characters are sent unchanged and may be handled differently by different editors.

The status line distinguishes completion, failure, and cancellation. A zero or partial `SendInput` result stops the task without retrying; progress counts only fully sent characters (measured in UTF-16 units, excluding incomplete surrogate pairs), and failures do not show 100%. The failure status includes the completed position, last call's sent/requested event counts, and any Windows error code. Completion means events were sent to Windows, **not that the target text was verified**. Closing the app cancels remaining paced input; an already submitted batch cannot be recalled.

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
4. **Interval (us)（间隔微秒）** 默认为 `1000`（1 毫秒）。如果目标程序出现丢字或错乱，可改为 `10000`（10 毫秒）或 `50000`（50 毫秒）。设置为 `0` 则不节流、整批发送。
5. 切换到目标应用程序（游戏或网页），并选中想要输入文字的输入框。
6. 按下键盘上的 **F2** 键开始注入。

### 输入限制
文本框最多接受 **100,000 个 UTF-16 代码单元**，换行也计入限制。常见汉字和英文字母各占 1 个单元，部分 emoji 占 2 个或更多单元。超过上限的输入或粘贴内容无法完整保留，请在提交前检查文本。

大文本在粘贴、编辑或目标程序处理输入时可能出现延迟。间隔设为 `0` 时会一次性发送全部输入事件，进度条仅在注入结束时更新；这不保证目标程序已经处理完全部文字。

正间隔从每次发送完成后开始等待，不会追赶补发。合法 UTF-16 代理对（例如许多 emoji）会整组发送。间隔是请求的等待时间，Windows 调度可能使实际等待更长；1 毫秒仍可能超过某些编辑器的处理能力。换行字符保持原样发送，不同编辑器可能有不同的处理结果。

状态栏区分完成、失败和取消。`SendInput` 零发送或部分发送时立即停止，不自动重试；进度只计入完整发送的字符（以 UTF-16 单元计数，不包含未完整发送的代理对），失败不会显示 100%。失败信息包含已完成的位置、最后一次调用的实际/请求事件数和可用的 Windows 错误码。完成仅代表事件已发送给 Windows，**不代表目标文本已核对正确**。关闭程序会取消剩余的限速发送；已提交的整批事件无法撤回。

---

## Build / 编译
- Requirements / 环境要求: **Visual Studio** with the **Desktop development with C++** workload, **MSVC v145**, and a **Windows 10/11 SDK**.
- C++ Standard / 语言标准: **C++20** (for concepts and `<stop_token>`).
- Platform / 平台: **x64 only / 仅支持 x64**; Debug and Release configurations are available. 32-bit and WOW64 targets are not supported.
- Dependency / 依赖: **Boost** headers, including `<boost/lockfree/spsc_queue.hpp>`. Add the Boost root directory to the compiler's include paths; no compiled Boost library is required. 将 Boost 根目录加入编译器包含路径，无需链接 Boost 二进制库。
- Build / 构建: Open `WinInputInjector.slnx` in Visual Studio and build **Debug | x64** or **Release | x64**. 打开解决方案并选择对应的 x64 配置构建。
- Validation / 验证: See [x64 injection checks](tests/README.md) for automated tests and the offline browser fixture. 自动化测试与离线浏览器测试页见该说明。

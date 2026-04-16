#pragma once

#ifndef _LOW_LATENCY_KEYBOARD_HPP_
#define _LOW_LATENCY_KEYBOARD_HPP_

#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>

class LowLatencyKeyboard final {
public:
	struct KeyEvent {
		uint16_t vkey = 0;
		uint16_t scancode = 0;
		uint16_t flags = 0;
		uint16_t down = 0;
	};

	inline static constexpr size_t kQueueCapacity = 8192;

	inline static LowLatencyKeyboard& getInstance() {
		static LowLatencyKeyboard instance;
		return instance;
	}

	inline static bool handleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		static bool init = [] (HWND hwnd) -> bool {
			hwnd_ = hwnd;
			if (!hwnd_) return false;

			RAWINPUTDEVICE rid{};
			rid.usUsagePage = 0x01;
			rid.usUsage = 0x06;
			rid.dwFlags = 0;
			rid.dwFlags |= RIDEV_NOLEGACY;
			rid.dwFlags |= RIDEV_INPUTSINK;
			rid.hwndTarget = hwnd_;

			if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
				hwnd_ = nullptr;
				return false;
			}

			clear();
			return true;
		} (hwnd);

		switch (msg) {
			case WM_INPUT:
			{
				onRawInput(lParam);
				return true;
			}
			default:
			{
				return false;
			}
		}
		return false;
	}

	inline static bool popEvent(KeyEvent& out) noexcept {
		return queue_.pop(out);
	}

	inline static size_t popEvents(KeyEvent* out, size_t maxCount) noexcept {
		size_t n = 0;
		for (; n < maxCount; ++n) {
			if (!queue_.pop(out[n])) break;
		}
		return n;
	}

	inline static void clear() {
		KeyEvent dummy{};
		while (queue_.pop(dummy)) {}
		clearKeyState();
	}

	inline static bool isKeyDown(uint16_t vkey) noexcept {
		if (vkey >= 256) return false;
		return key_down_[vkey].load(std::memory_order_acquire);
	}

private:
	LowLatencyKeyboard() = default;
	LowLatencyKeyboard(const LowLatencyKeyboard&) = delete;
	LowLatencyKeyboard& operator=(const LowLatencyKeyboard&) = delete;
	LowLatencyKeyboard(LowLatencyKeyboard&&) = delete;
	LowLatencyKeyboard& operator=(LowLatencyKeyboard&&) = delete;

	virtual ~LowLatencyKeyboard() {
		RAWINPUTDEVICE rid{};
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x06;
		rid.dwFlags = RIDEV_REMOVE;
		rid.hwndTarget = nullptr;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));
		hwnd_ = nullptr;
		clear();
	}

	inline static void clearKeyState() {
		for (size_t i = 0; i < 256; ++i) {
			shadow_down_[i] = 0;
			key_down_[i].store(0, std::memory_order_relaxed);
		}
	}

	inline static uint16_t normalizeVKey(const RAWKEYBOARD& kbd) {
		uint16_t vkey = (uint16_t)kbd.VKey;
		const uint16_t flags = (uint16_t)kbd.Flags;
		if (vkey == VK_SHIFT) {
			vkey = (kbd.MakeCode == 0x36) ? VK_RSHIFT : VK_LSHIFT;
		} else if (vkey == VK_CONTROL) {
			vkey = (flags & RI_KEY_E0) ? VK_RCONTROL : VK_LCONTROL;
		} else if (vkey == VK_MENU) {
			vkey = (flags & RI_KEY_E0) ? VK_RMENU : VK_LMENU;
		}
		return vkey;
	}

	inline static bool pushEvent_(const KeyEvent& ev) noexcept {
		return queue_.push(ev);
	}

	inline static void onRawInput(LPARAM lParam) {
		RAWINPUT raw{};
		UINT size = sizeof(raw);

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == (UINT)(-1)) return;

		if (raw.header.dwType != RIM_TYPEKEYBOARD) return;

		const RAWKEYBOARD& kbd = raw.data.keyboard;
		if (kbd.VKey == 255) return; // fake key

		const uint16_t vkey = normalizeVKey(kbd);
		if (vkey >= 256) return;

		const uint16_t scan = (uint16_t)kbd.MakeCode;
		const uint16_t flags = (uint16_t)kbd.Flags;
		const uint8_t newDown = (flags & RI_KEY_BREAK) ? 0 : 1;

		if (shadow_down_[vkey] == newDown) return;

		shadow_down_[vkey] = newDown;
		key_down_[vkey].store(newDown, std::memory_order_release);

		KeyEvent ev{ vkey, scan, flags, newDown };
		pushEvent_(ev);
	}

	inline static HWND hwnd_ = nullptr;

	// SPSC queue, fixed size
	inline static boost::lockfree::spsc_queue<KeyEvent, boost::lockfree::capacity<kQueueCapacity>> queue_{};

	inline static std::array<uint8_t, 256> shadow_down_{};
	inline static std::array<std::atomic<uint8_t>, 256> key_down_{};
};

#endif // !_LOW_LATENCY_KEYBOARD_HPP_
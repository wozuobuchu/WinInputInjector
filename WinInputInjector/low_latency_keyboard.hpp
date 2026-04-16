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

	static constexpr size_t kQueueCapacity = 8192;

	bool handleWndProc(HWND, UINT msg, WPARAM, LPARAM lParam) {
		switch (msg) {
		case WM_INPUT:
			onRawInput(lParam);
			return true;
		default:
			return false;
		}

		return false;
	}

	bool popEvent(KeyEvent& out) noexcept {
		return queue_.pop(out);
	}

	size_t popEvents(KeyEvent* out, size_t maxCount) noexcept {
		size_t n = 0;
		for (; n < maxCount; ++n) {
			if (!queue_.pop(out[n])) break;
		}
		return n;
	}

	void clear() {
		KeyEvent dummy{};
		while (queue_.pop(dummy)) {}
		clearKeyState();
	}

	bool init(HWND hwnd, bool disableLegacy = true, bool captureInBackground = false) noexcept {
		hwnd_ = hwnd;
		if (!hwnd_) return false;

		RAWINPUTDEVICE rid{};
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x06;
		rid.dwFlags = 0;
		if (disableLegacy) rid.dwFlags |= RIDEV_NOLEGACY;
		if (captureInBackground) rid.dwFlags |= RIDEV_INPUTSINK;
		rid.hwndTarget = hwnd_;

		if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
			hwnd_ = nullptr;
			return false;
		}

		clear();
		return true;
	}

	LowLatencyKeyboard() = default;

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

protected:
	// usually useless for external users
	bool isKeyDown(uint16_t vkey) const noexcept {
		if (vkey >= 256) return false;
		return key_down_[vkey].load(std::memory_order_acquire);
	}

private:
	void clearKeyState() {
		for (size_t i = 0; i < 256; ++i) {
			shadow_down_[i] = 0;
			key_down_[i].store(0, std::memory_order_relaxed);
		}
	}

	static uint16_t normalizeVKey(const RAWKEYBOARD& kbd) {
		uint16_t vkey = (uint16_t)kbd.VKey;
		const uint16_t flags = (uint16_t)kbd.Flags;
		if (vkey == VK_SHIFT) {
			vkey = (uint16_t)MapVirtualKeyW(kbd.MakeCode, MAPVK_VSC_TO_VK_EX);
		} else if (vkey == VK_CONTROL) {
			vkey = (flags & RI_KEY_E0) ? VK_RCONTROL : VK_LCONTROL;
		} else if (vkey == VK_MENU) {
			vkey = (flags & RI_KEY_E0) ? VK_RMENU : VK_LMENU;
		}
		return vkey;
	}

	bool pushEvent_(const KeyEvent& ev) noexcept {
		return queue_.push(ev);
	}

	void onRawInput(LPARAM lParam) {
		UINT size = 0;
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0) return;

		tmp_.resize(size);
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, tmp_.data(), &size, sizeof(RAWINPUTHEADER)) != size) return;

		const RAWINPUT* ri = reinterpret_cast<const RAWINPUT*>(tmp_.data());
		if (ri->header.dwType != RIM_TYPEKEYBOARD) return;

		const RAWKEYBOARD& kbd = ri->data.keyboard;
		if (kbd.VKey == 255) return; // fake key

		const uint16_t vkey = normalizeVKey(kbd);
		const uint16_t scan = (uint16_t)kbd.MakeCode;
		const uint16_t flags = (uint16_t)kbd.Flags;
		const uint8_t newDown = (flags & RI_KEY_BREAK) ? 0 : 1;

		// discard repeat events
		if (shadow_down_[vkey] == newDown) return;
		
		// update state only when changed
		shadow_down_[vkey] = newDown;
		key_down_[vkey].store(newDown, std::memory_order_release);

		KeyEvent ev{};
		ev.vkey = vkey;
		ev.scancode = scan;
		ev.flags = flags;
		ev.down = newDown;

		pushEvent_(ev);
	}

	HWND hwnd_ = nullptr;

	// SPSC queue, fixed size
	boost::lockfree::spsc_queue<KeyEvent, boost::lockfree::capacity<kQueueCapacity>> queue_{};

	std::array<uint8_t, 256> shadow_down_{};
	std::array<std::atomic<uint8_t>, 256> key_down_{};

	std::vector<uint8_t> tmp_;
};

#endif // !_LOW_LATENCY_KEYBOARD_HPP_
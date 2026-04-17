#ifndef _INJ_UNICODE_HPP
#define _INJ_UNICODE_HPP

#pragma once

#include <windows.h>
#include <string>
#include <vector>

#include "injector.hpp"

class UnicodeSender final : public Injector {
public:
	virtual bool inject_wstring(const std::wstring& text) override {
		std::vector<INPUT> inputs;
		inputs.reserve(text.size() * 2);

		for (wchar_t ch : text) {
			INPUT down = {};
			down.type = INPUT_KEYBOARD;
			down.ki.wScan = ch;
			down.ki.dwFlags = KEYEVENTF_UNICODE;

			INPUT up = {};
			up.type = INPUT_KEYBOARD;
			up.ki.wScan = ch;
			up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

			inputs.push_back(down);
			inputs.push_back(up);
		}

		if (!inputs.empty()) {
			return SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT)) == inputs.size();
		}

		return false;
	}

	virtual bool inject_wchar(wchar_t ch) override {
		INPUT inputs[2] = {};

		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wScan = ch;
		inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wScan = ch;
		inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

		return SendInput(2, inputs, sizeof(INPUT)) == 2;
	}
};

#endif // !_INJ_UNICODE_HPP
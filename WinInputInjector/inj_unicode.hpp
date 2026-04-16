#ifndef _INJ_UNICODE_HPP
#define _INJ_UNICODE_HPP

#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace inj_unicode {

	inline static void SendUnicodeString(const std::wstring& text) {
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
			SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
		}
	}

}

#endif // !_INJ_UNICODE_HPP
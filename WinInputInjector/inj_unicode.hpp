#ifndef _INJ_UNICODE_HPP
#define _INJ_UNICODE_HPP

#pragma once

#include <string>
#include <vector>
#include <windows.h>

#include "injector.hpp"

class UnicodeSender final : public Injector {
public:
    using SendInputFunction = decltype(&::SendInput);

    explicit UnicodeSender(SendInputFunction send_input = &::SendInput) : send_input_(send_input) {}

    InjectionResult inject_wstring(std::wstring_view text) override {
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
            // Clear stale errors: Windows may not provide a reason for a short send.
            SetLastError(ERROR_SUCCESS);
            const UINT sent = send_input_(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
            const DWORD error = sent == inputs.size() ? ERROR_SUCCESS : GetLastError();
            return {inputs.size(), sent, error};
        }

        return {};
    }

private:
    SendInputFunction send_input_;
};

#endif // !_INJ_UNICODE_HPP

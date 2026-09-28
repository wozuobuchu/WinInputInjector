#include <Windows.h>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

#include "../WinInputInjector/inj_unicode.hpp"
#include "../WinInputInjector/injection_run.hpp"

static_assert(sizeof(void*) == 8, "Tests require x64");
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::vector<INPUT>> calls;
std::vector<Clock::time_point> starts, finishes;
UINT send_limit = UINT_MAX;
size_t fail_on_call = 1;
DWORD error_to_set = 0;
std::chrono::milliseconds send_delay{0};

UINT WINAPI fake_send(UINT count, LPINPUT inputs, int size) {
    require(size == sizeof(INPUT), "INPUT size");
    starts.push_back(Clock::now());
    calls.emplace_back(inputs, inputs + count);
    std::this_thread::sleep_for(send_delay);
    if (error_to_set) SetLastError(error_to_set);
    finishes.push_back(Clock::now());
    return calls.size() >= fail_on_call ? (std::min)(count, send_limit) : count;
}

void reset() {
    calls.clear(); starts.clear(); finishes.clear();
    send_limit = UINT_MAX; error_to_set = 0; send_delay = 0ms;
    fail_on_call = 1;
}

std::wstring captured_text() {
    std::wstring actual;
    for (const auto& call : calls) {
        require(call.size() % 2 == 0, "paired events");
        for (size_t i = 0; i < call.size(); i += 2) {
            const auto& down = call[i];
            const auto& up = call[i + 1];
            require(down.type == INPUT_KEYBOARD && up.type == INPUT_KEYBOARD, "keyboard events");
            require(down.ki.wVk == 0 && up.ki.wVk == 0, "Unicode virtual key");
            require(down.ki.dwFlags == KEYEVENTF_UNICODE, "down flags");
            require(up.ki.dwFlags == (KEYEVENTF_UNICODE | KEYEVENTF_KEYUP), "up flags");
            require(down.ki.wScan == up.ki.wScan, "matching keyup");
            actual += static_cast<wchar_t>(down.ki.wScan);
        }
    }
    return actual;
}

void test_chunks(UnicodeSender& sender) {
    const auto ignore = [](size_t, size_t) {};
    using namespace injection_settings;
    require(parse_chunk_size(L"") == 128 && parse_chunk_size(L"abc") == 128 &&
        parse_chunk_size(L"128x") == 128 && parse_chunk_size(L"1.5") == 128, "invalid chunk default");
    require(parse_chunk_size(L"0") == 1 && parse_chunk_size(L"-3") == 1 &&
        parse_chunk_size(L"32769") == 32768 && parse_chunk_size(L"9999999999999999999999999999999999") == 32768,
        "chunk range and overflow");
    require(parse_chunk_size(L"999999999999999999999x") == 128 && parse_chunk_size(L"00128") == 128,
        "validate full input");
    require(parse_chunk_size(L"1") == 1 && parse_chunk_size(L"32768") == 32768, "valid boundaries");

    const std::wstring mixed = L"A\U0001F600\t \r\nB\n";
    std::wstring large;
    while (large.size() + mixed.size() <= 100000) large += mixed;
    large.resize(100000, L'X');
    for (int chunk_size : {1, 128, 32768, 0, -1, 100000}) {
        reset();
        auto result = run_injection(sender, large, 0, chunk_size, {}, ignore);
        require(result.status == InjectionStatus::Completed && calls.size() == 1 &&
            captured_text() == large, "zero ignores all chunk sizes");

        reset();
        // Small per-character case; large cases exercise all 100,000 units.
        const std::wstring_view text = clamp_chunk_size(chunk_size) == 1 ? std::wstring_view(mixed) : large;
        result = run_injection(sender, text, 1, chunk_size, {}, ignore);
        require(result.status == InjectionStatus::Completed && result.completed_units == text.size() &&
            captured_text() == text, "chunked exact text");
        for (size_t index = 0; index < calls.size(); ++index) {
            const auto& call = calls[index];
            size_t characters = 0;
            for (size_t i = 0; i < call.size(); i += 2) {
                ++characters;
                if (i + 2 < call.size() && is_surrogate_pair(call[i].ki.wScan, call[i + 2].ki.wScan)) i += 2;
            }
            require(characters <= static_cast<size_t>(clamp_chunk_size(chunk_size)), "batch limit by characters");
            if (index + 1 < calls.size()) {
                require(characters == static_cast<size_t>(clamp_chunk_size(chunk_size)), "full non-final batch");
                require(!is_surrogate_pair(call.back().ki.wScan, calls[index + 1].front().ki.wScan), "no split pair");
            }
        }
    }
    for (size_t length : {256u, 257u}) {
        reset();
        run_injection(sender, std::wstring(length, L'a'), 1, 128, {}, ignore);
        require(calls.size() == (length + 127) / 128, "full and short tail batches");
    }
    reset();
    run_injection(sender, L"A\U0001F600B", 1, 2, {}, ignore);
    require(calls.size() == 2 && calls[0].size() == 6 && calls[1].size() == 2, "pair counts as one character");

    reset();
    std::wstring supplementary;
    for (int i = 0; i < 32769; ++i) supplementary += L"\U0001F600";
    run_injection(sender, supplementary, 1, 32768, {}, ignore);
    require(calls.size() == 2 && calls[0].size() == 131072 && calls[1].size() == 4 &&
        captured_text() == supplementary, "maximum chunk counts pairs as characters, not units");
    reset();
    const std::wstring unpaired{static_cast<wchar_t>(0xD800), L'X', static_cast<wchar_t>(0xDC00)};
    run_injection(sender, unpaired, 1, 2, {}, ignore);
    require(calls.size() == 2 && calls[0].size() == 4 && captured_text() == unpaired,
        "unpaired surrogate units remain unchanged");

    reset(); send_limit = 3; fail_on_call = 2;
    std::vector<size_t> progress;
    auto result = run_injection(sender, L"AB\U0001F600CDE", 1000, 2, {},
        [&](size_t done, size_t) { progress.push_back(done); });
    require(result.status == InjectionStatus::Failed && calls.size() == 2 && result.completed_units == 2 &&
        progress == std::vector<size_t>{2, 2}, "partial second batch excludes incomplete pair and stops");
    reset(); send_limit = 2; fail_on_call = 2;
    result = run_injection(sender, L"ABCDE", 1, 2, {}, ignore);
    require(result.status == InjectionStatus::Failed && result.completed_units == 3 && calls.size() == 2,
        "partial batch counts completed characters");
    reset(); send_limit = 0; fail_on_call = 2;
    result = run_injection(sender, L"ABCDE", 1, 2, {}, ignore);
    require(result.status == InjectionStatus::Failed && result.completed_units == 2 && calls.size() == 2,
        "zero send in later batch stops");

    reset(); send_delay = 15ms;
    run_injection(sender, std::wstring(300, L'x'), 1000, 128, {}, ignore);
    require(calls.size() == 3, "batched pacing calls");
    for (size_t i = 1; i < starts.size(); ++i)
        require(starts[i] - finishes[i - 1] >= 1ms, "gap after whole batch completes");
    reset();
    std::stop_source stop;
    result = run_injection(sender, std::wstring(300, L'x'), 1000, 128, stop.get_token(),
        [&](size_t, size_t) { stop.request_stop(); });
    require(result.status == InjectionStatus::Cancelled && calls.size() == 1 && result.completed_units == 128,
        "cancel before next batch");
    reset();
}

int main() {
    try {
        UnicodeSender sender(fake_send);
        test_chunks(sender);
        const auto ignore = [](size_t, size_t) {};
        const std::wstring sample = L"const 中文 = '\U0001F600';\r\n\t<>&\"(){}[]\n";
        for (int interval : {0, 1000, 10000}) {
            reset();
            size_t last_progress = 0;
            const auto result = run_injection(sender, sample, interval, 1, {}, [&](size_t done, size_t total) {
                require(done > last_progress && done <= total, "monotonic progress");
                last_progress = done;
            });
            require(result.status == InjectionStatus::Completed && last_progress == sample.size(), "success");
            require(captured_text() == sample, "exact text and newline preservation");
            if (interval == 0) require(calls.size() == 1, "zero remains one batch");
            else {
                require(calls.size() == sample.size() - 1, "surrogate pair grouped");
                for (size_t i = 1; i < starts.size(); ++i)
                    require(starts[i] - finishes[i - 1] >= std::chrono::microseconds(interval), "minimum gap");
            }
        }
        reset();
        std::wstring large;
        while (large.size() + sample.size() <= 100000) large += sample;
        large.resize(100000, L'x');
        auto result = run_injection(sender, large, 0, 1, {}, ignore);
        require(result.status == InjectionStatus::Completed && calls.size() == 1, "100k batch");
        require(captured_text() == large && calls[0].size() == 200000, "100k exact order");

        // Every possible partial cut, including half a key pair and half a surrogate pair.
        const std::wstring pair_text = L"A\U0001F600B";
        const size_t expected_units[] = {0, 0, 1, 1, 1, 1, 3, 3};
        for (UINT cut = 0; cut < 8; ++cut) {
            reset(); send_limit = cut;
            SetLastError(ERROR_ACCESS_DENIED); // Must not leak a stale error.
            result = run_injection(sender, pair_text, 0, 1, {}, ignore);
            require(result.status == InjectionStatus::Failed && calls.size() == 1, "short send stops without retry");
            require(result.completed_units == expected_units[cut], "partial progress boundary");
            require(result.last_send.requested_events == 8 && result.last_send.sent_events == cut, "counts");
            require(result.last_send.error_code == 0, "no stale error");
        }
        reset(); send_limit = 0; error_to_set = ERROR_ACCESS_DENIED;
        result = run_injection(sender, sample, 1000, 1, {}, ignore);
        require(result.status == InjectionStatus::Failed && calls.size() == 1, "paced zero stops");
        require(result.last_send.error_code == ERROR_ACCESS_DENIED, "error propagation");
        reset(); send_limit = 2;
        result = run_injection(sender, pair_text, 1000, 1, {}, ignore);
        require(result.status == InjectionStatus::Failed && calls.size() == 2 && result.completed_units == 1,
            "paced half surrogate not counted");

        reset(); send_delay = 15ms;
        result = run_injection(sender, L"abcd", 1000, 1, {}, ignore);
        for (size_t i = 1; i < starts.size(); ++i)
            require(starts[i] - finishes[i - 1] >= 1ms, "slow sends do not trigger catch-up");

        reset();
        std::stop_source pre_cancel;
        pre_cancel.request_stop();
        result = run_injection(sender, sample, 0, 1, pre_cancel.get_token(), ignore);
        require(result.status == InjectionStatus::Cancelled && calls.empty(), "cancel before batch");
        reset();
        std::stop_source cancel;
        std::jthread stopper;
        const auto before = Clock::now();
        result = run_injection(sender, L"abc", 1000000, 1, cancel.get_token(), [&](size_t, size_t) {
            stopper = std::jthread([&] { std::this_thread::sleep_for(20ms); cancel.request_stop(); });
        });
        require(result.status == InjectionStatus::Cancelled && result.completed_units == 1 && calls.size() == 1,
            "cancel during wait sends no next character");
        require(Clock::now() - before < 500ms, "stop interrupts long wait");
        stopper.join();
        reset();
        result = run_injection(sender, L"", 1000, 1, {}, ignore);
        require(result.status == InjectionStatus::Completed && calls.empty(), "empty input");
        std::cout << "PASS: chunk sizes/parsing/boundaries, 100k text, zero mode, partial sends, cancellation, pacing, Unicode\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

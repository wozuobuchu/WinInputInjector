#ifndef _INJECT_HPP
#define _INJECT_HPP

#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <string_view>

struct InjectionResult {
    size_t requested_events = 0;
    size_t sent_events = 0;
    uint32_t error_code = 0;

    bool succeeded() const { return sent_events == requested_events; }
};

class Injector {
private:
    std::chrono::microseconds tick_interval_{0};
    std::chrono::steady_clock::time_point last_send_completed_{};
    bool has_sent_ = false;
    std::mutex wait_mutex_;
    std::condition_variable_any wait_cv_;

public:
    Injector() = default;

    virtual ~Injector() = default;

    virtual InjectionResult inject_wstring(std::wstring_view text) = 0;

    virtual InjectionResult inject_wchar(wchar_t ch) {
        return inject_wstring(std::wstring_view(&ch, 1));
    }

    void set_tick_interval(int64_t interval_us) {
        tick_interval_ = std::chrono::microseconds(std::clamp<int64_t>(interval_us, 0, 1000000));
        has_sent_ = false;
    }

    bool tick(std::stop_token stop) {
        if (has_sent_ && tick_interval_.count() > 0) {
            std::unique_lock lock(wait_mutex_);
            wait_cv_.wait_until(lock, stop, last_send_completed_ + tick_interval_, [] { return false; });
        }
        return !stop.stop_requested();
    }

    void mark_send_completed() {
        last_send_completed_ = std::chrono::steady_clock::now();
        has_sent_ = true;
    }
};

#endif // !

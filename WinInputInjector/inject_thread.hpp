#ifndef INJECT_THREAD_HPP
#define INJECT_THREAD_HPP

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "aop.hpp"

#include "shared_data.hpp"

#include "inj_keyboardsim.hpp"
#include "inj_unicode.hpp"
#include "injection_run.hpp"

class InjectThread final {
public:
    inline static InjectThread& getInstance() {
        static InjectThread instance;
        return instance;
    }

    inline static double get_progress() {
        return std::clamp(progress_.load(std::memory_order_relaxed), 0, 100);
    }

    inline static bool check_ready() {
        return ready_.load(std::memory_order_acquire);
    }

    inline static InjectionReport get_report() {
        auto lock = report_.acquire_lock();
        return *lock;
    }

    inline static bool launch_injection(const int mode, const int interval) {
        if (!ready_.load(std::memory_order_acquire)) {
            return false;
        }

        if (worker_.joinable()) {
            worker_.join();
        }

        progress_.store(0, std::memory_order_relaxed);
        {
            auto lock = report_.acquire_lock();
            *lock = {InjectionStatus::Running};
        }
        ready_.store(false, std::memory_order_release);

        try {
            worker_ = std::thread(thread_assist, mode, interval);
        } catch (...) {
            auto lock = report_.acquire_lock();
            *lock = {InjectionStatus::Failed, 0, 0, {0, 0, ERROR_NOT_ENOUGH_MEMORY}};
            ready_.store(true, std::memory_order_release);
        }
        return true;
    }

    inline static void set_input_text(const std::wstring& text) {
        auto lck = input_text_.acquire_lock();
        *lck = text;
    }

private:
    inline static std::unique_ptr<Injector> create_injector(const int mode) {
        switch (mode) {
            case 0: {
                return std::make_unique<UnicodeSender>();
            }

            default: {
                return nullptr;
            }
        }

        return nullptr;
    }

    inline static void thread_assist(const int mode, const int interval) {
        InjectionReport result{InjectionStatus::Failed};
        try {
            std::wstring text;
            {
                auto lck = input_text_.acquire_lock();
                text = *lck;
            }
            result.total_units = text.size();
            std::unique_ptr<Injector> inj = create_injector(mode);
            if (inj) {
                result = run_injection(*inj, text, interval, shared_data::sts_.get_token(),
                    [](size_t completed, size_t total) {
                        // Reserve 100% for the final successful outcome.
                        progress_.store((std::min)(99, static_cast<int>(completed * 100 / total)),
                            std::memory_order_relaxed);
                        auto lock = report_.acquire_lock();
                        lock->completed_units = completed;
                        lock->total_units = total;
                    });
            } else {
                result.last_send.error_code = ERROR_INVALID_PARAMETER;
            }
        } catch (...) {
            const auto previous = get_report();
            result.completed_units = previous.completed_units;
            result.last_send.error_code = ERROR_NOT_ENOUGH_MEMORY;
        }
        {
            auto lock = report_.acquire_lock();
            *lock = result;
        }
        if (result.status == InjectionStatus::Completed) {
            progress_.store(100, std::memory_order_relaxed);
        }
        ready_.store(true, std::memory_order_release);
    }

    inline static aop::LockBox<std::wstring> input_text_;
    inline static aop::LockBox<InjectionReport> report_;

    inline static std::atomic<int> progress_{0};
    inline static std::atomic<bool> ready_{true};
    inline static std::thread worker_;

    InjectThread() = default;
    InjectThread(const InjectThread&) = delete;
    InjectThread& operator=(const InjectThread&) = delete;
    InjectThread(InjectThread&&) = delete;
    InjectThread& operator=(InjectThread&&) = delete;

    virtual ~InjectThread() {
        if (worker_.joinable()) {
            shared_data::sts_.request_stop();
            worker_.join();
        }
    }
};

#endif // !INJECT_THREAD_HPP

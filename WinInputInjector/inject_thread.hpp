#ifndef INJECT_THREAD_HPP
#define INJECT_THREAD_HPP

#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <utility>

#include "aop.hpp"

#include "shared_data.hpp"

#include "inj_unicode.hpp"
#include "inj_keyboardsim.hpp"

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

	inline static bool launch_injection(const int mode, const int interval) {
		if (!ready_.load(std::memory_order_acquire)) {
			return false;
		}

		if (worker_.joinable()) {
			worker_.join();
		}

		progress_.store(0, std::memory_order_relaxed);
		ready_.store(false, std::memory_order_release);
	}

private:
	inline static std::unique_ptr<Injector> create_injector(const int mode) {
		switch (mode) {
			case 0:
			{
				return std::make_unique<UnicodeSender>();
			}

			default:
			{
				return nullptr;
			}
		}

		return nullptr;
	}

	inline static void thread_assist(const int mode, const int interval) {
		std::wstring text;
		{
			auto lck = input_text_.AcquireLock();
			text = *lck;
		}

		std::unique_ptr<Injector> inj = create_injector(mode);

		if (inj && !text.empty()) {
			if (interval > 0) {
				thread_assist_stepper(text, interval, inj.get());
			} else {
				thread_assist_utmost(text, interval, inj.get());
			}
		}

		progress_.store(100, std::memory_order_relaxed);
		ready_.store(true, std::memory_order_release);
	}

	inline static void thread_assist_utmost(const std::wstring& text, const int interval, Injector* inj) {
		(void) interval;
		inj->inject_wstring(text);
	}

	inline static void thread_assist_stepper(const std::wstring& text, const int interval, Injector* inj) {
		inj->set_tick_interval(interval);
		for (size_t i = 0; i < text.size(); ++i) {
			if (shared_data::sts_.stop_requested()) {
				break;
			}
			inj->tick();
			inj->inject_wchar(text[i]);
			progress_.store(static_cast<int>((static_cast<size_t>(i + 1) * 100 / text.size())), std::memory_order_relaxed);
		}
	}

	inline static aop::LockBox<std::wstring> input_text_;

	inline static std::atomic<int> progress_{ 0 };
	inline static std::atomic<bool> ready_{ true };
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
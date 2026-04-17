#ifndef _INJECT_HPP
#define _INJECT_HPP

#pragma once

#include <algorithm>
#include <chrono>
#include <thread>
#include <cstdint>

class Injector {
private:
	int64_t tick_interval_us_ = -1;
	std::chrono::high_resolution_clock::time_point last_tick_time_;

public:
	Injector() = default;

	virtual ~Injector() = default;

	virtual bool inject_wstring(const std::wstring& text) = 0;

	virtual bool inject_wchar(wchar_t ch) = 0;

	void set_tick_interval(int64_t interval_us) {
		tick_interval_us_ = std::clamp(interval_us, -1LL, 1000000LL);
	}

	void tick() {
		if (tick_interval_us_ > 0) {
			auto now = std::chrono::high_resolution_clock::now();
			auto next_tick = last_tick_time_ + std::chrono::microseconds(tick_interval_us_);
			if (now < next_tick) {
				std::this_thread::sleep_until(next_tick);
				last_tick_time_ = next_tick;
			} else {
				last_tick_time_ = now;
			}
		}
	}
};

#endif // !
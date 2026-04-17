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

private:
	

	InjectThread() = default;
	InjectThread(const InjectThread&) = delete;
	InjectThread& operator=(const InjectThread&) = delete;
	InjectThread(InjectThread&&) = delete;
	InjectThread& operator=(InjectThread&&) = delete;

	virtual ~InjectThread() {
		
	}
};

#endif // !INJECT_THREAD_HPP
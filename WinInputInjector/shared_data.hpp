#ifndef _SHARED_DATA_HPP
#define _SHARED_DATA_HPP

#pragma once

#include <exception>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <stop_token>

namespace shared_data {

std::stop_source sts_;

}

#endif // !_SHARED_DATA_HPP

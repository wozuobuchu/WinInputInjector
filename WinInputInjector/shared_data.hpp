#ifndef _SHARED_DATA_HPP
#define _SHARED_DATA_HPP

#pragma once

#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace shared_data {

    std::stop_source sts_;

}

#endif // !_SHARED_DATA_HPP

#pragma once

#include "injector.hpp"

namespace injection_settings {
    inline constexpr int MIN_CHUNK_SIZE = 1;
    inline constexpr int MAX_CHUNK_SIZE = 32768;
    inline constexpr int DEFAULT_CHUNK_SIZE = 128;

    inline int clamp_chunk_size(int value) {
        return std::clamp(value, MIN_CHUNK_SIZE, MAX_CHUNK_SIZE);
    }

    inline int parse_chunk_size(std::wstring_view text) {
        if (text.empty()) return DEFAULT_CHUNK_SIZE;
        const bool negative = text.front() == L'-';
        if (negative || text.front() == L'+') text.remove_prefix(1);
        if (text.empty()) return DEFAULT_CHUNK_SIZE;
        int value = 0;
        for (wchar_t ch : text) {
            if (ch < L'0' || ch > L'9') return DEFAULT_CHUNK_SIZE;
            // Saturate before overflow, but still validate every remaining character.
            value = (std::min)(MAX_CHUNK_SIZE, value * 10 + (ch - L'0'));
        }
        return clamp_chunk_size(negative ? -value : value);
    }
}

enum class InjectionStatus { Idle, Running, Completed, Failed, Cancelled };

struct InjectionReport {
    InjectionStatus status = InjectionStatus::Idle;
    size_t completed_units = 0; // Fully sent UTF-16 units, excluding incomplete surrogate pairs.
    size_t total_units = 0;
    InjectionResult last_send{};
};

inline bool is_surrogate_pair(wchar_t high, wchar_t low) {
    return high >= 0xD800 && high <= 0xDBFF && low >= 0xDC00 && low <= 0xDFFF;
}

inline size_t complete_prefix(std::wstring_view text, size_t sent_events) {
    size_t units = (std::min)(text.size(), sent_events / 2);
    if (units > 0 && units < text.size() && is_surrogate_pair(text[units - 1], text[units])) {
        --units;
    }
    return units;
}

// Shared by the worker and tests. The callback reports only fully sent characters.
template <typename Progress>
InjectionReport run_injection(Injector& injector, std::wstring_view text, int interval, int chunk_size,
    std::stop_token stop, Progress progress) {
    InjectionReport report{InjectionStatus::Running, 0, text.size(), {}};
    injector.set_tick_interval(interval);
    const int batch_characters = injection_settings::clamp_chunk_size(chunk_size);
    while (report.completed_units < text.size()) {
        if (!injector.tick(stop)) {
            report.status = InjectionStatus::Cancelled;
            return report;
        }
        const size_t offset = report.completed_units;
        size_t count = text.size() - offset;
        if (interval > 0) {
            count = 0;
            for (int characters = 0; characters < batch_characters && offset + count < text.size(); ++characters) {
                const size_t position = offset + count;
                count += position + 1 < text.size() && is_surrogate_pair(text[position], text[position + 1]) ? 2 : 1;
            }
        }
        const auto chunk = text.substr(offset, count);
        report.last_send = injector.inject_wstring(chunk);
        injector.mark_send_completed();
        report.completed_units += complete_prefix(chunk, report.last_send.sent_events);
        progress(report.completed_units, report.total_units);
        if (!report.last_send.succeeded()) {
            report.status = InjectionStatus::Failed;
            return report;
        }
    }
    report.status = stop.stop_requested() ? InjectionStatus::Cancelled : InjectionStatus::Completed;
    return report;
}

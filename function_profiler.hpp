#pragma once

#include <bits/stdc++.h>

class FunctionProfiler {
public:
    struct Stat {
        std::uint64_t calls = 0;
        std::chrono::nanoseconds total = std::chrono::nanoseconds::zero();
    };

    static FunctionProfiler& instance() {
        static FunctionProfiler inst;
        return inst;
    }

    void addSample(const char* name,
                   std::chrono::nanoseconds duration) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& s = stats_[name];
        s.calls++;
        s.total += duration;
    }

    static void report(std::ofstream& of) {
        auto& inst = instance();
        std::lock_guard<std::mutex> lock(inst.mutex_);

        if (inst.stats_.empty()) {
            std::cerr << "[Profiler] No samples collected.\n";
            return;
        }

        std::vector<std::pair<std::string, Stat>> vec(
            inst.stats_.begin(), inst.stats_.end());

        std::chrono::nanoseconds grandTotal = std::chrono::nanoseconds::zero();
        for (auto& [name, st] : vec) {
            grandTotal += st.total;
        }

        std::sort(vec.begin(), vec.end(),
                  [](auto const& a, auto const& b) {
                      return a.second.total > b.second.total;
                  });

        of << "\n===== Function runtime profile =====\n";
        of << std::left
                  << std::setw(40) << "Function"
                  << std::right
                  << std::setw(12) << "Calls"
                  << std::setw(16) << "Total (ms)"
                  << std::setw(16) << "Avg (us)"
                  << std::setw(10) << "%\n";

        for (auto& [name, st] : vec) {
            double total_ms = st.total.count() / 1e6;
            double avg_us =
                (st.calls ? (st.total.count() / 1e3 / static_cast<double>(st.calls))
                          : 0.0);
            double pct =
                grandTotal.count()
                    ? (100.0 * st.total.count() / grandTotal.count())
                    : 0.0;

            of << std::left << std::setw(40) << name
                      << std::right << std::setw(12) << st.calls
                      << std::setw(16) << std::fixed << std::setprecision(3)
                      << total_ms
                      << std::setw(16) << std::fixed << std::setprecision(3)
                      << avg_us
                      << std::setw(10) << std::fixed << std::setprecision(2)
                      << pct
                      << "\n";
        }
    }

private:
    FunctionProfiler() = default;

    std::mutex mutex_;
    std::map<std::string, Stat> stats_;
};

class ProfileScope {
public:
    using Clock = std::chrono::high_resolution_clock;

    explicit ProfileScope(const char* name)
        : name_(name), start_(Clock::now()) {}

    ~ProfileScope() {
        auto end = Clock::now();
        FunctionProfiler::instance().addSample(
            name_, std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_));
    }

private:
    const char* name_;
    Clock::time_point start_;
};

#define PROFILE_FUNCTION(x) ::ProfileScope profileScope__{x}

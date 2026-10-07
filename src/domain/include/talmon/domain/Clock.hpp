#pragma once

#include <chrono>

namespace talmon::domain {

class IClock {
public:
    virtual ~IClock() = default;
    [[nodiscard]] virtual std::chrono::system_clock::time_point now() const = 0;
};

class SystemClock final : public IClock {
public:
    [[nodiscard]] std::chrono::system_clock::time_point now() const override {
        return std::chrono::system_clock::now();
    }
};

} // namespace talmon::domain

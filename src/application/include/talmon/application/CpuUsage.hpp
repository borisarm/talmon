#pragma once

#include "talmon/domain/Models.hpp"

#include <algorithm>
#include <optional>

namespace talmon::application {

/// CPU utilisation (0..100) between two cumulative samples, or nullopt if it cannot be computed
/// (no elapsed time, counter reset after a reboot, ...).
[[nodiscard]] constexpr std::optional<double> cpuUsagePercent(const domain::CpuTimes& previous,
                                                              const domain::CpuTimes& current) noexcept {
    const double totalDelta = current.total() - previous.total();
    const double busyDelta = current.busyTotal() - previous.busyTotal();
    if (totalDelta <= 0.0 || busyDelta < 0.0) {
        return std::nullopt;
    }
    return std::clamp(busyDelta / totalDelta * 100.0, 0.0, 100.0);
}

} // namespace talmon::application

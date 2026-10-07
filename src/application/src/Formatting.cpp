#include "talmon/application/Formatting.hpp"

#include <array>
#include <cmath>
#include <format>

namespace talmon::application {

std::string formatBytes(std::uint64_t bytes) {
    static constexpr std::array units{"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    if (bytes < 1024) {
        return std::format("{} B", bytes);
    }
    auto value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size()) {
        value /= 1024.0;
        ++unit;
    }
    return std::format("{:.1f} {}", value, units[unit]);
}

std::string formatDuration(std::chrono::seconds duration) {
    using namespace std::chrono;
    if (duration < seconds::zero()) {
        duration = seconds::zero();
    }
    const auto d = duration_cast<days>(duration);
    const auto h = duration_cast<hours>(duration - d);
    const auto m = duration_cast<minutes>(duration - d - h);
    const auto s = duration - d - h - m;

    if (d.count() > 0) {
        return std::format("{}d {}h", d.count(), h.count());
    }
    if (h.count() > 0) {
        return std::format("{}h {}m", h.count(), m.count());
    }
    if (m.count() > 0) {
        return std::format("{}m {}s", m.count(), s.count());
    }
    return std::format("{}s", s.count());
}

std::string formatPercent(double percent) {
    if (!std::isfinite(percent)) {
        return "-";
    }
    return std::format("{:.1f} %", percent);
}

std::string formatLoad(double load1, double load5, double load15) {
    return std::format("{:.2f} / {:.2f} / {:.2f}", load1, load5, load15);
}

std::string formatCpuTime(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0) {
        return "-";
    }
    if (seconds < 60.0) {
        return std::format("{:.2f}s", seconds);
    }
    const auto total = static_cast<std::uint64_t>(seconds);
    const auto h = total / 3600;
    const auto m = (total % 3600) / 60;
    const auto s = total % 60;
    if (h > 0) {
        return std::format("{}h {:02}m {:02}s", h, m, s);
    }
    return std::format("{}m {:02}s", m, s);
}

} // namespace talmon::application

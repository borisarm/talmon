#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace talmon::application {

/// "512 B", "1.5 KiB", "3.2 GiB" ... (binary units).
[[nodiscard]] std::string formatBytes(std::uint64_t bytes);

/// "2d 3h", "3h 12m", "4m 10s", "12s".
[[nodiscard]] std::string formatDuration(std::chrono::seconds duration);

/// "42.3 %" (input is 0..100).
[[nodiscard]] std::string formatPercent(double percent);

/// "0.42 / 0.37 / 0.30".
[[nodiscard]] std::string formatLoad(double load1, double load5, double load15);

/// Seconds as "1h 02m 03.45s"-style CPU time; "0.12s" for small values.
[[nodiscard]] std::string formatCpuTime(double seconds);

} // namespace talmon::application

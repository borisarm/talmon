#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace talmon::domain {

/// Address or name of a Talos node as accepted by the `node` gRPC metadata header.
/// An empty value targets the endpoint itself.
using NodeName = std::string;

struct VersionInfo {
    std::string tag;
    std::string sha;
    std::string built;
    std::string goVersion;
    std::string os;
    std::string arch;
    std::string platformName;
    std::string platformMode;

    friend bool operator==(const VersionInfo&, const VersionInfo&) = default;
};

struct ServiceEvent {
    std::string message;
    std::string state;
    std::optional<std::chrono::system_clock::time_point> timestamp;

    friend bool operator==(const ServiceEvent&, const ServiceEvent&) = default;
};

struct ServiceHealth {
    bool unknown{true};
    bool healthy{false};
    std::string lastMessage;

    friend bool operator==(const ServiceHealth&, const ServiceHealth&) = default;
};

struct ServiceStatus {
    std::string id;
    std::string state;
    ServiceHealth health;
    std::vector<ServiceEvent> events;

    friend bool operator==(const ServiceStatus&, const ServiceStatus&) = default;
};

/// Memory figures in bytes (Talos reports /proc/meminfo in KiB; adapters convert).
struct MemoryInfo {
    std::uint64_t totalBytes{};
    std::uint64_t freeBytes{};
    std::uint64_t availableBytes{};
    std::uint64_t buffersBytes{};
    std::uint64_t cachedBytes{};
    std::uint64_t swapTotalBytes{};
    std::uint64_t swapFreeBytes{};

    [[nodiscard]] constexpr std::uint64_t usedBytes() const noexcept {
        return totalBytes > availableBytes ? totalBytes - availableBytes : 0;
    }

    [[nodiscard]] constexpr double usedRatio() const noexcept {
        return totalBytes == 0 ? 0.0 : static_cast<double>(usedBytes()) / static_cast<double>(totalBytes);
    }

    friend bool operator==(const MemoryInfo&, const MemoryInfo&) = default;
};

struct LoadAverage {
    double load1{};
    double load5{};
    double load15{};

    friend bool operator==(const LoadAverage&, const LoadAverage&) = default;
};

/// Cumulative CPU times (jiffies converted to seconds by Talos).
struct CpuTimes {
    double user{};
    double nice{};
    double system{};
    double idle{};
    double iowait{};
    double irq{};
    double softIrq{};
    double steal{};

    [[nodiscard]] constexpr double idleTotal() const noexcept { return idle + iowait; }
    [[nodiscard]] constexpr double busyTotal() const noexcept {
        return user + nice + system + irq + softIrq + steal;
    }
    [[nodiscard]] constexpr double total() const noexcept { return idleTotal() + busyTotal(); }

    friend bool operator==(const CpuTimes&, const CpuTimes&) = default;
};

struct SystemStats {
    std::chrono::system_clock::time_point bootTime{};
    CpuTimes cpuTotal;
    std::size_t cpuCount{};
    std::uint64_t processesRunning{};
    std::uint64_t processesBlocked{};
    std::uint64_t contextSwitches{};

    friend bool operator==(const SystemStats&, const SystemStats&) = default;
};

struct ProcessInfo {
    std::int32_t pid{};
    std::int32_t ppid{};
    std::string state;
    std::int32_t threads{};
    double cpuTimeSeconds{};
    std::uint64_t virtualMemoryBytes{};
    std::uint64_t residentMemoryBytes{};
    std::string command;
    std::string executable;
    std::string args;

    friend bool operator==(const ProcessInfo&, const ProcessInfo&) = default;
};

/// Parameters of a log stream for a Talos system service (containerd "system" namespace).
struct LogRequest {
    std::string serviceId;
    bool follow{true};
    std::int32_t tailLines{200};
};

} // namespace talmon::domain

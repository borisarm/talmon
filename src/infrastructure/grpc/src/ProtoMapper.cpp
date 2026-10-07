#include "talmon/infrastructure/grpc/ProtoMapper.hpp"

#include <chrono>
#include <functional>
#include <type_traits>

namespace talmon::infrastructure::grpc_adapter {

namespace {

constexpr std::uint64_t kKiB = 1024;

/// Talos wraps every unary response in a list of per-node messages. We always target a single node,
/// so we take the first one and surface the proxy error it may carry.
template <typename Response>
auto firstMessage(const Response& response, std::string_view what)
    -> domain::Result<std::reference_wrapper<const typename std::remove_cvref_t<decltype(response.messages(0))>>> {
    if (response.messages_size() == 0) {
        return domain::fail(domain::ErrorCode::Internal, std::string{what} + ": empty response");
    }
    const auto& message = response.messages(0);
    if (message.has_metadata() && !message.metadata().error().empty()) {
        return domain::fail(domain::ErrorCode::Unavailable, message.metadata().error());
    }
    return std::cref(message);
}

std::chrono::system_clock::time_point toTimePoint(const google::protobuf::Timestamp& ts) {
    return std::chrono::system_clock::time_point{
        std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds{ts.seconds()} +
                                                                        std::chrono::nanoseconds{ts.nanos()})};
}

domain::CpuTimes toDomain(const machine::CPUStat& stat) {
    return domain::CpuTimes{
        .user = stat.user(),
        .nice = stat.nice(),
        .system = stat.system(),
        .idle = stat.idle(),
        .iowait = stat.iowait(),
        .irq = stat.irq(),
        .softIrq = stat.soft_irq(),
        .steal = stat.steal(),
    };
}

} // namespace

domain::Result<domain::VersionInfo> toDomain(const machine::VersionResponse& response) {
    return firstMessage(response, "version").transform([](const machine::Version& message) {
        const auto& v = message.version();
        domain::VersionInfo info{
            .tag = v.tag(),
            .sha = v.sha(),
            .built = v.built(),
            .goVersion = v.go_version(),
            .os = v.os(),
            .arch = v.arch(),
            .platformName = {},
            .platformMode = {},
        };
        if (message.has_platform()) {
            info.platformName = message.platform().name();
            info.platformMode = message.platform().mode();
        }
        return info;
    });
}

domain::Result<std::string> toDomain(const machine::HostnameResponse& response) {
    return firstMessage(response, "hostname").transform([](const machine::Hostname& m) { return m.hostname(); });
}

domain::Result<domain::MemoryInfo> toDomain(const machine::MemoryResponse& response) {
    return firstMessage(response, "memory").transform([](const machine::Memory& message) {
        const auto& m = message.meminfo();
        return domain::MemoryInfo{
            .totalBytes = m.memtotal() * kKiB,
            .freeBytes = m.memfree() * kKiB,
            .availableBytes = m.memavailable() * kKiB,
            .buffersBytes = m.buffers() * kKiB,
            .cachedBytes = m.cached() * kKiB,
            .swapTotalBytes = m.swaptotal() * kKiB,
            .swapFreeBytes = m.swapfree() * kKiB,
        };
    });
}

domain::Result<domain::LoadAverage> toDomain(const machine::LoadAvgResponse& response) {
    return firstMessage(response, "loadavg").transform([](const machine::LoadAvg& m) {
        return domain::LoadAverage{.load1 = m.load1(), .load5 = m.load5(), .load15 = m.load15()};
    });
}

domain::Result<domain::SystemStats> toDomain(const machine::SystemStatResponse& response) {
    return firstMessage(response, "systemstat").transform([](const machine::SystemStat& m) {
        return domain::SystemStats{
            .bootTime = std::chrono::system_clock::time_point{std::chrono::seconds{m.boot_time()}},
            .cpuTotal = toDomain(m.cpu_total()),
            .cpuCount = static_cast<std::size_t>(m.cpu_size()),
            .processesRunning = m.process_running(),
            .processesBlocked = m.process_blocked(),
            .contextSwitches = m.context_switches(),
        };
    });
}

domain::Result<std::vector<domain::ServiceStatus>> toDomain(const machine::ServiceListResponse& response) {
    return firstMessage(response, "services").transform([](const machine::ServiceList& list) {
        std::vector<domain::ServiceStatus> services;
        services.reserve(static_cast<std::size_t>(list.services_size()));
        for (const auto& s : list.services()) {
            domain::ServiceStatus status{
                .id = s.id(),
                .state = s.state(),
                .health = {},
                .events = {},
            };
            if (s.has_health()) {
                status.health = domain::ServiceHealth{
                    .unknown = s.health().unknown(),
                    .healthy = s.health().healthy(),
                    .lastMessage = s.health().last_message(),
                };
            }
            for (const auto& e : s.events().events()) {
                status.events.push_back(domain::ServiceEvent{
                    .message = e.msg(),
                    .state = e.state(),
                    .timestamp = e.has_ts() ? std::optional{toTimePoint(e.ts())} : std::nullopt,
                });
            }
            services.push_back(std::move(status));
        }
        return services;
    });
}

domain::Result<std::vector<domain::ProcessInfo>> toDomain(const machine::ProcessesResponse& response) {
    return firstMessage(response, "processes").transform([](const machine::Process& message) {
        std::vector<domain::ProcessInfo> processes;
        processes.reserve(static_cast<std::size_t>(message.processes_size()));
        for (const auto& p : message.processes()) {
            processes.push_back(domain::ProcessInfo{
                .pid = p.pid(),
                .ppid = p.ppid(),
                .state = p.state(),
                .threads = p.threads(),
                .cpuTimeSeconds = p.cpu_time(),
                .virtualMemoryBytes = p.virtual_memory(),
                .residentMemoryBytes = p.resident_memory(),
                .command = p.command(),
                .executable = p.executable(),
                .args = p.args(),
            });
        }
        return processes;
    });
}

std::string streamError(const common::Data& data) {
    return data.has_metadata() ? data.metadata().error() : std::string{};
}

} // namespace talmon::infrastructure::grpc_adapter

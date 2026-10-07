#pragma once

#include "talmon/application/ServiceHealth.hpp"
#include "talmon/domain/Clock.hpp"
#include "talmon/domain/TalosApi.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace talmon::application {

/// Aggregated health/metrics snapshot of one node, ready to be presented.
struct NodeOverview {
    domain::NodeName node;
    std::string hostname;
    bool reachable{false};
    std::optional<domain::VersionInfo> version;
    std::optional<domain::MemoryInfo> memory;
    std::optional<domain::LoadAverage> load;
    std::optional<double> cpuUsagePercent;
    std::optional<std::chrono::seconds> uptime;
    std::size_t cpuCount{};
    std::optional<ServicesSummary> services;
    std::vector<std::string> errors;
};

/// Use case: collect a dashboard snapshot of a set of nodes.
/// Remembers the previous CPU sample of each node to compute utilisation between refreshes.
class ClusterMonitor {
public:
    explicit ClusterMonitor(std::shared_ptr<const domain::IClock> clock);

    /// Queries every node concurrently. Never throws; per node failures are reported in `errors`.
    [[nodiscard]] std::vector<NodeOverview> collect(domain::ITalosClient& client,
                                                    std::span<const domain::NodeName> nodes);

    [[nodiscard]] NodeOverview collectNode(domain::ITalosClient& client, const domain::NodeName& node);

    /// Forgets CPU samples (call when switching cluster/context).
    void reset();

private:
    std::optional<double> updateCpuSample(const domain::NodeName& node, const domain::CpuTimes& sample);

    std::shared_ptr<const domain::IClock> clock_;
    std::mutex mutex_;
    std::unordered_map<domain::NodeName, domain::CpuTimes> lastCpuSample_;
};

} // namespace talmon::application

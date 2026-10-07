#include "talmon/application/ClusterMonitor.hpp"

#include "talmon/application/CpuUsage.hpp"

#include <future>

namespace talmon::application {

namespace {

bool isConnectivityFailure(const domain::Error& error) noexcept {
    using enum domain::ErrorCode;
    return error.code == Unavailable || error.code == DeadlineExceeded || error.code == Unauthenticated;
}

template <typename T>
std::optional<T> capture(domain::Result<T>&& result, std::string_view what, std::vector<std::string>& errors) {
    if (result) {
        return std::move(*result);
    }
    errors.push_back(std::string{what} + ": " + result.error().describe());
    return std::nullopt;
}

} // namespace

ClusterMonitor::ClusterMonitor(std::shared_ptr<const domain::IClock> clock) : clock_{std::move(clock)} {}

std::vector<NodeOverview> ClusterMonitor::collect(domain::ITalosClient& client,
                                                  std::span<const domain::NodeName> nodes) {
    std::vector<std::future<NodeOverview>> pending;
    pending.reserve(nodes.size());
    for (const auto& node : nodes) {
        pending.push_back(std::async(std::launch::async, [this, &client, node] { return collectNode(client, node); }));
    }

    std::vector<NodeOverview> overviews;
    overviews.reserve(pending.size());
    for (auto& future : pending) {
        overviews.push_back(future.get());
    }
    return overviews;
}

NodeOverview ClusterMonitor::collectNode(domain::ITalosClient& client, const domain::NodeName& node) {
    NodeOverview overview;
    overview.node = node;

    auto version = client.version(node);
    if (!version) {
        overview.errors.push_back("version: " + version.error().describe());
        if (isConnectivityFailure(version.error())) {
            // Do not wait for one timeout per call on a node that is down.
            return overview;
        }
    } else {
        overview.version = std::move(*version);
    }
    overview.reachable = overview.version.has_value();

    if (auto hostname = capture(client.hostname(node), "hostname", overview.errors)) {
        overview.hostname = std::move(*hostname);
        overview.reachable = true;
    }
    overview.memory = capture(client.memory(node), "memory", overview.errors);
    overview.load = capture(client.loadAverage(node), "loadavg", overview.errors);

    if (auto stats = capture(client.systemStats(node), "systemstat", overview.errors)) {
        overview.cpuCount = stats->cpuCount;
        overview.cpuUsagePercent = updateCpuSample(node, stats->cpuTotal);
        if (stats->bootTime.time_since_epoch().count() > 0) {
            overview.uptime = std::chrono::duration_cast<std::chrono::seconds>(clock_->now() - stats->bootTime);
        }
    }

    if (auto services = capture(client.services(node), "services", overview.errors)) {
        overview.services = summarize(*services);
    }
    return overview;
}

void ClusterMonitor::reset() {
    std::scoped_lock lock{mutex_};
    lastCpuSample_.clear();
}

std::optional<double> ClusterMonitor::updateCpuSample(const domain::NodeName& node,
                                                      const domain::CpuTimes& sample) {
    std::scoped_lock lock{mutex_};
    auto [it, inserted] = lastCpuSample_.try_emplace(node, sample);
    if (inserted) {
        return std::nullopt;
    }
    const auto usage = cpuUsagePercent(it->second, sample);
    it->second = sample;
    return usage;
}

} // namespace talmon::application

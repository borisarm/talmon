#pragma once

#include "talmon/domain/Models.hpp"

#include <span>
#include <string_view>

namespace talmon::application {

/// High level condition of a Talos service, derived from its state and health checks.
enum class ServiceCondition {
    Healthy,  ///< Running and its health check passes.
    Running,  ///< Running without a health check (health unknown).
    Pending,  ///< Waiting / Preparing / Starting / Stopping.
    Inactive, ///< Finished or Skipped.
    Failed,   ///< Failed state, or running with a failing health check.
};

[[nodiscard]] ServiceCondition classify(const domain::ServiceStatus& service) noexcept;

[[nodiscard]] std::string_view toString(ServiceCondition condition) noexcept;

struct ServicesSummary {
    std::size_t total{};
    std::size_t healthy{};
    std::size_t running{};
    std::size_t pending{};
    std::size_t inactive{};
    std::size_t failed{};

    /// Services that are up (healthy + running without check).
    [[nodiscard]] constexpr std::size_t up() const noexcept { return healthy + running; }

    friend bool operator==(const ServicesSummary&, const ServicesSummary&) = default;
};

[[nodiscard]] ServicesSummary summarize(std::span<const domain::ServiceStatus> services) noexcept;

} // namespace talmon::application

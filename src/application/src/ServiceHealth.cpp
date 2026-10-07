#include "talmon/application/ServiceHealth.hpp"

namespace talmon::application {

ServiceCondition classify(const domain::ServiceStatus& service) noexcept {
    const std::string_view state = service.state;
    if (state == "Running") {
        if (service.health.unknown) {
            return ServiceCondition::Running;
        }
        return service.health.healthy ? ServiceCondition::Healthy : ServiceCondition::Failed;
    }
    if (state == "Failed") {
        return ServiceCondition::Failed;
    }
    if (state == "Finished" || state == "Skipped") {
        return ServiceCondition::Inactive;
    }
    return ServiceCondition::Pending;
}

std::string_view toString(ServiceCondition condition) noexcept {
    switch (condition) {
    case ServiceCondition::Healthy: return "Healthy";
    case ServiceCondition::Running: return "Running";
    case ServiceCondition::Pending: return "Pending";
    case ServiceCondition::Inactive: return "Inactive";
    case ServiceCondition::Failed: return "Failed";
    }
    return "Unknown";
}

ServicesSummary summarize(std::span<const domain::ServiceStatus> services) noexcept {
    ServicesSummary summary;
    summary.total = services.size();
    for (const auto& service : services) {
        switch (classify(service)) {
        case ServiceCondition::Healthy: ++summary.healthy; break;
        case ServiceCondition::Running: ++summary.running; break;
        case ServiceCondition::Pending: ++summary.pending; break;
        case ServiceCondition::Inactive: ++summary.inactive; break;
        case ServiceCondition::Failed: ++summary.failed; break;
        }
    }
    return summary;
}

} // namespace talmon::application

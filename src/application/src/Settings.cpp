#include "talmon/application/Settings.hpp"

namespace talmon::application {

std::optional<std::filesystem::path> resolveTalosConfigPath(const EnvironmentLookup& env) {
    if (auto explicitPath = env("TALOSCONFIG"); explicitPath && !explicitPath->empty()) {
        return std::filesystem::path{*explicitPath};
    }
    for (const auto* home : {"USERPROFILE", "HOME"}) {
        if (auto dir = env(home); dir && !dir->empty()) {
            return std::filesystem::path{*dir} / ".talos" / "config";
        }
    }
    return std::nullopt;
}

} // namespace talmon::application

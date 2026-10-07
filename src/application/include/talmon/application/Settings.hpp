#pragma once

#include "talmon/domain/Error.hpp"

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace talmon::application {

struct AppSettings {
    std::filesystem::path talosConfigPath;
    std::string context;
    std::chrono::seconds refreshInterval{5};
    std::chrono::seconds callTimeout{10};

    static constexpr std::chrono::seconds minRefreshInterval{1};
    static constexpr std::chrono::seconds maxRefreshInterval{3600};

    friend bool operator==(const AppSettings&, const AppSettings&) = default;
};

/// Port for persisting user preferences.
class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;
    [[nodiscard]] virtual AppSettings load() = 0;
    [[nodiscard]] virtual domain::Result<void> save(const AppSettings& settings) = 0;
};

/// Looks up an environment variable; injected so path resolution stays testable.
using EnvironmentLookup = std::function<std::optional<std::string>(std::string_view name)>;

/// Mirrors talosctl: $TALOSCONFIG, otherwise %USERPROFILE%\.talos\config (or $HOME/.talos/config).
[[nodiscard]] std::optional<std::filesystem::path> resolveTalosConfigPath(const EnvironmentLookup& env);

} // namespace talmon::application

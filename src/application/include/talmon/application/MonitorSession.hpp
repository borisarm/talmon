#pragma once

#include "talmon/domain/TalosApi.hpp"
#include "talmon/domain/TalosConfig.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace talmon::application {

/// Use case: load a talosconfig, pick a context and hold the resulting client.
/// Thread safe: the UI thread mutates it while background refreshes read the client.
class MonitorSession {
public:
    MonitorSession(std::shared_ptr<domain::ITalosConfigSource> configSource,
                   std::shared_ptr<domain::ITalosClientFactory> clientFactory);

    [[nodiscard]] domain::Result<void> loadConfig(const std::filesystem::path& path);

    /// Reads a talosconfig and returns its context names without touching the current connection.
    [[nodiscard]] domain::Result<std::vector<std::string>> inspectConfig(const std::filesystem::path& path) const;

    [[nodiscard]] std::vector<std::string> contextNames() const;
    /// The context marked as current in the loaded talosconfig.
    [[nodiscard]] std::string defaultContext() const;

    /// Creates a client for `contextName`, replacing the previous connection.
    [[nodiscard]] domain::Result<void> connect(std::string_view contextName,
                                               std::chrono::milliseconds callTimeout = std::chrono::seconds{10});
    void disconnect();

    [[nodiscard]] bool connected() const;
    [[nodiscard]] std::string activeContext() const;
    /// Nodes of the active context; falls back to its endpoints when no node is configured.
    [[nodiscard]] std::vector<domain::NodeName> nodes() const;
    [[nodiscard]] std::shared_ptr<domain::ITalosClient> client() const;

private:
    std::shared_ptr<domain::ITalosConfigSource> configSource_;
    std::shared_ptr<domain::ITalosClientFactory> clientFactory_;

    mutable std::mutex mutex_;
    std::optional<domain::TalosConfig> config_;
    std::optional<domain::TalosContext> activeContext_;
    std::shared_ptr<domain::ITalosClient> client_;
};

} // namespace talmon::application

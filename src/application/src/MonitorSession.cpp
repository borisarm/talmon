#include "talmon/application/MonitorSession.hpp"

#include <ranges>

namespace talmon::application {

MonitorSession::MonitorSession(std::shared_ptr<domain::ITalosConfigSource> configSource,
                               std::shared_ptr<domain::ITalosClientFactory> clientFactory)
    : configSource_{std::move(configSource)}, clientFactory_{std::move(clientFactory)} {}

domain::Result<void> MonitorSession::loadConfig(const std::filesystem::path& path) {
    auto config = configSource_->load(path);
    if (!config) {
        return std::unexpected{config.error()};
    }
    if (config->contexts.empty()) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration, "talosconfig has no contexts");
    }
    std::scoped_lock lock{mutex_};
    config_ = std::move(*config);
    activeContext_.reset();
    client_.reset();
    return {};
}

domain::Result<std::vector<std::string>> MonitorSession::inspectConfig(const std::filesystem::path& path) const {
    return configSource_->load(path).transform([](const domain::TalosConfig& config) {
        return config.contexts | std::views::transform(&domain::TalosContext::name) |
               std::ranges::to<std::vector<std::string>>();
    });
}

std::vector<std::string> MonitorSession::contextNames() const {
    std::scoped_lock lock{mutex_};
    if (!config_) {
        return {};
    }
    return config_->contexts | std::views::transform(&domain::TalosContext::name) |
           std::ranges::to<std::vector<std::string>>();
}

std::string MonitorSession::defaultContext() const {
    std::scoped_lock lock{mutex_};
    if (!config_) {
        return {};
    }
    if (config_->find(config_->currentContext) != nullptr) {
        return config_->currentContext;
    }
    return config_->contexts.front().name;
}

domain::Result<void> MonitorSession::connect(std::string_view contextName, std::chrono::milliseconds callTimeout) {
    domain::TalosContext context;
    {
        std::scoped_lock lock{mutex_};
        if (!config_) {
            return domain::fail(domain::ErrorCode::InvalidConfiguration, "no talosconfig loaded");
        }
        const auto* found = config_->find(contextName);
        if (found == nullptr) {
            return domain::fail(domain::ErrorCode::NotFound, "context '" + std::string{contextName} + "' not found");
        }
        context = *found;
    }

    if (context.endpoints.empty()) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration,
                            "context '" + context.name + "' has no endpoints");
    }

    domain::ConnectionSettings settings{
        .endpoints = context.endpoints,
        .caPem = context.caPem,
        .certificatePem = context.certificatePem,
        .privateKeyPem = context.privateKeyPem,
        .callTimeout = callTimeout,
        .tlsServerNameOverride = {},
    };
    auto client = clientFactory_->create(settings);
    if (!client) {
        return std::unexpected{client.error()};
    }

    std::scoped_lock lock{mutex_};
    activeContext_ = std::move(context);
    client_ = std::move(*client);
    return {};
}

void MonitorSession::disconnect() {
    std::scoped_lock lock{mutex_};
    activeContext_.reset();
    client_.reset();
}

bool MonitorSession::connected() const {
    std::scoped_lock lock{mutex_};
    return client_ != nullptr;
}

std::string MonitorSession::activeContext() const {
    std::scoped_lock lock{mutex_};
    return activeContext_ ? activeContext_->name : std::string{};
}

std::vector<domain::NodeName> MonitorSession::nodes() const {
    std::scoped_lock lock{mutex_};
    if (!activeContext_) {
        return {};
    }
    return activeContext_->nodes.empty() ? activeContext_->endpoints : activeContext_->nodes;
}

std::shared_ptr<domain::ITalosClient> MonitorSession::client() const {
    std::scoped_lock lock{mutex_};
    return client_;
}

} // namespace talmon::application

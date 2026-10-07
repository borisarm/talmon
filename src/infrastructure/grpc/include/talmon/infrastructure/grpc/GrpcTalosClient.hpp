#pragma once

#include "talmon/domain/TalosApi.hpp"

#include <grpcpp/channel.h>
#include <grpcpp/support/status.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace talmon::infrastructure::grpc_adapter {

/// Talos apid listens on 50000. Adds the port when missing; handles bare and bracketed IPv6.
[[nodiscard]] std::string normalizeEndpoint(std::string_view endpoint);

/// Talos writes Ed25519 keys as PKCS#8 under a non standard "ED25519 PRIVATE KEY" PEM label that
/// OpenSSL refuses; relabel it as plain "PRIVATE KEY". Other keys are returned unchanged.
[[nodiscard]] std::string normalizePrivateKeyPem(std::string pem);

[[nodiscard]] domain::Error toDomainError(const grpc::Status& status);

/// ITalosClient implemented directly on top of the Talos MachineService gRPC API.
class GrpcTalosClient final : public domain::ITalosClient {
public:
    GrpcTalosClient(std::shared_ptr<grpc::Channel> channel, std::chrono::milliseconds callTimeout);
    ~GrpcTalosClient() override;

    [[nodiscard]] domain::Result<domain::VersionInfo> version(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<std::string> hostname(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<domain::MemoryInfo> memory(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<domain::LoadAverage> loadAverage(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<domain::SystemStats> systemStats(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<std::vector<domain::ServiceStatus>> services(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<std::vector<domain::ProcessInfo>> processes(const domain::NodeName& node) override;
    [[nodiscard]] domain::Result<void> streamLogs(const domain::NodeName& node, const domain::LogRequest& request,
                                                  std::stop_token stop,
                                                  const domain::LogChunkHandler& onChunk) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// Creates mutual-TLS channels from talosconfig credentials.
class GrpcTalosClientFactory final : public domain::ITalosClientFactory {
public:
    [[nodiscard]] domain::Result<std::shared_ptr<domain::ITalosClient>> create(
        const domain::ConnectionSettings& settings) override;
};

} // namespace talmon::infrastructure::grpc_adapter

#pragma once

// Ports (Dependency Inversion): the application layer only depends on these abstractions.
// They are segregated by concern (Interface Segregation) so that each consumer — and each
// test double — only needs to know about the calls it actually uses.

#include "talmon/domain/Error.hpp"
#include "talmon/domain/Models.hpp"

#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

namespace talmon::domain {

class IMachineInfoApi {
public:
    virtual ~IMachineInfoApi() = default;
    [[nodiscard]] virtual Result<VersionInfo> version(const NodeName& node) = 0;
    [[nodiscard]] virtual Result<std::string> hostname(const NodeName& node) = 0;
};

class ISystemMetricsApi {
public:
    virtual ~ISystemMetricsApi() = default;
    [[nodiscard]] virtual Result<MemoryInfo> memory(const NodeName& node) = 0;
    [[nodiscard]] virtual Result<LoadAverage> loadAverage(const NodeName& node) = 0;
    [[nodiscard]] virtual Result<SystemStats> systemStats(const NodeName& node) = 0;
};

class IServiceApi {
public:
    virtual ~IServiceApi() = default;
    [[nodiscard]] virtual Result<std::vector<ServiceStatus>> services(const NodeName& node) = 0;
};

class IProcessApi {
public:
    virtual ~IProcessApi() = default;
    [[nodiscard]] virtual Result<std::vector<ProcessInfo>> processes(const NodeName& node) = 0;
};

/// Receives raw chunks of a log stream as they arrive (not necessarily line aligned).
using LogChunkHandler = std::function<void(std::string_view chunk)>;

class ILogApi {
public:
    virtual ~ILogApi() = default;
    /// Blocks until the stream ends, fails, or `stop` is requested (which yields Cancelled).
    [[nodiscard]] virtual Result<void> streamLogs(const NodeName& node, const LogRequest& request,
                                                  std::stop_token stop, const LogChunkHandler& onChunk) = 0;
};

/// Full client facade aggregating the segregated APIs. Implementations must be thread safe.
class ITalosClient : public IMachineInfoApi,
                     public ISystemMetricsApi,
                     public IServiceApi,
                     public IProcessApi,
                     public ILogApi {};

/// Everything needed to open a mutual-TLS connection to apid.
struct ConnectionSettings {
    std::vector<std::string> endpoints;
    std::string caPem;
    std::string certificatePem;
    std::string privateKeyPem;
    std::chrono::milliseconds callTimeout{std::chrono::seconds{10}};
    /// Optional TLS server name override (useful when connecting through a tunnel / NAT).
    std::string tlsServerNameOverride;
};

class ITalosClientFactory {
public:
    virtual ~ITalosClientFactory() = default;
    [[nodiscard]] virtual Result<std::shared_ptr<ITalosClient>> create(const ConnectionSettings& settings) = 0;
};

} // namespace talmon::domain

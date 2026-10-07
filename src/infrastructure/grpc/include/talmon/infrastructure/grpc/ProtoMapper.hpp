#pragma once

// Anti-corruption layer: converts Talos protobuf messages into domain types.

#include "talmon/domain/Error.hpp"
#include "talmon/domain/Models.hpp"

#include <machine/machine.pb.h>

#include <string>
#include <vector>

namespace talmon::infrastructure::grpc_adapter {

[[nodiscard]] domain::Result<domain::VersionInfo> toDomain(const machine::VersionResponse& response);
[[nodiscard]] domain::Result<std::string> toDomain(const machine::HostnameResponse& response);
[[nodiscard]] domain::Result<domain::MemoryInfo> toDomain(const machine::MemoryResponse& response);
[[nodiscard]] domain::Result<domain::LoadAverage> toDomain(const machine::LoadAvgResponse& response);
[[nodiscard]] domain::Result<domain::SystemStats> toDomain(const machine::SystemStatResponse& response);
[[nodiscard]] domain::Result<std::vector<domain::ServiceStatus>> toDomain(const machine::ServiceListResponse& response);
[[nodiscard]] domain::Result<std::vector<domain::ProcessInfo>> toDomain(const machine::ProcessesResponse& response);

/// Error reported by apid inside a streamed chunk's metadata (empty when none).
[[nodiscard]] std::string streamError(const common::Data& data);

} // namespace talmon::infrastructure::grpc_adapter

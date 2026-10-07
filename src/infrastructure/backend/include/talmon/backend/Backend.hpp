#pragma once

// Binary boundary between the UI and the infrastructure.
//
// The WinUI executable only sees the domain ports; gRPC, protobuf, OpenSSL and yaml-cpp live
// inside talmon_backend.dll. Objects cross the boundary exclusively through abstract interfaces
// (vtables + virtual destructors), so no third-party header leaks into the UI project.

#include "talmon/domain/TalosApi.hpp"
#include "talmon/domain/TalosConfig.hpp"

#include <memory>

#if defined(TALMON_BACKEND_EXPORTS)
#define TALMON_BACKEND_API __declspec(dllexport)
#else
#define TALMON_BACKEND_API __declspec(dllimport)
#endif

namespace talmon::backend {

[[nodiscard]] TALMON_BACKEND_API std::shared_ptr<domain::ITalosClientFactory> createTalosClientFactory();
[[nodiscard]] TALMON_BACKEND_API std::shared_ptr<domain::ITalosConfigSource> createTalosConfigSource();

} // namespace talmon::backend

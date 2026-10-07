#include "talmon/backend/Backend.hpp"

#include "talmon/infrastructure/TalosConfigParser.hpp"
#include "talmon/infrastructure/grpc/GrpcTalosClient.hpp"

namespace talmon::backend {

std::shared_ptr<domain::ITalosClientFactory> createTalosClientFactory() {
    return std::make_shared<infrastructure::grpc_adapter::GrpcTalosClientFactory>();
}

std::shared_ptr<domain::ITalosConfigSource> createTalosConfigSource() {
    return std::make_shared<infrastructure::YamlTalosConfigSource>();
}

} // namespace talmon::backend

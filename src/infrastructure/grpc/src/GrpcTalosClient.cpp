#include "talmon/infrastructure/grpc/GrpcTalosClient.hpp"

#include "talmon/infrastructure/grpc/ProtoMapper.hpp"

#include <machine/machine.grpc.pb.h>

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <algorithm>
#include <mutex>

namespace talmon::infrastructure::grpc_adapter {

namespace {

constexpr std::string_view kDefaultPort = "50000";
constexpr std::string_view kEd25519Label = "ED25519 PRIVATE KEY";

void replaceAll(std::string& text, std::string_view from, std::string_view to) {
    for (auto pos = text.find(from); pos != std::string::npos; pos = text.find(from, pos + to.size())) {
        text.replace(pos, from.size(), to);
    }
}

} // namespace

std::string normalizeEndpoint(std::string_view endpoint) {
    std::string value{endpoint};
    // Strip an optional scheme such as "https://".
    if (const auto scheme = value.find("://"); scheme != std::string::npos) {
        value.erase(0, scheme + 3);
    }
    if (value.empty()) {
        return value;
    }
    if (value.front() == '[') {
        // [v6] or [v6]:port
        const auto close = value.find(']');
        if (close != std::string::npos && close + 1 == value.size()) {
            value += ':';
            value += kDefaultPort;
        }
        return value;
    }
    const auto colons = std::ranges::count(value, ':');
    if (colons == 0) {
        value += ':';
        value += kDefaultPort;
    } else if (colons > 1) {
        // Bare IPv6 address without port.
        value = "[" + value + "]:" + std::string{kDefaultPort};
    }
    return value;
}

std::string normalizePrivateKeyPem(std::string pem) {
    replaceAll(pem, kEd25519Label, "PRIVATE KEY");
    return pem;
}

domain::Error toDomainError(const grpc::Status& status) {
    using enum domain::ErrorCode;
    domain::ErrorCode code = Unknown;
    switch (status.error_code()) {
    case grpc::StatusCode::INVALID_ARGUMENT:
    case grpc::StatusCode::FAILED_PRECONDITION:
    case grpc::StatusCode::OUT_OF_RANGE: code = InvalidArgument; break;
    case grpc::StatusCode::NOT_FOUND: code = NotFound; break;
    case grpc::StatusCode::UNAVAILABLE: code = Unavailable; break;
    case grpc::StatusCode::UNAUTHENTICATED: code = Unauthenticated; break;
    case grpc::StatusCode::PERMISSION_DENIED: code = PermissionDenied; break;
    case grpc::StatusCode::DEADLINE_EXCEEDED: code = DeadlineExceeded; break;
    case grpc::StatusCode::CANCELLED: code = Cancelled; break;
    case grpc::StatusCode::INTERNAL:
    case grpc::StatusCode::DATA_LOSS:
    case grpc::StatusCode::UNIMPLEMENTED: code = Internal; break;
    default: code = Unknown; break;
    }
    return domain::Error{code, status.error_message()};
}

struct GrpcTalosClient::Impl {
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<machine::MachineService::Stub> stub;
    std::chrono::milliseconds timeout;

    void prepare(grpc::ClientContext& context, const domain::NodeName& node, bool withDeadline) const {
        if (withDeadline) {
            context.set_deadline(std::chrono::system_clock::now() + timeout);
        }
        if (!node.empty()) {
            // apid proxies the call to the given node.
            context.AddMetadata("node", node);
        }
    }

    /// Runs a unary RPC and maps its response through the anti-corruption layer.
    template <typename Response, typename Invoke>
    auto unary(const domain::NodeName& node, Invoke&& invoke) const -> decltype(toDomain(std::declval<Response>())) {
        grpc::ClientContext context;
        prepare(context, node, true);
        Response response;
        const grpc::Status status = std::forward<Invoke>(invoke)(&context, &response);
        if (!status.ok()) {
            return std::unexpected{toDomainError(status)};
        }
        return toDomain(response);
    }
};

GrpcTalosClient::GrpcTalosClient(std::shared_ptr<grpc::Channel> channel, std::chrono::milliseconds callTimeout)
    : impl_{std::make_unique<Impl>()} {
    impl_->stub = machine::MachineService::NewStub(channel);
    impl_->channel = std::move(channel);
    impl_->timeout = callTimeout;
}

GrpcTalosClient::~GrpcTalosClient() = default;

domain::Result<domain::VersionInfo> GrpcTalosClient::version(const domain::NodeName& node) {
    return impl_->unary<machine::VersionResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->Version(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<std::string> GrpcTalosClient::hostname(const domain::NodeName& node) {
    return impl_->unary<machine::HostnameResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->Hostname(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<domain::MemoryInfo> GrpcTalosClient::memory(const domain::NodeName& node) {
    return impl_->unary<machine::MemoryResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->Memory(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<domain::LoadAverage> GrpcTalosClient::loadAverage(const domain::NodeName& node) {
    return impl_->unary<machine::LoadAvgResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->LoadAvg(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<domain::SystemStats> GrpcTalosClient::systemStats(const domain::NodeName& node) {
    return impl_->unary<machine::SystemStatResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->SystemStat(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<std::vector<domain::ServiceStatus>> GrpcTalosClient::services(const domain::NodeName& node) {
    return impl_->unary<machine::ServiceListResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->ServiceList(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<std::vector<domain::ProcessInfo>> GrpcTalosClient::processes(const domain::NodeName& node) {
    return impl_->unary<machine::ProcessesResponse>(node, [&](auto* ctx, auto* resp) {
        return impl_->stub->Processes(ctx, google::protobuf::Empty{}, resp);
    });
}

domain::Result<void> GrpcTalosClient::streamLogs(const domain::NodeName& node, const domain::LogRequest& request,
                                                 std::stop_token stop, const domain::LogChunkHandler& onChunk) {
    if (request.serviceId.empty()) {
        return domain::fail(domain::ErrorCode::InvalidArgument, "service id is required");
    }

    grpc::ClientContext context;
    // A followed stream is open-ended: no deadline, cancellation comes from `stop`.
    impl_->prepare(context, node, !request.follow);

    machine::LogsRequest logsRequest;
    logsRequest.set_namespace_("system");
    logsRequest.set_id(request.serviceId);
    logsRequest.set_driver(common::CONTAINERD);
    logsRequest.set_follow(request.follow);
    logsRequest.set_tail_lines(request.tailLines);

    std::stop_callback cancelOnStop{stop, [&context] { context.TryCancel(); }};
    if (stop.stop_requested()) {
        return domain::fail(domain::ErrorCode::Cancelled, "log stream cancelled");
    }

    auto reader = impl_->stub->Logs(&context, logsRequest);
    common::Data chunk;
    while (reader->Read(&chunk)) {
        if (auto error = streamError(chunk); !error.empty()) {
            context.TryCancel();
            reader->Finish();
            return domain::fail(domain::ErrorCode::Unavailable, std::move(error));
        }
        if (!chunk.bytes().empty()) {
            onChunk(chunk.bytes());
        }
    }

    const grpc::Status status = reader->Finish();
    if (stop.stop_requested()) {
        return domain::fail(domain::ErrorCode::Cancelled, "log stream cancelled");
    }
    if (!status.ok()) {
        return std::unexpected{toDomainError(status)};
    }
    return {};
}

domain::Result<std::shared_ptr<domain::ITalosClient>> GrpcTalosClientFactory::create(
    const domain::ConnectionSettings& settings) {
    if (settings.endpoints.empty()) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration, "no endpoints configured");
    }
    if (settings.caPem.empty() || settings.certificatePem.empty() || settings.privateKeyPem.empty()) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration, "client TLS credentials are incomplete");
    }

    grpc::SslCredentialsOptions tls;
    tls.pem_root_certs = settings.caPem;
    tls.pem_cert_chain = settings.certificatePem;
    tls.pem_private_key = normalizePrivateKeyPem(settings.privateKeyPem);

    grpc::ChannelArguments arguments;
    arguments.SetMaxReceiveMessageSize(64 * 1024 * 1024);
    arguments.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 30'000);
    if (!settings.tlsServerNameOverride.empty()) {
        arguments.SetSslTargetNameOverride(settings.tlsServerNameOverride);
    }

    // apid answers on every control plane endpoint; the first one is used as the entry point
    // and per call routing to a node is done through the `node` metadata header.
    auto channel =
        grpc::CreateCustomChannel(normalizeEndpoint(settings.endpoints.front()), grpc::SslCredentials(tls), arguments);
    if (!channel) {
        return domain::fail(domain::ErrorCode::Internal, "could not create gRPC channel");
    }
    return std::make_shared<GrpcTalosClient>(std::move(channel), settings.callTimeout);
}

} // namespace talmon::infrastructure::grpc_adapter

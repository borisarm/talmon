// Exercises GrpcTalosClient end to end against an in-process fake of the Talos MachineService.

#include "talmon/infrastructure/grpc/GrpcTalosClient.hpp"

#include <machine/machine.grpc.pb.h>

#include <gmock/gmock.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>

using namespace talmon;
using namespace talmon::infrastructure::grpc_adapter;
using namespace std::chrono_literals;
using ::testing::ElementsAre;

namespace {

std::string metadataValue(const grpc::ServerContext& context, std::string_view key) {
    const auto& metadata = context.client_metadata();
    const auto it = metadata.find(grpc::string_ref{key.data(), key.size()});
    return it == metadata.end() ? std::string{} : std::string{it->second.data(), it->second.size()};
}

class FakeMachineService final : public machine::MachineService::Service {
public:
    grpc::Status Version(grpc::ServerContext* context, const google::protobuf::Empty*,
                         machine::VersionResponse* response) override {
        remember(*context);
        response->add_messages()->mutable_version()->set_tag("v1.14.2");
        return grpc::Status::OK;
    }

    grpc::Status Hostname(grpc::ServerContext* context, const google::protobuf::Empty*,
                          machine::HostnameResponse*) override {
        remember(*context);
        return grpc::Status{grpc::StatusCode::PERMISSION_DENIED, "not allowed"};
    }

    grpc::Status Memory(grpc::ServerContext*, const google::protobuf::Empty*,
                        machine::MemoryResponse* response) override {
        response->add_messages()->mutable_meminfo()->set_memtotal(4);
        return grpc::Status::OK;
    }

    grpc::Status LoadAvg(grpc::ServerContext*, const google::protobuf::Empty*, machine::LoadAvgResponse*) override {
        std::this_thread::sleep_for(2s); // longer than the client deadline
        return grpc::Status::OK;
    }

    grpc::Status Logs(grpc::ServerContext* context, const machine::LogsRequest* request,
                      grpc::ServerWriter<common::Data>* writer) override {
        {
            std::scoped_lock lock{mutex_};
            lastLogsRequest_ = *request;
        }
        common::Data data;
        data.set_bytes("line 1\nline");
        writer->Write(data);
        data.set_bytes(" 2\n");
        writer->Write(data);
        if (!request->follow()) {
            return grpc::Status::OK;
        }
        // Follow mode: keep streaming until the client cancels.
        while (!context->IsCancelled()) {
            std::this_thread::sleep_for(10ms);
        }
        return grpc::Status::CANCELLED;
    }

    std::string lastNode() {
        std::scoped_lock lock{mutex_};
        return lastNode_;
    }
    machine::LogsRequest lastLogsRequest() {
        std::scoped_lock lock{mutex_};
        return lastLogsRequest_;
    }

private:
    void remember(const grpc::ServerContext& context) {
        std::scoped_lock lock{mutex_};
        lastNode_ = metadataValue(context, "node");
    }

    std::mutex mutex_;
    std::string lastNode_;
    machine::LogsRequest lastLogsRequest_;
};

class GrpcTalosClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        grpc::ServerBuilder builder;
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        ASSERT_NE(server, nullptr);
        client = std::make_unique<GrpcTalosClient>(server->InProcessChannel(grpc::ChannelArguments{}), 500ms);
    }

    void TearDown() override {
        client.reset();
        server->Shutdown(std::chrono::system_clock::now() + 1s);
    }

    FakeMachineService service;
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<GrpcTalosClient> client;
};

} // namespace

TEST_F(GrpcTalosClientTest, UnaryCallReturnsMappedValueAndRoutesToNode) {
    const auto version = client->version("10.0.0.7");

    ASSERT_TRUE(version) << version.error().describe();
    EXPECT_EQ(version->tag, "v1.14.2");
    EXPECT_EQ(service.lastNode(), "10.0.0.7");
}

TEST_F(GrpcTalosClientTest, EmptyNodeSendsNoRoutingMetadata) {
    ASSERT_TRUE(client->version(""));
    EXPECT_EQ(service.lastNode(), "");
}

TEST_F(GrpcTalosClientTest, MapsGrpcStatusToDomainError) {
    const auto hostname = client->hostname("n");
    ASSERT_FALSE(hostname);
    EXPECT_EQ(hostname.error(), (domain::Error{domain::ErrorCode::PermissionDenied, "not allowed"}));
}

TEST_F(GrpcTalosClientTest, AppliesCallDeadline) {
    const auto load = client->loadAverage("n");
    ASSERT_FALSE(load);
    EXPECT_EQ(load.error().code, domain::ErrorCode::DeadlineExceeded);
}

TEST_F(GrpcTalosClientTest, StreamsLogsOfSystemService) {
    std::string received;
    const auto outcome = client->streamLogs("n", {.serviceId = "etcd", .follow = false, .tailLines = 50}, {},
                                            [&](std::string_view chunk) { received += chunk; });

    ASSERT_TRUE(outcome) << outcome.error().describe();
    EXPECT_EQ(received, "line 1\nline 2\n");
    const auto request = service.lastLogsRequest();
    EXPECT_EQ(request.namespace_(), "system");
    EXPECT_EQ(request.id(), "etcd");
    EXPECT_EQ(request.tail_lines(), 50);
    EXPECT_EQ(request.driver(), common::CONTAINERD);
}

TEST_F(GrpcTalosClientTest, StopTokenCancelsFollowedStream) {
    std::stop_source stop;
    std::atomic<int> chunks{0};
    std::jthread canceller{[&] {
        while (chunks.load() < 2) {
            std::this_thread::sleep_for(5ms);
        }
        stop.request_stop();
    }};

    const auto outcome = client->streamLogs("n", {.serviceId = "kubelet", .follow = true, .tailLines = 10},
                                            stop.get_token(), [&](std::string_view) { ++chunks; });

    ASSERT_FALSE(outcome);
    EXPECT_EQ(outcome.error().code, domain::ErrorCode::Cancelled);
}

TEST_F(GrpcTalosClientTest, StreamRequiresServiceId) {
    EXPECT_EQ(client->streamLogs("n", {}, {}, [](std::string_view) {}).error().code,
              domain::ErrorCode::InvalidArgument);
}

TEST(NormalizeEndpoint, AddsDefaultTalosPort) {
    EXPECT_EQ(normalizeEndpoint("10.0.0.1"), "10.0.0.1:50000");
    EXPECT_EQ(normalizeEndpoint("cp.example.com"), "cp.example.com:50000");
    EXPECT_EQ(normalizeEndpoint("10.0.0.1:6443"), "10.0.0.1:6443");
    EXPECT_EQ(normalizeEndpoint("https://cp.example.com"), "cp.example.com:50000");
}

TEST(NormalizeEndpoint, HandlesIpv6) {
    EXPECT_EQ(normalizeEndpoint("fd00::1"), "[fd00::1]:50000");
    EXPECT_EQ(normalizeEndpoint("[fd00::1]"), "[fd00::1]:50000");
    EXPECT_EQ(normalizeEndpoint("[fd00::1]:50001"), "[fd00::1]:50001");
}

TEST(NormalizePrivateKeyPem, RelabelsTalosEd25519Keys) {
    EXPECT_EQ(normalizePrivateKeyPem("-----BEGIN ED25519 PRIVATE KEY-----\nAAAA\n-----END ED25519 PRIVATE KEY-----\n"),
              "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----\n");
    EXPECT_EQ(normalizePrivateKeyPem("-----BEGIN EC PRIVATE KEY-----"), "-----BEGIN EC PRIVATE KEY-----");
}

TEST(GrpcTalosClientFactory, ValidatesSettings) {
    GrpcTalosClientFactory factory;
    EXPECT_EQ(factory.create({}).error().code, domain::ErrorCode::InvalidConfiguration);
    EXPECT_EQ(factory.create({.endpoints = {"10.0.0.1"}}).error().code, domain::ErrorCode::InvalidConfiguration);
}

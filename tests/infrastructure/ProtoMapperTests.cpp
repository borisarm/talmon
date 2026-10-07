#include "talmon/infrastructure/grpc/ProtoMapper.hpp"

#include <gtest/gtest.h>

using namespace talmon;
using namespace talmon::infrastructure::grpc_adapter;

TEST(ProtoMapper, MapsVersionAndPlatform) {
    machine::VersionResponse response;
    auto* message = response.add_messages();
    message->mutable_version()->set_tag("v1.14.2");
    message->mutable_version()->set_arch("amd64");
    message->mutable_version()->set_go_version("go1.25");
    message->mutable_platform()->set_name("metal");
    message->mutable_platform()->set_mode("metal");

    const auto version = toDomain(response);

    ASSERT_TRUE(version);
    EXPECT_EQ(version->tag, "v1.14.2");
    EXPECT_EQ(version->arch, "amd64");
    EXPECT_EQ(version->goVersion, "go1.25");
    EXPECT_EQ(version->platformName, "metal");
}

TEST(ProtoMapper, EmptyResponseIsAnError) {
    const auto version = toDomain(machine::VersionResponse{});
    ASSERT_FALSE(version);
    EXPECT_EQ(version.error().code, domain::ErrorCode::Internal);
}

TEST(ProtoMapper, ProxyErrorInMetadataIsUnavailable) {
    machine::HostnameResponse response;
    auto* message = response.add_messages();
    message->mutable_metadata()->set_error("rpc error: connection refused");
    message->set_hostname("ignored");

    const auto hostname = toDomain(response);

    ASSERT_FALSE(hostname);
    EXPECT_EQ(hostname.error(), (domain::Error{domain::ErrorCode::Unavailable, "rpc error: connection refused"}));
}

TEST(ProtoMapper, ConvertsMemoryFromKibibytes) {
    machine::MemoryResponse response;
    auto* info = response.add_messages()->mutable_meminfo();
    info->set_memtotal(2048);
    info->set_memavailable(512);
    info->set_swaptotal(1);

    const auto memory = toDomain(response);

    ASSERT_TRUE(memory);
    EXPECT_EQ(memory->totalBytes, 2048u * 1024u);
    EXPECT_EQ(memory->availableBytes, 512u * 1024u);
    EXPECT_EQ(memory->swapTotalBytes, 1024u);
    EXPECT_DOUBLE_EQ(memory->usedRatio(), 0.75);
}

TEST(ProtoMapper, MapsSystemStat) {
    machine::SystemStatResponse response;
    auto* stat = response.add_messages();
    stat->set_boot_time(1'700'000'000);
    stat->mutable_cpu_total()->set_user(12.5);
    stat->mutable_cpu_total()->set_idle(80);
    stat->add_cpu();
    stat->add_cpu();
    stat->set_process_running(3);

    const auto stats = toDomain(response);

    ASSERT_TRUE(stats);
    EXPECT_EQ(stats->bootTime.time_since_epoch(), std::chrono::seconds{1'700'000'000});
    EXPECT_DOUBLE_EQ(stats->cpuTotal.user, 12.5);
    EXPECT_DOUBLE_EQ(stats->cpuTotal.idle, 80);
    EXPECT_EQ(stats->cpuCount, 2u);
    EXPECT_EQ(stats->processesRunning, 3u);
}

TEST(ProtoMapper, MapsServicesWithHealthAndEvents) {
    machine::ServiceListResponse response;
    auto* list = response.add_messages();
    auto* etcd = list->add_services();
    etcd->set_id("etcd");
    etcd->set_state("Running");
    etcd->mutable_health()->set_healthy(true);
    etcd->mutable_health()->set_last_message("ok");
    auto* event = etcd->mutable_events()->add_events();
    event->set_msg("Health check successful");
    event->set_state("Running");
    event->mutable_ts()->set_seconds(10);
    list->add_services()->set_id("cri"); // no health -> unknown

    const auto services = toDomain(response);

    ASSERT_TRUE(services);
    ASSERT_EQ(services->size(), 2u);
    const auto& first = services->at(0);
    EXPECT_EQ(first.id, "etcd");
    EXPECT_FALSE(first.health.unknown);
    EXPECT_TRUE(first.health.healthy);
    EXPECT_EQ(first.health.lastMessage, "ok");
    ASSERT_EQ(first.events.size(), 1u);
    EXPECT_EQ(first.events[0].message, "Health check successful");
    EXPECT_EQ(first.events[0].timestamp->time_since_epoch(), std::chrono::seconds{10});
    EXPECT_TRUE(services->at(1).health.unknown);
}

TEST(ProtoMapper, MapsProcesses) {
    machine::ProcessesResponse response;
    auto* p = response.add_messages()->add_processes();
    p->set_pid(42);
    p->set_ppid(1);
    p->set_state("S");
    p->set_threads(12);
    p->set_cpu_time(3.5);
    p->set_resident_memory(4096);
    p->set_command("kubelet");
    p->set_args("--v=2");

    const auto processes = toDomain(response);

    ASSERT_TRUE(processes);
    ASSERT_EQ(processes->size(), 1u);
    EXPECT_EQ(processes->front(), (domain::ProcessInfo{.pid = 42,
                                                       .ppid = 1,
                                                       .state = "S",
                                                       .threads = 12,
                                                       .cpuTimeSeconds = 3.5,
                                                       .virtualMemoryBytes = 0,
                                                       .residentMemoryBytes = 4096,
                                                       .command = "kubelet",
                                                       .executable = "",
                                                       .args = "--v=2"}));
}

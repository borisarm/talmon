#include "talmon/application/ClusterMonitor.hpp"

#include "fakes/MockTalosClient.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace talmon;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

const auto kNow = std::chrono::system_clock::time_point{std::chrono::seconds{1'700'000'000}};

class ClusterMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        ON_CALL(client, version(_)).WillByDefault(Return(domain::VersionInfo{.tag = "v1.14.2"}));
        ON_CALL(client, hostname(_)).WillByDefault(Return(std::string{"cp-1"}));
        ON_CALL(client, memory(_)).WillByDefault(Return(domain::MemoryInfo{.totalBytes = 1000, .availableBytes = 250}));
        ON_CALL(client, loadAverage(_)).WillByDefault(Return(domain::LoadAverage{0.5, 0.4, 0.3}));
        ON_CALL(client, services(_))
            .WillByDefault(Return(std::vector<domain::ServiceStatus>{
                {.id = "etcd", .state = "Running", .health = {false, true, {}}, .events = {}},
                {.id = "kubelet", .state = "Failed", .health = {}, .events = {}},
            }));
        ON_CALL(client, systemStats(_)).WillByDefault(Return(stats(domain::CpuTimes{.user = 10, .idle = 90})));
    }

    static domain::SystemStats stats(domain::CpuTimes cpu) {
        return domain::SystemStats{.bootTime = kNow - 2h, .cpuTotal = cpu, .cpuCount = 4};
    }

    NiceMock<fakes::MockTalosClient> client;
    application::ClusterMonitor monitor{std::make_shared<fakes::FixedClock>(kNow)};
};

} // namespace

TEST_F(ClusterMonitorTest, AggregatesEveryMetricOfAHealthyNode) {
    const auto overview = monitor.collectNode(client, "10.0.0.2");

    EXPECT_EQ(overview.node, "10.0.0.2");
    EXPECT_TRUE(overview.reachable);
    EXPECT_EQ(overview.hostname, "cp-1");
    ASSERT_TRUE(overview.version);
    EXPECT_EQ(overview.version->tag, "v1.14.2");
    ASSERT_TRUE(overview.memory);
    EXPECT_EQ(overview.memory->usedBytes(), 750u);
    EXPECT_EQ(overview.uptime, std::chrono::seconds{2h});
    EXPECT_EQ(overview.cpuCount, 4u);
    ASSERT_TRUE(overview.services);
    EXPECT_EQ(overview.services->healthy, 1u);
    EXPECT_EQ(overview.services->failed, 1u);
    EXPECT_TRUE(overview.errors.empty());
}

TEST_F(ClusterMonitorTest, ComputesCpuUsageFromConsecutiveSamples) {
    EXPECT_CALL(client, systemStats("n1"))
        .WillOnce(Return(stats({.user = 10, .idle = 90})))
        .WillOnce(Return(stats({.user = 40, .idle = 160}))); // +30 busy / +100 total

    EXPECT_FALSE(monitor.collectNode(client, "n1").cpuUsagePercent.has_value());
    const auto second = monitor.collectNode(client, "n1");

    ASSERT_TRUE(second.cpuUsagePercent);
    EXPECT_DOUBLE_EQ(*second.cpuUsagePercent, 30.0);
}

TEST_F(ClusterMonitorTest, ResetForgetsPreviousSamples) {
    (void)monitor.collectNode(client, "n1");
    monitor.reset();
    EXPECT_FALSE(monitor.collectNode(client, "n1").cpuUsagePercent.has_value());
}

TEST_F(ClusterMonitorTest, StopsEarlyWhenNodeIsUnreachable) {
    EXPECT_CALL(client, version("down"))
        .WillOnce(Return(domain::fail(domain::ErrorCode::Unavailable, "connection refused")));
    EXPECT_CALL(client, hostname(_)).Times(0);
    EXPECT_CALL(client, memory(_)).Times(0);

    const auto overview = monitor.collectNode(client, "down");

    EXPECT_FALSE(overview.reachable);
    ASSERT_EQ(overview.errors.size(), 1u);
    EXPECT_THAT(overview.errors.front(), ::testing::HasSubstr("connection refused"));
}

TEST_F(ClusterMonitorTest, KeepsPartialDataWhenSingleCallsFail) {
    EXPECT_CALL(client, memory(_)).WillOnce(Return(domain::fail(domain::ErrorCode::PermissionDenied, "rbac")));

    const auto overview = monitor.collectNode(client, "n1");

    EXPECT_TRUE(overview.reachable);
    EXPECT_FALSE(overview.memory);
    EXPECT_TRUE(overview.load);
    ASSERT_EQ(overview.errors.size(), 1u);
    EXPECT_THAT(overview.errors.front(), ::testing::StartsWith("memory: PermissionDenied"));
}

TEST_F(ClusterMonitorTest, CollectsAllNodesPreservingOrder) {
    const std::vector<domain::NodeName> nodes{"a", "b", "c"};

    const auto overviews = monitor.collect(client, nodes);

    ASSERT_EQ(overviews.size(), 3u);
    EXPECT_EQ(overviews[0].node, "a");
    EXPECT_EQ(overviews[1].node, "b");
    EXPECT_EQ(overviews[2].node, "c");
}

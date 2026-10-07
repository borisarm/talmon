#include "talmon/application/CpuUsage.hpp"

#include <gtest/gtest.h>

using talmon::application::cpuUsagePercent;
using talmon::domain::CpuTimes;

TEST(CpuUsage, ComputesBusyShareOfElapsedTime) {
    const CpuTimes before{.user = 100, .system = 50, .idle = 850};
    const CpuTimes after{.user = 130, .system = 60, .idle = 910}; // +40 busy, +60 idle

    const auto usage = cpuUsagePercent(before, after);

    ASSERT_TRUE(usage.has_value());
    EXPECT_DOUBLE_EQ(*usage, 40.0);
}

TEST(CpuUsage, CountsIoWaitAsIdle) {
    const CpuTimes before{};
    const CpuTimes after{.user = 10, .idle = 40, .iowait = 50};

    EXPECT_DOUBLE_EQ(cpuUsagePercent(before, after).value(), 10.0);
}

TEST(CpuUsage, ReturnsNulloptWithoutElapsedTime) {
    const CpuTimes sample{.user = 1, .idle = 1};
    EXPECT_FALSE(cpuUsagePercent(sample, sample).has_value());
}

TEST(CpuUsage, ReturnsNulloptWhenCountersReset) {
    const CpuTimes before{.user = 1000, .idle = 1000};
    const CpuTimes after{.user = 5, .idle = 5};
    EXPECT_FALSE(cpuUsagePercent(before, after).has_value());
}

static_assert(cpuUsagePercent(CpuTimes{}, CpuTimes{.user = 1, .idle = 3}).value() == 25.0,
              "usable in constant expressions");

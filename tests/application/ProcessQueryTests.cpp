#include "talmon/application/ProcessQuery.hpp"

#include <gtest/gtest.h>

#include <ranges>

using namespace talmon::application;
using talmon::domain::ProcessInfo;

namespace {

std::vector<ProcessInfo> sample() {
    return {
        ProcessInfo{.pid = 1, .threads = 1, .cpuTimeSeconds = 5, .residentMemoryBytes = 100, .command = "init"},
        ProcessInfo{.pid = 42, .threads = 30, .cpuTimeSeconds = 900, .residentMemoryBytes = 5000, .command = "kubelet",
                    .args = "--config=/etc/kubernetes/kubelet.yaml"},
        ProcessInfo{.pid = 77, .threads = 12, .cpuTimeSeconds = 300, .residentMemoryBytes = 9000, .command = "etcd"},
        ProcessInfo{.pid = 420, .threads = 4, .cpuTimeSeconds = 1, .residentMemoryBytes = 10, .command = "containerd-shim",
                    .executable = "/bin/containerd-shim-runc-v2"},
    };
}

std::vector<std::int32_t> pids(const std::vector<ProcessInfo>& processes) {
    return processes | std::views::transform(&ProcessInfo::pid) | std::ranges::to<std::vector>();
}

} // namespace

TEST(ProcessQuery, SortsByResidentMemoryDescendingByDefault) {
    EXPECT_EQ(pids(applyQuery(sample(), {})), (std::vector{77, 42, 1, 420}));
}

TEST(ProcessQuery, SortsAscendingWhenRequested) {
    EXPECT_EQ(pids(applyQuery(sample(), {.sortBy = ProcessSortKey::CpuTime, .descending = false})),
              (std::vector{420, 1, 77, 42}));
}

TEST(ProcessQuery, SortsByCommandName) {
    EXPECT_EQ(pids(applyQuery(sample(), {.sortBy = ProcessSortKey::Command, .descending = false})),
              (std::vector{420, 77, 1, 42}));
}

TEST(ProcessQuery, FiltersCaseInsensitivelyOnCommandArgsAndExecutable) {
    EXPECT_EQ(pids(applyQuery(sample(), {.filter = "KUBE"})), (std::vector{42}));
    EXPECT_EQ(pids(applyQuery(sample(), {.filter = "runc"})), (std::vector{420}));
}

TEST(ProcessQuery, NumericFilterMatchesExactPidOrText) {
    // "42" selects pid 42, but not pid 420: pids are compared exactly, never as substrings.
    EXPECT_EQ(pids(applyQuery(sample(), {.filter = "42", .sortBy = ProcessSortKey::Pid, .descending = false})),
              (std::vector{42}));
}

TEST(ProcessQuery, ReturnsEmptyWhenNothingMatches) { EXPECT_TRUE(applyQuery(sample(), {.filter = "nginx"}).empty()); }

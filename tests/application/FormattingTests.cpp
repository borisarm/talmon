#include "talmon/application/Formatting.hpp"

#include <gtest/gtest.h>

#include <limits>

using namespace talmon::application;
using namespace std::chrono_literals;

TEST(FormatBytes, UsesPlainBytesBelowOneKiB) {
    EXPECT_EQ(formatBytes(0), "0 B");
    EXPECT_EQ(formatBytes(1023), "1023 B");
}

TEST(FormatBytes, UsesBinaryUnits) {
    EXPECT_EQ(formatBytes(1024), "1.0 KiB");
    EXPECT_EQ(formatBytes(1536), "1.5 KiB");
    EXPECT_EQ(formatBytes(8ull * 1024 * 1024 * 1024), "8.0 GiB");
}

TEST(FormatBytes, StopsAtLargestUnit) {
    EXPECT_EQ(formatBytes(std::numeric_limits<std::uint64_t>::max()), "16384.0 PiB");
}

TEST(FormatDuration, PicksTwoMostSignificantUnits) {
    EXPECT_EQ(formatDuration(12s), "12s");
    EXPECT_EQ(formatDuration(4min + 10s), "4m 10s");
    EXPECT_EQ(formatDuration(3h + 12min + 5s), "3h 12m");
    EXPECT_EQ(formatDuration(std::chrono::days{2} + 3h + 59min), "2d 3h");
}

TEST(FormatDuration, ClampsNegativeValues) { EXPECT_EQ(formatDuration(-5s), "0s"); }

TEST(FormatPercent, RoundsToOneDecimal) {
    EXPECT_EQ(formatPercent(42.345), "42.3 %");
    EXPECT_EQ(formatPercent(std::numeric_limits<double>::quiet_NaN()), "-");
}

TEST(FormatLoad, ShowsThreeAverages) { EXPECT_EQ(formatLoad(0.4, 1.256, 12), "0.40 / 1.26 / 12.00"); }

TEST(FormatCpuTime, FormatsShortAndLongDurations) {
    EXPECT_EQ(formatCpuTime(0.123), "0.12s");
    EXPECT_EQ(formatCpuTime(125), "2m 05s");
    EXPECT_EQ(formatCpuTime(3 * 3600 + 61), "3h 01m 01s");
    EXPECT_EQ(formatCpuTime(-1), "-");
}

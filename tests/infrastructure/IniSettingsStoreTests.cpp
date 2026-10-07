#include "talmon/infrastructure/IniSettingsStore.hpp"

#include <gtest/gtest.h>

#include <fstream>

using namespace talmon;
using namespace std::chrono_literals;

TEST(ParseSettings, ReadsKnownKeys) {
    const auto settings = infrastructure::parseSettings(
        "# comment\n"
        "talosconfig = C:\\Users\\me\\.talos\\config\r\n"
        "context=home-lab\n"
        "refresh_seconds=15\n"
        "timeout_seconds=7\n"
        "unknown=ignored\n"
        "garbage line\n");

    EXPECT_EQ(settings.talosConfigPath, std::filesystem::path{R"(C:\Users\me\.talos\config)"});
    EXPECT_EQ(settings.context, "home-lab");
    EXPECT_EQ(settings.refreshInterval, 15s);
    EXPECT_EQ(settings.callTimeout, 7s);
}

TEST(ParseSettings, ClampsRefreshIntervalAndIgnoresBadNumbers) {
    EXPECT_EQ(infrastructure::parseSettings("refresh_seconds=0").refreshInterval, 1s);
    EXPECT_EQ(infrastructure::parseSettings("refresh_seconds=999999").refreshInterval, 3600s);
    EXPECT_EQ(infrastructure::parseSettings("refresh_seconds=abc").refreshInterval, 5s);
    EXPECT_EQ(infrastructure::parseSettings("timeout_seconds=-3").callTimeout, 10s);
}

TEST(ParseSettings, RoundTripsThroughSerialization) {
    const application::AppSettings original{
        .talosConfigPath = std::filesystem::path{u8"D:/clústers/ñandú/talosconfig"},
        .context = "prod",
        .refreshInterval = 30s,
        .callTimeout = 4s,
    };
    EXPECT_EQ(infrastructure::parseSettings(infrastructure::serializeSettings(original)), original);
}

TEST(IniSettingsStore, SavesAndLoadsFromDisk) {
    const auto dir = std::filesystem::temp_directory_path() / "talmon-tests" /
                     ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::filesystem::remove_all(dir);
    infrastructure::IniSettingsStore store{dir / "nested" / "settings.ini"};

    EXPECT_EQ(store.load(), application::AppSettings{}); // missing file -> defaults

    application::AppSettings settings;
    settings.context = "lab";
    settings.refreshInterval = 9s;
    ASSERT_TRUE(store.save(settings));
    EXPECT_EQ(store.load(), settings);

    settings.context = "prod";
    ASSERT_TRUE(store.save(settings)); // overwrite existing file
    EXPECT_EQ(store.load().context, "prod");

    std::filesystem::remove_all(dir);
}

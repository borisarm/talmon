#include "talmon/application/Settings.hpp"

#include <gtest/gtest.h>

#include <map>

using namespace talmon::application;

namespace {

EnvironmentLookup fakeEnv(std::map<std::string, std::string, std::less<>> values) {
    return [values = std::move(values)](std::string_view name) -> std::optional<std::string> {
        if (const auto it = values.find(name); it != values.end()) {
            return it->second;
        }
        return std::nullopt;
    };
}

} // namespace

TEST(ResolveTalosConfigPath, PrefersTalosConfigVariable) {
    const auto path = resolveTalosConfigPath(fakeEnv({{"TALOSCONFIG", R"(D:\clusters\talosconfig)"},
                                                      {"USERPROFILE", R"(C:\Users\me)"}}));
    EXPECT_EQ(path, std::filesystem::path{R"(D:\clusters\talosconfig)"});
}

TEST(ResolveTalosConfigPath, FallsBackToUserProfile) {
    const auto path = resolveTalosConfigPath(fakeEnv({{"USERPROFILE", R"(C:\Users\me)"}}));
    EXPECT_EQ(path, std::filesystem::path{R"(C:\Users\me)"} / ".talos" / "config");
}

TEST(ResolveTalosConfigPath, IgnoresEmptyValues) {
    const auto path = resolveTalosConfigPath(fakeEnv({{"TALOSCONFIG", ""}, {"USERPROFILE", ""}, {"HOME", "/home/me"}}));
    EXPECT_EQ(path, std::filesystem::path{"/home/me"} / ".talos" / "config");
}

TEST(ResolveTalosConfigPath, ReturnsNulloptWithoutHome) { EXPECT_FALSE(resolveTalosConfigPath(fakeEnv({}))); }

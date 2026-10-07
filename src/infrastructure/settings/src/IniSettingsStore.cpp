#include "talmon/infrastructure/IniSettingsStore.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <ranges>
#include <sstream>

namespace talmon::infrastructure {

namespace {

constexpr std::string_view kWhitespace = " \t\r\n";

std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(kWhitespace);
    return text.substr(first, last - first + 1);
}

std::optional<std::chrono::seconds> parseSeconds(std::string_view text) {
    long long value{};
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return std::chrono::seconds{value};
}

std::filesystem::path pathFromUtf8(std::string_view text) {
    return std::filesystem::path{std::u8string{text.begin(), text.end()}};
}

std::string pathToUtf8(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return std::string{u8.begin(), u8.end()};
}

} // namespace

application::AppSettings parseSettings(std::string_view text) {
    application::AppSettings settings;
    for (const auto rawLine : text | std::views::split('\n')) {
        const auto line = trim(std::string_view{rawLine.begin(), rawLine.end()});
        if (line.empty() || line.front() == '#' || line.front() == ';') {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        const auto key = trim(line.substr(0, eq));
        const auto value = trim(line.substr(eq + 1));

        if (key == "talosconfig") {
            settings.talosConfigPath = pathFromUtf8(value);
        } else if (key == "context") {
            settings.context = std::string{value};
        } else if (key == "refresh_seconds") {
            if (auto seconds = parseSeconds(value)) {
                settings.refreshInterval = std::clamp(*seconds, application::AppSettings::minRefreshInterval,
                                                      application::AppSettings::maxRefreshInterval);
            }
        } else if (key == "timeout_seconds") {
            if (auto seconds = parseSeconds(value); seconds && seconds->count() > 0) {
                settings.callTimeout = *seconds;
            }
        }
    }
    return settings;
}

std::string serializeSettings(const application::AppSettings& settings) {
    return std::format("# Talmon settings\n"
                       "talosconfig={}\n"
                       "context={}\n"
                       "refresh_seconds={}\n"
                       "timeout_seconds={}\n",
                       pathToUtf8(settings.talosConfigPath), settings.context, settings.refreshInterval.count(),
                       settings.callTimeout.count());
}

IniSettingsStore::IniSettingsStore(std::filesystem::path file) : file_{std::move(file)} {}

application::AppSettings IniSettingsStore::load() {
    std::ifstream in{file_, std::ios::binary};
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parseSettings(buffer.str());
}

domain::Result<void> IniSettingsStore::save(const application::AppSettings& settings) {
    std::error_code ec;
    if (file_.has_parent_path()) {
        std::filesystem::create_directories(file_.parent_path(), ec);
        if (ec) {
            return domain::fail(domain::ErrorCode::Internal, "cannot create settings directory: " + ec.message());
        }
    }

    auto temporary = file_;
    temporary += ".tmp";
    {
        std::ofstream out{temporary, std::ios::binary | std::ios::trunc};
        if (!out) {
            return domain::fail(domain::ErrorCode::Internal, "cannot write " + pathToUtf8(temporary));
        }
        out << serializeSettings(settings);
    }
    std::filesystem::rename(temporary, file_, ec);
    if (ec) {
        return domain::fail(domain::ErrorCode::Internal, "cannot replace settings file: " + ec.message());
    }
    return {};
}

} // namespace talmon::infrastructure

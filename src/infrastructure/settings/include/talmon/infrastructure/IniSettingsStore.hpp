#pragma once

#include "talmon/application/Settings.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace talmon::infrastructure {

/// `key=value` text serialisation of AppSettings (UTF-8). Unknown keys and bad values are ignored.
[[nodiscard]] application::AppSettings parseSettings(std::string_view text);
[[nodiscard]] std::string serializeSettings(const application::AppSettings& settings);

/// Stores the settings in a small UTF-8 file (e.g. %LOCALAPPDATA%\Talmon\settings.ini).
class IniSettingsStore final : public application::ISettingsStore {
public:
    explicit IniSettingsStore(std::filesystem::path file);

    [[nodiscard]] application::AppSettings load() override;
    [[nodiscard]] domain::Result<void> save(const application::AppSettings& settings) override;

private:
    std::filesystem::path file_;
};

} // namespace talmon::infrastructure

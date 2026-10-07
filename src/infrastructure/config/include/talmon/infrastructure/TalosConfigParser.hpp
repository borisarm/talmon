#pragma once

#include "talmon/domain/TalosConfig.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace talmon::infrastructure {

/// Standard (RFC 4648) base64 decoding; whitespace is ignored. nullopt on malformed input.
[[nodiscard]] std::optional<std::string> base64Decode(std::string_view encoded);

/// Parses the YAML content of a talosconfig file (as written by `talosctl config`).
[[nodiscard]] domain::Result<domain::TalosConfig> parseTalosConfig(std::string_view yaml);

/// Reads talosconfig files from disk.
class YamlTalosConfigSource final : public domain::ITalosConfigSource {
public:
    [[nodiscard]] domain::Result<domain::TalosConfig> load(const std::filesystem::path& path) override;
};

} // namespace talmon::infrastructure

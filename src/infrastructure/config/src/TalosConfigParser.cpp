#include "talmon/infrastructure/TalosConfigParser.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <fstream>
#include <sstream>

namespace talmon::infrastructure {

namespace {

constexpr auto kBase64Table = [] {
    std::array<std::int8_t, 256> table{};
    table.fill(-1);
    constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (std::size_t i = 0; i < alphabet.size(); ++i) {
        table[static_cast<unsigned char>(alphabet[i])] = static_cast<std::int8_t>(i);
    }
    return table;
}();

std::vector<std::string> readStringList(const YAML::Node& node) {
    std::vector<std::string> values;
    if (node && node.IsSequence()) {
        for (const auto& item : node) {
            values.push_back(item.as<std::string>());
        }
    }
    return values;
}

domain::Result<std::string> decodePem(const YAML::Node& node, std::string_view field, std::string_view context) {
    if (!node || node.as<std::string>().empty()) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration,
                            std::format("context '{}' is missing '{}'", context, field));
    }
    auto decoded = base64Decode(node.as<std::string>());
    if (!decoded) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration,
                            std::format("context '{}': '{}' is not valid base64", context, field));
    }
    return std::move(*decoded);
}

} // namespace

std::optional<std::string> base64Decode(std::string_view encoded) {
    std::string output;
    output.reserve(encoded.size() * 3 / 4);
    std::uint32_t buffer = 0;
    int bits = 0;
    std::size_t padding = 0;
    for (const char c : encoded) {
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;
        }
        if (c == '=') {
            ++padding;
            continue;
        }
        if (padding > 0) {
            return std::nullopt; // data after padding
        }
        const auto value = kBase64Table[static_cast<unsigned char>(c)];
        if (value < 0) {
            return std::nullopt;
        }
        buffer = (buffer << 6) | static_cast<std::uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output.push_back(static_cast<char>((buffer >> bits) & 0xFFu));
        }
    }
    if (padding > 2 || bits >= 6) {
        return std::nullopt;
    }
    return output;
}

domain::Result<domain::TalosConfig> parseTalosConfig(std::string_view yaml) {
    YAML::Node root;
    try {
        root = YAML::Load(std::string{yaml});
    } catch (const YAML::Exception& e) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration, std::string{"invalid YAML: "} + e.what());
    }

    if (!root || !root.IsMap()) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration, "talosconfig root must be a mapping");
    }

    domain::TalosConfig config;
    try {
        if (const auto current = root["context"]) {
            config.currentContext = current.as<std::string>();
        }

        const auto contexts = root["contexts"];
        if (!contexts || !contexts.IsMap()) {
            return domain::fail(domain::ErrorCode::InvalidConfiguration, "talosconfig has no 'contexts' mapping");
        }

        for (const auto& entry : contexts) {
            domain::TalosContext context;
            context.name = entry.first.as<std::string>();
            const auto& body = entry.second;
            if (!body.IsMap()) {
                return domain::fail(domain::ErrorCode::InvalidConfiguration,
                                    std::format("context '{}' must be a mapping", context.name));
            }
            context.endpoints = readStringList(body["endpoints"]);
            context.nodes = readStringList(body["nodes"]);

            auto ca = decodePem(body["ca"], "ca", context.name);
            auto crt = decodePem(body["crt"], "crt", context.name);
            auto key = decodePem(body["key"], "key", context.name);
            for (const auto* part : {&ca, &crt, &key}) {
                if (!*part) {
                    return std::unexpected{part->error()};
                }
            }
            context.caPem = std::move(*ca);
            context.certificatePem = std::move(*crt);
            context.privateKeyPem = std::move(*key);
            config.contexts.push_back(std::move(context));
        }
    } catch (const YAML::Exception& e) {
        return domain::fail(domain::ErrorCode::InvalidConfiguration, std::string{"unexpected talosconfig layout: "} +
                                                                         e.what());
    }

    std::ranges::sort(config.contexts, {}, &domain::TalosContext::name);
    return config;
}

domain::Result<domain::TalosConfig> YamlTalosConfigSource::load(const std::filesystem::path& path) {
    std::ifstream in{path, std::ios::binary};
    if (!in) {
        const auto u8 = path.u8string();
        return domain::fail(domain::ErrorCode::NotFound,
                            "cannot open talosconfig '" + std::string{u8.begin(), u8.end()} + "'");
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parseTalosConfig(buffer.str());
}

} // namespace talmon::infrastructure

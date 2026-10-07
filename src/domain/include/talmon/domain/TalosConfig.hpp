#pragma once

#include "talmon/domain/Error.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace talmon::domain {

/// One context of a talosconfig file, with credentials already decoded to PEM.
struct TalosContext {
    std::string name;
    std::vector<std::string> endpoints;
    std::vector<std::string> nodes;
    std::string caPem;
    std::string certificatePem;
    std::string privateKeyPem;

    friend bool operator==(const TalosContext&, const TalosContext&) = default;
};

struct TalosConfig {
    std::string currentContext;
    std::vector<TalosContext> contexts;

    [[nodiscard]] const TalosContext* find(std::string_view name) const noexcept {
        const auto it = std::ranges::find(contexts, name, &TalosContext::name);
        return it == contexts.end() ? nullptr : &*it;
    }
};

class ITalosConfigSource {
public:
    virtual ~ITalosConfigSource() = default;
    [[nodiscard]] virtual Result<TalosConfig> load(const std::filesystem::path& path) = 0;
};

} // namespace talmon::domain

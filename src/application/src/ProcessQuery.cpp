#include "talmon/application/ProcessQuery.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <ranges>
#include <optional>
#include <string_view>
#include <tuple>

namespace talmon::application {

namespace {

bool containsCaseInsensitive(std::string_view haystack, std::string_view needle) {
    const auto lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
    return !std::ranges::search(haystack, needle, {}, lower, lower).empty();
}

std::optional<std::int32_t> parsePid(std::string_view text) {
    std::int32_t value{};
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

bool matches(const domain::ProcessInfo& process, std::string_view filter, std::optional<std::int32_t> pid) {
    if (pid && process.pid == *pid) {
        return true;
    }
    return containsCaseInsensitive(process.command, filter) || containsCaseInsensitive(process.executable, filter) ||
           containsCaseInsensitive(process.args, filter);
}

auto projection(ProcessSortKey key) {
    return [key](const domain::ProcessInfo& p) -> std::tuple<double, std::string_view, std::int32_t> {
        switch (key) {
        case ProcessSortKey::CpuTime: return {p.cpuTimeSeconds, {}, p.pid};
        case ProcessSortKey::ResidentMemory: return {static_cast<double>(p.residentMemoryBytes), {}, p.pid};
        case ProcessSortKey::Threads: return {static_cast<double>(p.threads), {}, p.pid};
        case ProcessSortKey::Command: return {0.0, p.command, p.pid};
        case ProcessSortKey::Pid: break;
        }
        return {static_cast<double>(p.pid), {}, p.pid};
    };
}

} // namespace

std::vector<domain::ProcessInfo> applyQuery(std::vector<domain::ProcessInfo> processes, const ProcessQuery& query) {
    if (!query.filter.empty()) {
        const auto pid = parsePid(query.filter);
        std::erase_if(processes, [&](const auto& p) { return !matches(p, query.filter, pid); });
    }

    const auto key = projection(query.sortBy);
    if (query.descending) {
        std::ranges::stable_sort(processes, std::ranges::greater{}, key);
    } else {
        std::ranges::stable_sort(processes, std::ranges::less{}, key);
    }
    return processes;
}

} // namespace talmon::application

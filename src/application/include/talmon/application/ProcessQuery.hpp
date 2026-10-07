#pragma once

#include "talmon/domain/Models.hpp"

#include <string>
#include <vector>

namespace talmon::application {

enum class ProcessSortKey { Pid, CpuTime, ResidentMemory, Threads, Command };

struct ProcessQuery {
    /// Case-insensitive substring matched against command, executable and args; or an exact PID.
    std::string filter;
    ProcessSortKey sortBy{ProcessSortKey::ResidentMemory};
    bool descending{true};
};

[[nodiscard]] std::vector<domain::ProcessInfo> applyQuery(std::vector<domain::ProcessInfo> processes,
                                                          const ProcessQuery& query);

} // namespace talmon::application

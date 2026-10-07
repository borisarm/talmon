#include "pch.h"
#include "ProcessItem.h"
#if __has_include("ProcessItem.g.cpp")
#include "ProcessItem.g.cpp"
#endif

#include "AppServices.h"

#include "talmon/application/Formatting.hpp"

using namespace talmon;

namespace winrt::Talmon::implementation
{
    ProcessItem::ProcessItem(domain::ProcessInfo const& process)
    {
        m_pid = winrt::to_hstring(process.pid);
        m_state = ui::ToHString(process.state);
        m_threads = winrt::to_hstring(process.threads);
        m_cpuTime = ui::ToHString(application::formatCpuTime(process.cpuTimeSeconds));
        m_memory = ui::ToHString(application::formatBytes(process.residentMemoryBytes));
        m_command = ui::ToHString(process.command.empty() ? process.executable : process.command);
        m_args = ui::ToHString(process.args);
    }
}

#pragma once

#include "ProcessItem.g.h"

#include "talmon/domain/Models.hpp"

namespace winrt::Talmon::implementation
{
    struct ProcessItem : ProcessItemT<ProcessItem>
    {
        explicit ProcessItem(talmon::domain::ProcessInfo const& process);

        hstring Pid() const { return m_pid; }
        hstring State() const { return m_state; }
        hstring Threads() const { return m_threads; }
        hstring CpuTime() const { return m_cpuTime; }
        hstring Memory() const { return m_memory; }
        hstring Command() const { return m_command; }
        hstring Args() const { return m_args; }

    private:
        hstring m_pid;
        hstring m_state;
        hstring m_threads;
        hstring m_cpuTime;
        hstring m_memory;
        hstring m_command;
        hstring m_args;
    };
}

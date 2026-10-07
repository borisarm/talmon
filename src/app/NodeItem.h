#pragma once

#include "NodeItem.g.h"

#include "talmon/application/ClusterMonitor.hpp"

namespace winrt::Talmon::implementation
{
    struct NodeItem : NodeItemT<NodeItem>
    {
        explicit NodeItem(talmon::application::NodeOverview const& overview);

        hstring Title() const { return m_title; }
        hstring Address() const { return m_address; }
        hstring Version() const { return m_version; }
        hstring Platform() const { return m_platform; }
        hstring Uptime() const { return m_uptime; }
        hstring CpuText() const { return m_cpuText; }
        double CpuPercent() const { return m_cpuPercent; }
        hstring MemoryText() const { return m_memoryText; }
        double MemoryPercent() const { return m_memoryPercent; }
        hstring LoadText() const { return m_loadText; }
        hstring ServicesText() const { return m_servicesText; }
        hstring StatusText() const { return m_statusText; }
        Microsoft::UI::Xaml::Media::Brush StatusBrush() const { return m_statusBrush; }
        hstring ErrorText() const { return m_errorText; }
        Microsoft::UI::Xaml::Visibility ErrorVisibility() const;

    private:
        hstring m_title;
        hstring m_address;
        hstring m_version;
        hstring m_platform;
        hstring m_uptime;
        hstring m_cpuText;
        double m_cpuPercent{};
        hstring m_memoryText;
        double m_memoryPercent{};
        hstring m_loadText;
        hstring m_servicesText;
        hstring m_statusText;
        Microsoft::UI::Xaml::Media::Brush m_statusBrush{ nullptr };
        hstring m_errorText;
    };
}

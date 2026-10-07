#include "pch.h"
#include "NodeItem.h"
#if __has_include("NodeItem.g.cpp")
#include "NodeItem.g.cpp"
#endif

#include "AppServices.h"
#include "StatusBrushes.h"

#include "talmon/application/Formatting.hpp"

#include <format>

using namespace talmon;

namespace winrt::Talmon::implementation
{
    NodeItem::NodeItem(application::NodeOverview const& overview)
    {
        using ui::ToHString;
        constexpr auto dash = L"—";

        m_title = ToHString(overview.hostname.empty() ? overview.node : overview.hostname);
        m_address = ToHString(overview.node.empty() ? std::string{ "(endpoint)" } : overview.node);

        if (overview.version)
        {
            m_version = ToHString(overview.version->tag);
            m_platform = ToHString(overview.version->platformName.empty()
                ? overview.version->arch
                : std::format("{} · {}", overview.version->platformName, overview.version->arch));
        }
        else
        {
            m_version = dash;
            m_platform = dash;
        }

        m_uptime = overview.uptime ? ToHString(application::formatDuration(*overview.uptime)) : hstring{ dash };

        if (overview.cpuUsagePercent)
        {
            m_cpuPercent = *overview.cpuUsagePercent;
            m_cpuText = ToHString(std::format("{} · {} vCPU", application::formatPercent(m_cpuPercent), overview.cpuCount));
        }
        else
        {
            m_cpuText = overview.cpuCount > 0 ? ToHString(std::format("midiendo… · {} vCPU", overview.cpuCount)) : hstring{ dash };
        }

        if (overview.memory)
        {
            m_memoryPercent = overview.memory->usedRatio() * 100.0;
            m_memoryText = ToHString(std::format("{} / {}",
                application::formatBytes(overview.memory->usedBytes()),
                application::formatBytes(overview.memory->totalBytes)));
        }
        else
        {
            m_memoryText = dash;
        }

        m_loadText = overview.load
            ? ToHString(application::formatLoad(overview.load->load1, overview.load->load5, overview.load->load15))
            : hstring{ dash };

        ui::Tone tone = ui::Tone::Critical;
        if (overview.services)
        {
            const auto& s = *overview.services;
            m_servicesText = ToHString(std::format("{}/{} activos", s.up(), s.total - s.inactive));
            if (s.failed > 0)
            {
                m_servicesText = m_servicesText + ToHString(std::format(" · {} con fallo", s.failed));
            }
        }
        else
        {
            m_servicesText = dash;
        }

        if (!overview.reachable)
        {
            m_statusText = L"Inaccesible";
            tone = ui::Tone::Critical;
        }
        else if (!overview.errors.empty() || (overview.services && overview.services->failed > 0))
        {
            m_statusText = L"Degradado";
            tone = ui::Tone::Caution;
        }
        else
        {
            m_statusText = L"Operativo";
            tone = ui::Tone::Success;
        }
        m_statusBrush = ui::BrushFor(tone);

        std::string errors;
        for (auto const& error : overview.errors)
        {
            if (!errors.empty())
            {
                errors += '\n';
            }
            errors += error;
        }
        m_errorText = ToHString(errors);
    }

    Microsoft::UI::Xaml::Visibility NodeItem::ErrorVisibility() const
    {
        return m_errorText.empty() ? Microsoft::UI::Xaml::Visibility::Collapsed : Microsoft::UI::Xaml::Visibility::Visible;
    }
}

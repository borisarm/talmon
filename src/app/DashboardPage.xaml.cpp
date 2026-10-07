#include "pch.h"
#include "DashboardPage.xaml.h"
#if __has_include("DashboardPage.g.cpp")
#include "DashboardPage.g.cpp"
#endif

#include "NodeItem.h"
#include "UiAsync.h"

#include <algorithm>
#include <format>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using talmon::application::NodeOverview;

namespace winrt::Talmon::implementation
{
    void DashboardPage::Attach(std::shared_ptr<talmon::ui::AppServices> services)
    {
        m_services = std::move(services);
        NodesView().ItemsSource(m_items);
    }

    void DashboardPage::Refresh()
    {
        auto session = m_services->Session();
        auto client = session->client();
        if (m_busy || !client)
        {
            return;
        }

        m_busy = true;
        m_services->ReportBusy(true);
        talmon::ui::RunInBackground(get_weak(),
            [client, nodes = session->nodes(), monitor = m_services->Monitor()]
            {
                return monitor->collect(*client, nodes);
            },
            [generation = m_services->Generation()](auto const& self, std::vector<NodeOverview> overviews)
            {
                self->Apply(generation, std::move(overviews));
            });
    }

    void DashboardPage::Apply(std::uint64_t generation, std::vector<NodeOverview> overviews)
    {
        m_busy = false;
        m_services->ReportBusy(false);
        if (generation != m_services->Generation())
        {
            return; // stale result from a previous connection
        }

        // Replace in place when the node set is unchanged to avoid re-creating every card.
        if (m_items.Size() == overviews.size())
        {
            for (uint32_t i = 0; i < m_items.Size(); ++i)
            {
                m_items.SetAt(i, make<NodeItem>(overviews[i]));
            }
        }
        else
        {
            m_items.Clear();
            for (auto const& overview : overviews)
            {
                m_items.Append(make<NodeItem>(overview));
            }
        }
        EmptyText().Visibility(overviews.empty() ? Visibility::Visible : Visibility::Collapsed);

        auto const unreachable = std::ranges::count_if(overviews, [](auto const& o) { return !o.reachable; });
        if (unreachable > 0)
        {
            m_services->ReportStatus(talmon::ui::StatusKind::Warning,
                std::format(L"{} de {} nodo(s) inaccesibles", unreachable, overviews.size()),
                L"Revisa la conectividad con el puerto 50000 (apid) y las credenciales del contexto.");
        }
        else
        {
            m_services->ReportStatus(talmon::ui::StatusKind::Success, {});
        }
    }
}

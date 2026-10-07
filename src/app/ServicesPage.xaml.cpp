#include "pch.h"
#include "ServicesPage.xaml.h"
#if __has_include("ServicesPage.g.cpp")
#include "ServicesPage.g.cpp"
#endif

#include "ServiceItem.h"
#include "UiAsync.h"

#include "talmon/application/ServiceHealth.hpp"

#include <format>

using namespace winrt;
using namespace talmon;

namespace winrt::Talmon::implementation
{
    void ServicesPage::Attach(std::shared_ptr<ui::AppServices> services)
    {
        m_services = std::move(services);
        ServicesList().ItemsSource(m_items);
    }

    void ServicesPage::Refresh()
    {
        auto client = m_services->Session()->client();
        if (m_busy || !client)
        {
            return;
        }

        m_busy = true;
        m_services->ReportBusy(true);
        auto node = m_services->SelectedNode();
        ui::RunInBackground(get_weak(),
            [client, node] { return client->services(node); },
            [generation = m_services->Generation(), node](auto const& self, auto result)
            {
                self->Apply(generation, node, std::move(result));
            });
    }

    void ServicesPage::Apply(std::uint64_t generation, std::string const& node,
        domain::Result<std::vector<domain::ServiceStatus>> result)
    {
        m_busy = false;
        m_services->ReportBusy(false);
        if (generation != m_services->Generation() || node != m_services->SelectedNode())
        {
            return;
        }
        if (!result)
        {
            m_services->ReportError(L"No se pudo obtener la lista de servicios", result.error());
            return;
        }

        m_items.Clear();
        for (auto const& service : *result)
        {
            m_items.Append(make<ServiceItem>(service));
        }

        auto const summary = application::summarize(*result);
        SummaryText().Text(ui::ToHString(std::format(
            "{} servicios en {} · {} saludables · {} en ejecución · {} pendientes · {} con fallo",
            summary.total, node, summary.healthy, summary.running, summary.pending, summary.failed)));
        m_services->ReportStatus(ui::StatusKind::Success, {});
    }
}

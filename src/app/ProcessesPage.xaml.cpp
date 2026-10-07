#include "pch.h"
#include "ProcessesPage.xaml.h"
#if __has_include("ProcessesPage.g.cpp")
#include "ProcessesPage.g.cpp"
#endif

#include "ProcessItem.h"
#include "UiAsync.h"

#include <array>
#include <format>

using namespace winrt;
using namespace talmon;

namespace winrt::Talmon::implementation
{
    namespace
    {
        // Same order as the SortBox items in ProcessesPage.xaml.
        constexpr std::array kSortKeys{
            application::ProcessSortKey::ResidentMemory,
            application::ProcessSortKey::CpuTime,
            application::ProcessSortKey::Threads,
            application::ProcessSortKey::Pid,
            application::ProcessSortKey::Command,
        };
    }

    void ProcessesPage::Attach(std::shared_ptr<ui::AppServices> services)
    {
        m_services = std::move(services);
        ProcessList().ItemsSource(m_items);
    }

    void ProcessesPage::Refresh()
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
            [client, node] { return client->processes(node); },
            [generation = m_services->Generation(), node](auto const& self, auto result)
            {
                self->Apply(generation, node, std::move(result));
            });
    }

    void ProcessesPage::Apply(std::uint64_t generation, std::string const& node,
        domain::Result<std::vector<domain::ProcessInfo>> result)
    {
        m_busy = false;
        m_services->ReportBusy(false);
        if (generation != m_services->Generation() || node != m_services->SelectedNode())
        {
            return;
        }
        if (!result)
        {
            m_services->ReportError(L"No se pudo obtener la lista de procesos", result.error());
            return;
        }
        m_processes = std::move(*result);
        Render();
        m_services->ReportStatus(ui::StatusKind::Success, {});
    }

    application::ProcessQuery ProcessesPage::CurrentQuery()
    {
        auto const index = SortBox().SelectedIndex();
        auto const valid = index >= 0 && static_cast<std::size_t>(index) < kSortKeys.size();
        return application::ProcessQuery{
            .filter = ui::ToUtf8(FilterBox().Text()),
            .sortBy = valid ? kSortKeys[static_cast<std::size_t>(index)] : kSortKeys[0],
            .descending = DescendingToggle().IsChecked().Value(),
        };
    }

    void ProcessesPage::Render()
    {
        auto const visible = application::applyQuery(m_processes, CurrentQuery());

        // Rebuild with a single ReplaceAll to keep ListView updates cheap.
        std::vector<Windows::Foundation::IInspectable> items;
        items.reserve(visible.size());
        for (auto const& process : visible)
        {
            items.push_back(make<ProcessItem>(process));
        }
        m_items.ReplaceAll(items);
        CountText().Text(ui::ToHString(std::format("{} de {} procesos", visible.size(), m_processes.size())));
    }

    void ProcessesPage::OnFilterChanged(IInspectable const&, Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&)
    {
        Render();
    }

    void ProcessesPage::OnSortChanged(IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&)
    {
        if (m_services)
        {
            Render();
        }
    }

    void ProcessesPage::OnSortDirectionClicked(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        Render();
    }
}

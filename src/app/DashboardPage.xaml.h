#pragma once

#include "DashboardPage.g.h"

#include "IMonitorView.h"

namespace winrt::Talmon::implementation
{
    /// Cards with health and resource usage of every node of the active context.
    struct DashboardPage : DashboardPageT<DashboardPage>, talmon::ui::IMonitorView
    {
        DashboardPage() = default;

        void Attach(std::shared_ptr<talmon::ui::AppServices> services) override;
        void Refresh() override;

    private:
        void Apply(std::uint64_t generation, std::vector<talmon::application::NodeOverview> overviews);

        std::shared_ptr<talmon::ui::AppServices> m_services;
        Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> m_items{
            single_threaded_observable_vector<Windows::Foundation::IInspectable>() };
        bool m_busy{ false };
    };
}

namespace winrt::Talmon::factory_implementation
{
    struct DashboardPage : DashboardPageT<DashboardPage, implementation::DashboardPage>
    {
    };
}

#pragma once

#include "ProcessesPage.g.h"

#include "IMonitorView.h"

#include "talmon/application/ProcessQuery.hpp"

namespace winrt::Talmon::implementation
{
    /// Process table of the selected node with client side filtering and sorting.
    struct ProcessesPage : ProcessesPageT<ProcessesPage>, talmon::ui::IMonitorView
    {
        ProcessesPage() = default;

        void Attach(std::shared_ptr<talmon::ui::AppServices> services) override;
        void Refresh() override;

        void OnFilterChanged(Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);
        void OnSortChanged(Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void OnSortDirectionClicked(Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void Apply(std::uint64_t generation, std::string const& node,
            talmon::domain::Result<std::vector<talmon::domain::ProcessInfo>> result);
        void Render();
        talmon::application::ProcessQuery CurrentQuery();

        std::shared_ptr<talmon::ui::AppServices> m_services;
        std::vector<talmon::domain::ProcessInfo> m_processes;
        Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> m_items{
            single_threaded_observable_vector<Windows::Foundation::IInspectable>() };
        bool m_busy{ false };
    };
}

namespace winrt::Talmon::factory_implementation
{
    struct ProcessesPage : ProcessesPageT<ProcessesPage, implementation::ProcessesPage>
    {
    };
}

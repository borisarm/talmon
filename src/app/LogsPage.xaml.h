#pragma once

#include "LogsPage.g.h"

#include "IMonitorView.h"

#include "talmon/application/LogStreamSession.hpp"

namespace winrt::Talmon::implementation
{
    /// Streams the log of a Talos system service (MachineService.Logs) of the selected node.
    struct LogsPage : LogsPageT<LogsPage>, talmon::ui::IMonitorView
    {
        LogsPage() = default;
        ~LogsPage() override;

        void Attach(std::shared_ptr<talmon::ui::AppServices> services) override;
        void Refresh() override;
        bool AutoRefresh() const override { return false; }
        void OnShown() override;

        void OnStartClicked(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnStopClicked(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnClearClicked(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        static constexpr uint32_t kMaxLines = 10'000;

        void StopStream();
        void AppendLines(std::vector<std::string> const& lines);
        void OnStreamFinished(talmon::domain::Result<void> const& outcome);
        void SetStreaming(bool streaming);

        std::shared_ptr<talmon::ui::AppServices> m_services;
        std::unique_ptr<talmon::application::LogStreamSession> m_stream;
        std::uint64_t m_streamId{ 0 };
        std::string m_servicesNode;
        Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> m_lines{
            single_threaded_observable_vector<Windows::Foundation::IInspectable>() };
        bool m_busy{ false };
    };
}

namespace winrt::Talmon::factory_implementation
{
    struct LogsPage : LogsPageT<LogsPage, implementation::LogsPage>
    {
    };
}

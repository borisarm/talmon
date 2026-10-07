#pragma once

#include "MainWindow.g.h"

#include "AppServices.h"
#include "IMonitorView.h"

#include <map>

namespace winrt::Talmon::implementation
{
    /// Shell: navigation, node selection, refresh cadence and status reporting.
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void InitializeComponent();

        void OnNavigationSelectionChanged(Microsoft::UI::Xaml::Controls::NavigationView const& sender,
            Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);
        void OnNodeSelectionChanged(Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void OnRefreshClicked(Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        struct ViewEntry
        {
            Microsoft::UI::Xaml::UIElement element{ nullptr };
            talmon::ui::IMonitorView* contract{};
            hstring title;
        };

        template <typename TPage>
        void RegisterView(hstring const& tag, hstring const& title);
        void ShowView(hstring const& tag);
        talmon::ui::IMonitorView* CurrentView() const;

        void OnConnectionChanged();
        void PopulateNodes();
        void ShowStatus(talmon::ui::StatusKind kind, std::wstring_view title, std::wstring_view message);
        void RestartTimer();
        void TryAutoConnect();

        std::shared_ptr<talmon::ui::AppServices> m_services;
        std::map<hstring, ViewEntry> m_views;
        hstring m_currentTag;
        Microsoft::UI::Dispatching::DispatcherQueueTimer m_timer{ nullptr };
        bool m_populatingNodes{ false };
    };
}

namespace winrt::Talmon::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}

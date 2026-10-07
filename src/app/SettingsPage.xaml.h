#pragma once

#include "SettingsPage.g.h"

#include "IMonitorView.h"

namespace winrt::Talmon::implementation
{
    /// talosconfig location, context selection and refresh preferences.
    struct SettingsPage : SettingsPageT<SettingsPage>, talmon::ui::IMonitorView
    {
        SettingsPage() = default;

        void Attach(std::shared_ptr<talmon::ui::AppServices> services) override;
        void Refresh() override;
        bool AutoRefresh() const override { return false; }

        fire_and_forget OnBrowseClicked(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnInspectClicked(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnConnectClicked(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void LoadContexts(std::string const& preferred);
        std::filesystem::path ConfigPath();

        std::shared_ptr<talmon::ui::AppServices> m_services;
    };
}

namespace winrt::Talmon::factory_implementation
{
    struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage>
    {
    };
}

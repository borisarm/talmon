#include "pch.h"
#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

#include <cmath>
#include <format>

using namespace winrt;
using namespace talmon;
using namespace Microsoft::UI::Xaml;

namespace winrt::Talmon::implementation
{
    void SettingsPage::Attach(std::shared_ptr<ui::AppServices> services)
    {
        m_services = std::move(services);
    }

    void SettingsPage::Refresh()
    {
        auto const& settings = m_services->Settings();
        ConfigPathBox().Text(hstring{ m_services->EffectiveTalosConfigPath().wstring() });
        RefreshBox().Value(static_cast<double>(settings.refreshInterval.count()));
        TimeoutBox().Value(static_cast<double>(settings.callTimeout.count()));

        auto const& session = *m_services->Session();
        ConnectionText().Text(session.connected()
            ? ui::ToHString(std::format("Conectado al contexto «{}»", session.activeContext()))
            : hstring{ L"Sin conexión" });

        if (ContextBox().Items().Size() == 0)
        {
            LoadContexts(session.connected() ? session.activeContext() : settings.context);
        }
    }

    std::filesystem::path SettingsPage::ConfigPath()
    {
        return std::filesystem::path{ std::wstring{ ConfigPathBox().Text() } };
    }

    void SettingsPage::LoadContexts(std::string const& preferred)
    {
        ContextBox().Items().Clear();
        auto const path = ConfigPath();
        if (path.empty())
        {
            return;
        }

        auto names = m_services->Session()->inspectConfig(path);
        if (!names)
        {
            m_services->ReportError(L"No se pudo leer el talosconfig", names.error());
            return;
        }

        int32_t selected = 0;
        for (auto const& name : *names)
        {
            if (name == preferred)
            {
                selected = static_cast<int32_t>(ContextBox().Items().Size());
            }
            ContextBox().Items().Append(box_value(ui::ToHString(name)));
        }
        if (ContextBox().Items().Size() > 0)
        {
            ContextBox().SelectedIndex(selected);
        }
    }

    fire_and_forget SettingsPage::OnBrowseClicked(IInspectable const&, RoutedEventArgs const&)
    {
        auto strong = get_strong();
        Microsoft::Windows::Storage::Pickers::FileOpenPicker picker{ m_services->MainWindowId };
        picker.FileTypeFilter().Append(L"*");
        picker.CommitButtonText(L"Usar este talosconfig");

        auto const result = co_await picker.PickSingleFileAsync();
        if (result)
        {
            ConfigPathBox().Text(result.Path());
            LoadContexts({});
        }
    }

    void SettingsPage::OnInspectClicked(IInspectable const&, RoutedEventArgs const&)
    {
        LoadContexts(m_services->Session()->activeContext());
    }

    void SettingsPage::OnConnectClicked(IInspectable const&, RoutedEventArgs const&)
    {
        auto settings = m_services->Settings();
        auto const seconds = [](double value, std::chrono::seconds fallback)
        {
            return std::isnan(value) ? fallback : std::chrono::seconds{ static_cast<long long>(value) };
        };
        settings.refreshInterval = std::clamp(seconds(RefreshBox().Value(), settings.refreshInterval),
            application::AppSettings::minRefreshInterval, application::AppSettings::maxRefreshInterval);
        settings.callTimeout = std::max(seconds(TimeoutBox().Value(), settings.callTimeout), std::chrono::seconds{ 1 });
        if (auto saved = m_services->UpdateSettings(settings); !saved)
        {
            m_services->ReportError(L"No se pudieron guardar las preferencias", saved.error());
        }

        auto const context = ContextBox().SelectedItem()
            ? ui::ToUtf8(unbox_value<hstring>(ContextBox().SelectedItem()))
            : std::string{};

        // Connecting only builds the channel (gRPC connects lazily), so it is safe on the UI thread.
        if (auto connected = m_services->Connect(ConfigPath(), context); !connected)
        {
            m_services->ReportError(L"No se pudo conectar", connected.error());
            ConnectionText().Text(L"Sin conexión");
            return;
        }
        m_services->ReportStatus(ui::StatusKind::Success,
            ui::ToHString(std::format("Conectado a «{}»", m_services->Session()->activeContext())).c_str());
    }
}

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "DashboardPage.xaml.h"
#include "LogsPage.xaml.h"
#include "ProcessesPage.xaml.h"
#include "ServicesPage.xaml.h"
#include "SettingsPage.xaml.h"

#include <format>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using talmon::ui::StatusKind;

namespace winrt::Talmon::implementation
{
    namespace
    {
        constexpr wchar_t kSettingsTag[] = L"settings";
    }

    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();

        // AppWindow sizes are in physical pixels: scale the design size by the monitor DPI.
        auto const hwnd = Microsoft::UI::GetWindowFromWindowId(AppWindow().Id());
        auto const scale = static_cast<double>(::GetDpiForWindow(hwnd)) / USER_DEFAULT_SCREEN_DPI;
        AppWindow().Resize({ static_cast<int32_t>(1280 * scale), static_cast<int32_t>(820 * scale) });

        m_services = talmon::ui::AppServices::Create();
        m_services->MainWindowId = AppWindow().Id();
        m_services->SetStatusSink([weak = get_weak()](StatusKind kind, std::wstring_view title, std::wstring_view message)
        {
            if (auto self = weak.get())
            {
                self->ShowStatus(kind, title, message);
            }
        });
        m_services->SetBusySink([weak = get_weak()](bool busy)
        {
            if (auto self = weak.get())
            {
                self->BusyRing().IsActive(busy);
            }
        });
        m_services->OnConnectionChanged([weak = get_weak()]
        {
            if (auto self = weak.get())
            {
                self->OnConnectionChanged();
            }
        });
        m_services->OnSelectedNodeChanged([weak = get_weak()]
        {
            if (auto self = weak.get())
            {
                if (auto* view = self->CurrentView())
                {
                    view->Refresh();
                }
            }
        });

        RegisterView<DashboardPage>(L"dashboard", L"Panel del clúster");
        RegisterView<ServicesPage>(L"services", L"Servicios");
        RegisterView<ProcessesPage>(L"processes", L"Procesos");
        RegisterView<LogsPage>(L"logs", L"Logs de servicios");
        RegisterView<SettingsPage>(kSettingsTag, L"Configuración");

        m_timer = Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread().CreateTimer();
        m_timer.Tick([weak = get_weak()](auto&&, auto&&)
        {
            if (auto self = weak.get())
            {
                if (auto* view = self->CurrentView(); view && view->AutoRefresh() && self->m_services->Session()->connected())
                {
                    view->Refresh();
                }
            }
        });

        Navigation().SelectedItem(Navigation().MenuItems().GetAt(0));
        TryAutoConnect();
    }

    template <typename TPage>
    void MainWindow::RegisterView(hstring const& tag, hstring const& title)
    {
        auto page = make_self<TPage>();
        page->Attach(m_services);
        talmon::ui::IMonitorView* contract = page.get();
        m_views.insert_or_assign(tag, ViewEntry{ page.template as<UIElement>(), contract, title });
    }

    void MainWindow::ShowView(hstring const& tag)
    {
        auto const it = m_views.find(tag);
        if (it == m_views.end() || tag == m_currentTag)
        {
            return;
        }
        if (auto* previous = CurrentView())
        {
            previous->OnHidden();
        }
        m_currentTag = tag;
        PageHost().Content(it->second.element);
        PageTitle().Text(it->second.title);

        bool const isSettings = tag == kSettingsTag;
        NodeSelector().Visibility(isSettings ? Visibility::Collapsed : Visibility::Visible);
        RefreshButton().Visibility(isSettings ? Visibility::Collapsed : Visibility::Visible);

        if (isSettings || m_services->Session()->connected())
        {
            it->second.contract->OnShown();
        }
    }

    talmon::ui::IMonitorView* MainWindow::CurrentView() const
    {
        auto const it = m_views.find(m_currentTag);
        return it == m_views.end() ? nullptr : it->second.contract;
    }

    void MainWindow::OnNavigationSelectionChanged(NavigationView const&, NavigationViewSelectionChangedEventArgs const& args)
    {
        if (args.IsSettingsSelected())
        {
            ShowView(kSettingsTag);
        }
        else if (auto item = args.SelectedItemContainer())
        {
            ShowView(unbox_value_or<hstring>(item.Tag(), L"dashboard"));
        }
    }

    void MainWindow::OnNodeSelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (m_populatingNodes)
        {
            return;
        }
        if (auto selected = NodeSelector().SelectedItem())
        {
            m_services->SelectNode(talmon::ui::ToUtf8(unbox_value<hstring>(selected)));
        }
    }

    void MainWindow::OnRefreshClicked(IInspectable const&, RoutedEventArgs const&)
    {
        if (auto* view = CurrentView(); view && m_services->Session()->connected())
        {
            view->Refresh();
        }
    }

    void MainWindow::OnConnectionChanged()
    {
        auto const& session = *m_services->Session();
        ContextText().Text(talmon::ui::ToHString(std::format("Contexto: {} · {} nodo(s)",
            session.activeContext(), session.nodes().size())));
        LastUpdateText().Text(L"");
        StatusBar().IsOpen(false);
        PopulateNodes();
        RestartTimer();

        if (m_currentTag == kSettingsTag)
        {
            Navigation().SelectedItem(Navigation().MenuItems().GetAt(0));
        }
        else if (auto* view = CurrentView())
        {
            view->Refresh();
        }
    }

    void MainWindow::PopulateNodes()
    {
        m_populatingNodes = true;
        NodeSelector().Items().Clear();
        for (auto const& node : m_services->Session()->nodes())
        {
            NodeSelector().Items().Append(box_value(talmon::ui::ToHString(node)));
        }
        if (NodeSelector().Items().Size() > 0)
        {
            NodeSelector().SelectedIndex(0);
        }
        m_populatingNodes = false;
    }

    void MainWindow::ShowStatus(StatusKind kind, std::wstring_view title, std::wstring_view message)
    {
        if (kind == StatusKind::Success)
        {
            auto const now = std::chrono::zoned_time{ std::chrono::current_zone(),
                std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()) };
            LastUpdateText().Text(hstring{ std::format(L"Actualizado {:%H:%M:%S}", now) });
            if (title.empty())
            {
                // A plain successful refresh clears any error left by a previous one.
                StatusBar().IsOpen(false);
                return;
            }
        }

        InfoBarSeverity severity = InfoBarSeverity::Informational;
        switch (kind)
        {
        case StatusKind::Success: severity = InfoBarSeverity::Success; break;
        case StatusKind::Warning: severity = InfoBarSeverity::Warning; break;
        case StatusKind::Error: severity = InfoBarSeverity::Error; break;
        case StatusKind::Informational: break;
        }
        StatusBar().Severity(severity);
        StatusBar().Title(hstring{ title });
        StatusBar().Message(hstring{ message });
        StatusBar().IsOpen(true);
    }

    void MainWindow::RestartTimer()
    {
        m_timer.Stop();
        m_timer.Interval(m_services->Settings().refreshInterval);
        m_timer.Start();
    }

    void MainWindow::TryAutoConnect()
    {
        auto const path = m_services->EffectiveTalosConfigPath();
        std::error_code ec;
        if (path.empty() || !std::filesystem::exists(path, ec))
        {
            ShowStatus(StatusKind::Informational, L"Configura el acceso al clúster",
                L"No se encontró un talosconfig. Indica su ruta en Configuración.");
            Navigation().SelectedItem(Navigation().SettingsItem());
            return;
        }

        if (auto connected = m_services->Connect(path, m_services->Settings().context); !connected)
        {
            m_services->ReportError(L"No se pudo conectar", connected.error());
            Navigation().SelectedItem(Navigation().SettingsItem());
        }
    }
}

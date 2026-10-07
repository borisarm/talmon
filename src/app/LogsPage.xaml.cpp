#include "pch.h"
#include "LogsPage.xaml.h"
#if __has_include("LogsPage.g.cpp")
#include "LogsPage.g.cpp"
#endif

#include "UiAsync.h"

#include <charconv>
#include <format>

using namespace winrt;
using namespace talmon;
using namespace Microsoft::UI::Xaml;

namespace winrt::Talmon::implementation
{
    LogsPage::~LogsPage()
    {
        m_stream.reset(); // cancel the gRPC stream and join its worker before members go away
    }

    void LogsPage::Attach(std::shared_ptr<ui::AppServices> services)
    {
        m_services = std::move(services);
        LogList().ItemsSource(m_lines);

        auto const reset = [weak = get_weak()]
        {
            if (auto self = weak.get())
            {
                self->StopStream();
                self->m_servicesNode.clear();
                self->ServiceBox().Items().Clear();
            }
        };
        m_services->OnConnectionChanged(reset);
        m_services->OnSelectedNodeChanged(reset);
    }

    void LogsPage::OnShown()
    {
        Refresh();
    }

    void LogsPage::Refresh()
    {
        // Loads the list of services of the selected node into the selector.
        auto client = m_services->Session()->client();
        auto node = m_services->SelectedNode();
        if (m_busy || !client || (node == m_servicesNode && ServiceBox().Items().Size() > 0))
        {
            return;
        }

        m_busy = true;
        ui::RunInBackground(get_weak(),
            [client, node] { return client->services(node); },
            [node](auto const& self, domain::Result<std::vector<domain::ServiceStatus>> result)
            {
                self->m_busy = false;
                if (node != self->m_services->SelectedNode())
                {
                    return;
                }
                if (!result)
                {
                    self->m_services->ReportError(L"No se pudo obtener la lista de servicios", result.error());
                    return;
                }
                self->m_servicesNode = node;
                auto items = self->ServiceBox().Items();
                items.Clear();
                for (auto const& service : *result)
                {
                    items.Append(box_value(ui::ToHString(service.id)));
                }
                if (items.Size() > 0)
                {
                    self->ServiceBox().SelectedIndex(0);
                }
            });
    }

    void LogsPage::OnStartClicked(IInspectable const&, RoutedEventArgs const&)
    {
        auto client = m_services->Session()->client();
        auto selected = ServiceBox().SelectedItem();
        if (!client || !selected)
        {
            return;
        }

        StopStream();

        int tailLines = 500;
        auto const tailText = ui::ToUtf8(unbox_value_or<hstring>(TailBox().SelectedItem(), L"500"));
        std::from_chars(tailText.data(), tailText.data() + tailText.size(), tailLines);

        domain::LogRequest request{
            .serviceId = ui::ToUtf8(unbox_value<hstring>(selected)),
            .follow = FollowSwitch().IsOn(),
            .tailLines = tailLines,
        };
        auto const node = m_services->SelectedNode();
        auto const streamId = ++m_streamId;
        auto dispatcher = DispatcherQueue();

        StreamStatusText().Text(ui::ToHString(std::format("Recibiendo logs de «{}» en {}…", request.serviceId, node)));
        SetStreaming(true);

        m_stream = std::make_unique<application::LogStreamSession>(
            client, node, request,
            [weak = get_weak(), dispatcher, streamId](std::vector<std::string> lines)
            {
                dispatcher.TryEnqueue([weak, streamId, lines = std::move(lines)]
                {
                    if (auto self = weak.get(); self && self->m_streamId == streamId)
                    {
                        self->AppendLines(lines);
                    }
                });
            },
            [weak = get_weak(), dispatcher, streamId](domain::Result<void> const& outcome)
            {
                dispatcher.TryEnqueue([weak, streamId, outcome]
                {
                    if (auto self = weak.get(); self && self->m_streamId == streamId)
                    {
                        self->OnStreamFinished(outcome);
                    }
                });
            });
    }

    void LogsPage::OnStopClicked(IInspectable const&, RoutedEventArgs const&)
    {
        StopStream();
        StreamStatusText().Text(L"Detenido.");
    }

    void LogsPage::OnClearClicked(IInspectable const&, RoutedEventArgs const&)
    {
        m_lines.Clear();
    }

    void LogsPage::StopStream()
    {
        ++m_streamId; // late callbacks of the old stream are ignored
        m_stream.reset(); // cancels the gRPC call and joins the worker
        SetStreaming(false);
    }

    void LogsPage::AppendLines(std::vector<std::string> const& lines)
    {
        for (auto const& line : lines)
        {
            m_lines.Append(box_value(ui::ToHString(line)));
        }
        while (m_lines.Size() > kMaxLines)
        {
            m_lines.RemoveAt(0);
        }
        if (AutoScrollSwitch().IsOn() && m_lines.Size() > 0)
        {
            LogList().ScrollIntoView(m_lines.GetAt(m_lines.Size() - 1));
        }
    }

    void LogsPage::OnStreamFinished(domain::Result<void> const& outcome)
    {
        SetStreaming(false);
        if (outcome)
        {
            StreamStatusText().Text(ui::ToHString(std::format("Fin del log · {} líneas", m_lines.Size())));
        }
        else if (outcome.error().code != domain::ErrorCode::Cancelled)
        {
            StreamStatusText().Text(L"El stream terminó con error.");
            m_services->ReportError(L"Error leyendo logs", outcome.error());
        }
    }

    void LogsPage::SetStreaming(bool streaming)
    {
        StartButton().IsEnabled(!streaming);
        StopButton().IsEnabled(streaming);
    }
}

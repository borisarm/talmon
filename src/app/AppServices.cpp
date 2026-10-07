#include "pch.h"
#include "AppServices.h"

#include "talmon/backend/Backend.hpp"
#include "talmon/infrastructure/IniSettingsStore.hpp"

#include <cstdlib>

namespace talmon::ui {

namespace {

std::optional<std::string> ReadEnvironment(std::string_view name) {
    const std::wstring wideName = winrt::to_hstring(name).c_str();
    const DWORD size = ::GetEnvironmentVariableW(wideName.c_str(), nullptr, 0);
    if (size == 0) {
        return std::nullopt;
    }
    std::wstring value(size, L'\0');
    const DWORD written = ::GetEnvironmentVariableW(wideName.c_str(), value.data(), size);
    value.resize(written);
    return winrt::to_string(value);
}

std::filesystem::path SettingsFilePath() {
    if (auto localAppData = ReadEnvironment("LOCALAPPDATA")) {
        return std::filesystem::path{winrt::to_hstring(*localAppData).c_str()} / L"Talmon" / L"settings.ini";
    }
    return std::filesystem::path{L"talmon-settings.ini"};
}

} // namespace

std::shared_ptr<AppServices> AppServices::Create() {
    auto session = std::make_shared<application::MonitorSession>(backend::createTalosConfigSource(),
                                                                 backend::createTalosClientFactory());
    auto monitor = std::make_shared<application::ClusterMonitor>(std::make_shared<domain::SystemClock>());
    auto store = std::make_shared<infrastructure::IniSettingsStore>(SettingsFilePath());
    return std::make_shared<AppServices>(std::move(session), std::move(monitor), std::move(store));
}

AppServices::AppServices(std::shared_ptr<application::MonitorSession> session,
                         std::shared_ptr<application::ClusterMonitor> monitor,
                         std::shared_ptr<application::ISettingsStore> settingsStore)
    : m_session{std::move(session)}, m_monitor{std::move(monitor)}, m_settingsStore{std::move(settingsStore)} {
    m_settings = m_settingsStore->load();
}

domain::Result<void> AppServices::UpdateSettings(application::AppSettings settings) {
    m_settings = std::move(settings);
    return m_settingsStore->save(m_settings);
}

std::filesystem::path AppServices::EffectiveTalosConfigPath() const {
    if (!m_settings.talosConfigPath.empty()) {
        return m_settings.talosConfigPath;
    }
    return application::resolveTalosConfigPath(ReadEnvironment).value_or(std::filesystem::path{});
}

domain::Result<void> AppServices::Connect(const std::filesystem::path& talosConfig, std::string context) {
    if (auto loaded = m_session->loadConfig(talosConfig); !loaded) {
        return loaded;
    }
    if (const auto names = m_session->contextNames(); std::ranges::find(names, context) == names.end()) {
        context = m_session->defaultContext();
    }
    if (auto connected = m_session->connect(context, m_settings.callTimeout); !connected) {
        return connected;
    }

    ++m_generation;
    m_monitor->reset();

    auto settings = m_settings;
    settings.talosConfigPath = talosConfig;
    settings.context = context;
    (void)UpdateSettings(std::move(settings)); // failing to persist preferences is not fatal

    const auto nodes = m_session->nodes();
    m_selectedNode = nodes.empty() ? std::string{} : nodes.front();
    Notify(m_connectionListeners);
    return {};
}

void AppServices::SelectNode(std::string node) {
    if (node == m_selectedNode) {
        return;
    }
    m_selectedNode = std::move(node);
    Notify(m_nodeListeners);
}

void AppServices::ReportStatus(StatusKind kind, std::wstring_view title, std::wstring_view message) const {
    if (m_statusSink) {
        m_statusSink(kind, title, message);
    }
}

void AppServices::ReportError(std::wstring_view title, const domain::Error& error) const {
    ReportStatus(StatusKind::Error, title, winrt::to_hstring(error.describe()));
}

void AppServices::ReportBusy(bool busy) const {
    if (m_busySink) {
        m_busySink(busy);
    }
}

void AppServices::Notify(const std::vector<Listener>& listeners) {
    for (const auto& listener : listeners) {
        listener();
    }
}

} // namespace talmon::ui

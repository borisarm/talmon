#pragma once

#include "talmon/application/ClusterMonitor.hpp"
#include "talmon/application/MonitorSession.hpp"
#include "talmon/application/Settings.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace talmon::ui {

enum class StatusKind { Success, Informational, Warning, Error };

/// Composition root and shared UI state. Pages receive it through IMonitorView::Attach
/// (dependency injection) instead of reaching for globals.
/// Everything except the session/monitor objects is meant to be used from the UI thread.
class AppServices {
public:
    using Listener = std::function<void()>;
    using StatusSink = std::function<void(StatusKind kind, std::wstring_view title, std::wstring_view message)>;

    /// Wires the concrete implementations (backend DLL + settings file).
    [[nodiscard]] static std::shared_ptr<AppServices> Create();

    AppServices(std::shared_ptr<application::MonitorSession> session,
                std::shared_ptr<application::ClusterMonitor> monitor,
                std::shared_ptr<application::ISettingsStore> settingsStore);

    [[nodiscard]] const std::shared_ptr<application::MonitorSession>& Session() const noexcept { return m_session; }
    [[nodiscard]] const std::shared_ptr<application::ClusterMonitor>& Monitor() const noexcept { return m_monitor; }

    // --- Settings ------------------------------------------------------------------------------
    [[nodiscard]] const application::AppSettings& Settings() const noexcept { return m_settings; }
    domain::Result<void> UpdateSettings(application::AppSettings settings);
    /// Path from the settings, or the talosctl default location.
    [[nodiscard]] std::filesystem::path EffectiveTalosConfigPath() const;

    // --- Connection ----------------------------------------------------------------------------
    /// Loads the talosconfig, connects to `context` (or its default context) and persists the choice.
    domain::Result<void> Connect(const std::filesystem::path& talosConfig, std::string context);
    /// Increments on every (re)connection so background results from a previous cluster can be dropped.
    [[nodiscard]] std::uint64_t Generation() const noexcept { return m_generation.load(); }

    // --- Node selection ------------------------------------------------------------------------
    [[nodiscard]] const std::string& SelectedNode() const noexcept { return m_selectedNode; }
    void SelectNode(std::string node);

    // --- Notifications (observer) --------------------------------------------------------------
    void OnConnectionChanged(Listener listener) { m_connectionListeners.push_back(std::move(listener)); }
    void OnSelectedNodeChanged(Listener listener) { m_nodeListeners.push_back(std::move(listener)); }
    void SetStatusSink(StatusSink sink) { m_statusSink = std::move(sink); }
    void SetBusySink(std::function<void(bool)> sink) { m_busySink = std::move(sink); }
    void ReportStatus(StatusKind kind, std::wstring_view title, std::wstring_view message = {}) const;
    void ReportError(std::wstring_view title, const domain::Error& error) const;
    void ReportBusy(bool busy) const;

    // --- Window --------------------------------------------------------------------------------
    winrt::Microsoft::UI::WindowId MainWindowId{};

private:
    static void Notify(const std::vector<Listener>& listeners);

    std::shared_ptr<application::MonitorSession> m_session;
    std::shared_ptr<application::ClusterMonitor> m_monitor;
    std::shared_ptr<application::ISettingsStore> m_settingsStore;
    application::AppSettings m_settings;
    std::atomic<std::uint64_t> m_generation{0};
    std::string m_selectedNode;
    std::vector<Listener> m_connectionListeners;
    std::vector<Listener> m_nodeListeners;
    StatusSink m_statusSink;
    std::function<void(bool)> m_busySink;
};

/// UTF-8 std::string -> hstring.
[[nodiscard]] inline winrt::hstring ToHString(std::string_view text) { return winrt::to_hstring(text); }
/// hstring -> UTF-8 std::string.
[[nodiscard]] inline std::string ToUtf8(winrt::hstring const& text) { return winrt::to_string(text); }

} // namespace talmon::ui

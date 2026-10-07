#pragma once

#include "AppServices.h"

namespace talmon::ui {

/// Contract between the shell (MainWindow) and each page. The shell owns the refresh cadence and
/// navigation; pages only know how to load and render their own data (Single Responsibility).
struct IMonitorView {
    virtual ~IMonitorView() = default;

    /// Injects the shared services. Called once, right after construction.
    virtual void Attach(std::shared_ptr<AppServices> services) = 0;
    /// Reloads data in the background and updates the UI when done. Must not block.
    virtual void Refresh() = 0;
    /// Whether the shell timer should refresh this view periodically.
    [[nodiscard]] virtual bool AutoRefresh() const { return true; }
    virtual void OnShown() { Refresh(); }
    virtual void OnHidden() {}
};

} // namespace talmon::ui

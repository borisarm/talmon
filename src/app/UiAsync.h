#pragma once

#include <winrt/Microsoft.UI.Dispatching.h>
#include <wil/cppwinrt_helpers.h>

namespace talmon::ui {

/// Runs `work` on the thread pool, then `apply(self, result)` back on the calling UI thread if
/// the owner (held weakly) is still alive. Keeps blocking gRPC calls off the UI thread.
template <typename WeakOwner, typename Work, typename Apply>
winrt::fire_and_forget RunInBackground(WeakOwner weakOwner, Work work, Apply apply) {
    auto dispatcher = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    co_await winrt::resume_background();
    auto result = work();
    co_await wil::resume_foreground(dispatcher);
    if (auto self = weakOwner.get()) {
        apply(self, std::move(result));
    }
}

} // namespace talmon::ui

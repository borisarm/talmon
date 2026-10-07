#pragma once

#include "talmon/application/ServiceHealth.hpp"

namespace talmon::ui {

enum class Tone { Success, Attention, Caution, Critical, Neutral };

/// Theme aware brush for a semantic tone (WinUI "SystemFillColor*Brush" resources).
[[nodiscard]] inline winrt::Microsoft::UI::Xaml::Media::Brush BrushFor(Tone tone) {
    using namespace winrt::Microsoft::UI::Xaml;
    const wchar_t* key = L"SystemFillColorNeutralBrush";
    winrt::Windows::UI::Color fallback = winrt::Microsoft::UI::Colors::Gray();
    switch (tone) {
    case Tone::Success:
        key = L"SystemFillColorSuccessBrush";
        fallback = winrt::Microsoft::UI::Colors::SeaGreen();
        break;
    case Tone::Attention:
        key = L"SystemFillColorAttentionBrush";
        fallback = winrt::Microsoft::UI::Colors::DodgerBlue();
        break;
    case Tone::Caution:
        key = L"SystemFillColorCautionBrush";
        fallback = winrt::Microsoft::UI::Colors::Orange();
        break;
    case Tone::Critical:
        key = L"SystemFillColorCriticalBrush";
        fallback = winrt::Microsoft::UI::Colors::Firebrick();
        break;
    case Tone::Neutral: break;
    }
    try {
        if (auto value = Application::Current().Resources().TryLookup(winrt::box_value(key))) {
            if (auto brush = value.try_as<Media::Brush>()) {
                return brush;
            }
        }
    } catch (...) {
        // Resource missing in this WinUI version: use the fallback colour.
    }
    return Media::SolidColorBrush{fallback};
}

[[nodiscard]] inline Tone ToneFor(application::ServiceCondition condition) noexcept {
    switch (condition) {
    case application::ServiceCondition::Healthy: return Tone::Success;
    case application::ServiceCondition::Running: return Tone::Attention;
    case application::ServiceCondition::Pending: return Tone::Caution;
    case application::ServiceCondition::Failed: return Tone::Critical;
    case application::ServiceCondition::Inactive: break;
    }
    return Tone::Neutral;
}

} // namespace talmon::ui

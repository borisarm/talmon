#pragma once

#include "ServiceItem.g.h"

#include "talmon/domain/Models.hpp"

namespace winrt::Talmon::implementation
{
    struct ServiceItem : ServiceItemT<ServiceItem>
    {
        explicit ServiceItem(talmon::domain::ServiceStatus const& service);

        hstring Id() const { return m_id; }
        hstring State() const { return m_state; }
        hstring Condition() const { return m_condition; }
        hstring Message() const { return m_message; }
        hstring LastEvent() const { return m_lastEvent; }
        Microsoft::UI::Xaml::Media::Brush StatusBrush() const { return m_statusBrush; }

    private:
        hstring m_id;
        hstring m_state;
        hstring m_condition;
        hstring m_message;
        hstring m_lastEvent;
        Microsoft::UI::Xaml::Media::Brush m_statusBrush{ nullptr };
    };
}

#include "pch.h"
#include "ServiceItem.h"
#if __has_include("ServiceItem.g.cpp")
#include "ServiceItem.g.cpp"
#endif

#include "AppServices.h"
#include "StatusBrushes.h"

#include <format>

using namespace talmon;

namespace winrt::Talmon::implementation
{
    namespace
    {
        hstring ConditionText(application::ServiceCondition condition)
        {
            switch (condition)
            {
            case application::ServiceCondition::Healthy: return L"Saludable";
            case application::ServiceCondition::Running: return L"En ejecución";
            case application::ServiceCondition::Pending: return L"Pendiente";
            case application::ServiceCondition::Inactive: return L"Inactivo";
            case application::ServiceCondition::Failed: return L"Con fallo";
            }
            return L"Desconocido";
        }

        std::string FormatEvent(domain::ServiceEvent const& event)
        {
            if (!event.timestamp)
            {
                return event.message;
            }
            auto const local = std::chrono::zoned_time{ std::chrono::current_zone(),
                std::chrono::floor<std::chrono::seconds>(*event.timestamp) };
            return std::format("{:%Y-%m-%d %H:%M:%S}  {}", local, event.message);
        }
    }

    ServiceItem::ServiceItem(domain::ServiceStatus const& service)
    {
        auto const condition = application::classify(service);
        m_id = ui::ToHString(service.id);
        m_state = ui::ToHString(service.state);
        m_condition = ConditionText(condition);
        m_message = ui::ToHString(service.health.lastMessage);
        if (!service.events.empty())
        {
            m_lastEvent = ui::ToHString(FormatEvent(service.events.back()));
        }
        m_statusBrush = ui::BrushFor(ui::ToneFor(condition));
    }
}

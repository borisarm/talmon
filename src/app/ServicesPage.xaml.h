#pragma once

#include "ServicesPage.g.h"

#include "IMonitorView.h"

namespace winrt::Talmon::implementation
{
    /// Talos system services (etcd, kubelet, apid, ...) of the selected node.
    struct ServicesPage : ServicesPageT<ServicesPage>, talmon::ui::IMonitorView
    {
        ServicesPage() = default;

        void Attach(std::shared_ptr<talmon::ui::AppServices> services) override;
        void Refresh() override;

    private:
        void Apply(std::uint64_t generation, std::string const& node,
            talmon::domain::Result<std::vector<talmon::domain::ServiceStatus>> result);

        std::shared_ptr<talmon::ui::AppServices> m_services;
        Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> m_items{
            single_threaded_observable_vector<Windows::Foundation::IInspectable>() };
        bool m_busy{ false };
    };
}

namespace winrt::Talmon::factory_implementation
{
    struct ServicesPage : ServicesPageT<ServicesPage, implementation::ServicesPage>
    {
    };
}

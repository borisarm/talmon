#pragma once

#include "talmon/domain/TalosApi.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace talmon::application {

/// Follows the log of one service on a background thread and delivers complete lines.
/// Callbacks run on the worker thread; consumers marshal to their UI thread themselves.
/// Destroying the session cancels the stream and joins the worker.
class LogStreamSession {
public:
    using LinesHandler = std::function<void(std::vector<std::string> lines)>;
    using FinishedHandler = std::function<void(const domain::Result<void>& outcome)>;

    LogStreamSession(std::shared_ptr<domain::ILogApi> api, domain::NodeName node, domain::LogRequest request,
                     LinesHandler onLines, FinishedHandler onFinished);
    ~LogStreamSession();

    LogStreamSession(const LogStreamSession&) = delete;
    LogStreamSession& operator=(const LogStreamSession&) = delete;

    void stop();
    [[nodiscard]] bool running() const noexcept { return running_.load(); }

private:
    void run(std::stop_token stop);

    std::shared_ptr<domain::ILogApi> api_;
    domain::NodeName node_;
    domain::LogRequest request_;
    LinesHandler onLines_;
    FinishedHandler onFinished_;
    std::atomic<bool> running_{true};
    std::jthread worker_; // last member: started after everything else is initialised
};

} // namespace talmon::application

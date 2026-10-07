#include "talmon/application/LogStreamSession.hpp"

#include "talmon/application/LogBuffer.hpp"

namespace talmon::application {

LogStreamSession::LogStreamSession(std::shared_ptr<domain::ILogApi> api, domain::NodeName node,
                                   domain::LogRequest request, LinesHandler onLines, FinishedHandler onFinished)
    : api_{std::move(api)},
      node_{std::move(node)},
      request_{std::move(request)},
      onLines_{std::move(onLines)},
      onFinished_{std::move(onFinished)},
      worker_{[this](std::stop_token stop) { run(std::move(stop)); }} {}

LogStreamSession::~LogStreamSession() { stop(); }

void LogStreamSession::stop() {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join();
    }
}

void LogStreamSession::run(std::stop_token stop) {
    LineAssembler assembler;
    const auto deliver = [this](std::vector<std::string> lines) {
        if (!lines.empty() && onLines_) {
            onLines_(std::move(lines));
        }
    };

    domain::Result<void> outcome =
        api_ ? api_->streamLogs(node_, request_, stop, [&](std::string_view chunk) { deliver(assembler.feed(chunk)); })
             : domain::fail(domain::ErrorCode::InvalidArgument, "no log api");

    deliver(assembler.flush());
    running_ = false;
    if (onFinished_) {
        onFinished_(outcome);
    }
}

} // namespace talmon::application

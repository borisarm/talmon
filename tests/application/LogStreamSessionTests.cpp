#include "talmon/application/LogStreamSession.hpp"

#include "fakes/MockTalosClient.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>
#include <future>
#include <mutex>

using namespace talmon;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Invoke;

namespace {

struct Collector {
    std::mutex mutex;
    std::vector<std::string> lines;
    std::promise<domain::Result<void>> finished;

    application::LogStreamSession::LinesHandler linesHandler() {
        return [this](std::vector<std::string> batch) {
            std::scoped_lock lock{mutex};
            lines.insert(lines.end(), batch.begin(), batch.end());
        };
    }
    application::LogStreamSession::FinishedHandler finishedHandler() {
        return [this](const domain::Result<void>& outcome) { finished.set_value(outcome); };
    }
};

} // namespace

TEST(LogStreamSession, DeliversCompleteLinesAndFlushesTail) {
    auto client = std::make_shared<fakes::MockTalosClient>();
    EXPECT_CALL(*client, streamLogs("n1", Field(&domain::LogRequest::serviceId, "etcd"), _, _))
        .WillOnce(Invoke([](auto&, auto&, std::stop_token, const domain::LogChunkHandler& onChunk) {
            onChunk("first li");
            onChunk("ne\nsecond\nthi");
            onChunk("rd");
            return domain::Result<void>{};
        }));

    Collector collector;
    auto finished = collector.finished.get_future();
    application::LogStreamSession session{client, "n1", {.serviceId = "etcd"}, collector.linesHandler(),
                                          collector.finishedHandler()};

    ASSERT_EQ(finished.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(finished.get());
    EXPECT_THAT(collector.lines, ElementsAre("first line", "second", "third"));
    EXPECT_FALSE(session.running());
}

TEST(LogStreamSession, StopCancelsABlockingStream) {
    auto client = std::make_shared<fakes::MockTalosClient>();
    EXPECT_CALL(*client, streamLogs(_, _, _, _))
        .WillOnce(Invoke([](auto&, auto&, std::stop_token stop, const domain::LogChunkHandler&) {
            std::mutex mutex;
            std::condition_variable_any cv;
            std::unique_lock lock{mutex};
            cv.wait(lock, stop, [] { return false; }); // blocks until stop is requested
            return domain::Result<void>{domain::fail(domain::ErrorCode::Cancelled, "stopped")};
        }));

    Collector collector;
    auto finished = collector.finished.get_future();
    application::LogStreamSession session{client, "n1", {.serviceId = "kubelet"}, collector.linesHandler(),
                                          collector.finishedHandler()};
    EXPECT_EQ(finished.wait_for(50ms), std::future_status::timeout);

    session.stop();

    ASSERT_EQ(finished.wait_for(5s), std::future_status::ready);
    const auto outcome = finished.get();
    ASSERT_FALSE(outcome);
    EXPECT_EQ(outcome.error().code, domain::ErrorCode::Cancelled);
}

TEST(LogStreamSession, ReportsMissingApi) {
    Collector collector;
    auto finished = collector.finished.get_future();
    application::LogStreamSession session{nullptr, "n1", {}, collector.linesHandler(), collector.finishedHandler()};

    ASSERT_EQ(finished.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(finished.get().error().code, domain::ErrorCode::InvalidArgument);
}

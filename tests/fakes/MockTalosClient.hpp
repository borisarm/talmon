#pragma once

#include "talmon/domain/Clock.hpp"
#include "talmon/domain/TalosApi.hpp"
#include "talmon/domain/TalosConfig.hpp"

#include <gmock/gmock.h>

namespace talmon::fakes {

class MockTalosClient : public domain::ITalosClient {
public:
    MOCK_METHOD(domain::Result<domain::VersionInfo>, version, (const domain::NodeName&), (override));
    MOCK_METHOD(domain::Result<std::string>, hostname, (const domain::NodeName&), (override));
    MOCK_METHOD(domain::Result<domain::MemoryInfo>, memory, (const domain::NodeName&), (override));
    MOCK_METHOD(domain::Result<domain::LoadAverage>, loadAverage, (const domain::NodeName&), (override));
    MOCK_METHOD(domain::Result<domain::SystemStats>, systemStats, (const domain::NodeName&), (override));
    MOCK_METHOD(domain::Result<std::vector<domain::ServiceStatus>>, services, (const domain::NodeName&),
                (override));
    MOCK_METHOD(domain::Result<std::vector<domain::ProcessInfo>>, processes, (const domain::NodeName&),
                (override));
    MOCK_METHOD(domain::Result<void>, streamLogs,
                (const domain::NodeName&, const domain::LogRequest&, std::stop_token,
                 const domain::LogChunkHandler&),
                (override));
};

class MockConfigSource : public domain::ITalosConfigSource {
public:
    MOCK_METHOD(domain::Result<domain::TalosConfig>, load, (const std::filesystem::path&), (override));
};

class MockClientFactory : public domain::ITalosClientFactory {
public:
    MOCK_METHOD(domain::Result<std::shared_ptr<domain::ITalosClient>>, create, (const domain::ConnectionSettings&),
                (override));
};

class FixedClock final : public domain::IClock {
public:
    explicit FixedClock(std::chrono::system_clock::time_point now) : now_{now} {}
    [[nodiscard]] std::chrono::system_clock::time_point now() const override { return now_; }

private:
    std::chrono::system_clock::time_point now_;
};

} // namespace talmon::fakes

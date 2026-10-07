#include "talmon/application/MonitorSession.hpp"

#include "fakes/MockTalosClient.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace talmon;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Return;

namespace {

domain::TalosConfig twoContexts() {
    return domain::TalosConfig{
        .currentContext = "prod",
        .contexts =
            {
                {.name = "lab", .endpoints = {"10.0.0.1"}, .nodes = {}, .caPem = "ca", .certificatePem = "crt", .privateKeyPem = "key"},
                {.name = "prod", .endpoints = {"192.168.1.10"}, .nodes = {"192.168.1.11", "192.168.1.12"},
                 .caPem = "CA", .certificatePem = "CRT", .privateKeyPem = "KEY"},
            },
    };
}

class MonitorSessionTest : public ::testing::Test {
protected:
    std::shared_ptr<fakes::MockConfigSource> source = std::make_shared<fakes::MockConfigSource>();
    std::shared_ptr<fakes::MockClientFactory> factory = std::make_shared<fakes::MockClientFactory>();
    application::MonitorSession session{source, factory};
};

} // namespace

TEST_F(MonitorSessionTest, ExposesContextsOfLoadedConfig) {
    EXPECT_CALL(*source, load(std::filesystem::path{"cfg"})).WillOnce(Return(twoContexts()));

    ASSERT_TRUE(session.loadConfig("cfg"));

    EXPECT_THAT(session.contextNames(), ElementsAre("lab", "prod"));
    EXPECT_EQ(session.defaultContext(), "prod");
    EXPECT_FALSE(session.connected());
}

TEST_F(MonitorSessionTest, InspectConfigDoesNotAffectActiveConnection) {
    EXPECT_CALL(*source, load(_)).WillRepeatedly(Return(twoContexts()));
    EXPECT_CALL(*factory, create(_))
        .WillOnce(Return(std::shared_ptr<domain::ITalosClient>{std::make_shared<fakes::MockTalosClient>()}));
    ASSERT_TRUE(session.loadConfig("cfg"));
    ASSERT_TRUE(session.connect("prod"));

    const auto names = session.inspectConfig("other");

    ASSERT_TRUE(names);
    EXPECT_THAT(*names, ElementsAre("lab", "prod"));
    EXPECT_TRUE(session.connected());
    EXPECT_EQ(session.activeContext(), "prod");
}

TEST_F(MonitorSessionTest, PropagatesLoadErrors) {
    EXPECT_CALL(*source, load(_)).WillOnce(Return(domain::fail(domain::ErrorCode::NotFound, "missing")));

    const auto result = session.loadConfig("nope");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, domain::ErrorCode::NotFound);
}

TEST_F(MonitorSessionTest, RejectsConfigWithoutContexts) {
    EXPECT_CALL(*source, load(_)).WillOnce(Return(domain::TalosConfig{}));
    EXPECT_EQ(session.loadConfig("empty").error().code, domain::ErrorCode::InvalidConfiguration);
}

TEST_F(MonitorSessionTest, ConnectBuildsClientFromContextCredentials) {
    EXPECT_CALL(*source, load(_)).WillOnce(Return(twoContexts()));
    auto client = std::make_shared<fakes::MockTalosClient>();
    EXPECT_CALL(*factory, create(::testing::AllOf(
                              Field(&domain::ConnectionSettings::endpoints, ElementsAre("192.168.1.10")),
                              Field(&domain::ConnectionSettings::caPem, "CA"),
                              Field(&domain::ConnectionSettings::certificatePem, "CRT"),
                              Field(&domain::ConnectionSettings::privateKeyPem, "KEY"),
                              Field(&domain::ConnectionSettings::callTimeout, std::chrono::milliseconds{3000}))))
        .WillOnce(Return(std::shared_ptr<domain::ITalosClient>{client}));

    ASSERT_TRUE(session.loadConfig("cfg"));
    ASSERT_TRUE(session.connect("prod", std::chrono::seconds{3}));

    EXPECT_TRUE(session.connected());
    EXPECT_EQ(session.activeContext(), "prod");
    EXPECT_EQ(session.client(), client);
    EXPECT_THAT(session.nodes(), ElementsAre("192.168.1.11", "192.168.1.12"));
}

TEST_F(MonitorSessionTest, NodesFallBackToEndpoints) {
    EXPECT_CALL(*source, load(_)).WillOnce(Return(twoContexts()));
    EXPECT_CALL(*factory, create(_))
        .WillOnce(Return(std::shared_ptr<domain::ITalosClient>{std::make_shared<fakes::MockTalosClient>()}));

    ASSERT_TRUE(session.loadConfig("cfg"));
    ASSERT_TRUE(session.connect("lab"));

    EXPECT_THAT(session.nodes(), ElementsAre("10.0.0.1"));
}

TEST_F(MonitorSessionTest, ConnectFailsForUnknownContext) {
    EXPECT_CALL(*source, load(_)).WillOnce(Return(twoContexts()));
    EXPECT_CALL(*factory, create(_)).Times(0);

    ASSERT_TRUE(session.loadConfig("cfg"));

    EXPECT_EQ(session.connect("staging").error().code, domain::ErrorCode::NotFound);
}

TEST_F(MonitorSessionTest, ConnectWithoutConfigFails) {
    EXPECT_EQ(session.connect("prod").error().code, domain::ErrorCode::InvalidConfiguration);
}

TEST_F(MonitorSessionTest, DisconnectDropsClient) {
    EXPECT_CALL(*source, load(_)).WillOnce(Return(twoContexts()));
    EXPECT_CALL(*factory, create(_))
        .WillOnce(Return(std::shared_ptr<domain::ITalosClient>{std::make_shared<fakes::MockTalosClient>()}));
    ASSERT_TRUE(session.loadConfig("cfg"));
    ASSERT_TRUE(session.connect("prod"));

    session.disconnect();

    EXPECT_FALSE(session.connected());
    EXPECT_TRUE(session.nodes().empty());
}

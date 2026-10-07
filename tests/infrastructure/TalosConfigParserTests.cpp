#include "talmon/infrastructure/TalosConfigParser.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace talmon;
using ::testing::ElementsAre;
using ::testing::HasSubstr;

TEST(Base64Decode, DecodesWithAndWithoutPadding) {
    EXPECT_EQ(infrastructure::base64Decode("aGVsbG8="), "hello");
    EXPECT_EQ(infrastructure::base64Decode("aGVsbG8gd29ybGQ="), "hello world");
    EXPECT_EQ(infrastructure::base64Decode("YQ=="), "a");
    EXPECT_EQ(infrastructure::base64Decode("YWJj"), "abc");
    EXPECT_EQ(infrastructure::base64Decode(""), "");
}

TEST(Base64Decode, IgnoresWhitespace) { EXPECT_EQ(infrastructure::base64Decode("aGVs\nbG8=\r\n"), "hello"); }

TEST(Base64Decode, RejectsInvalidInput) {
    EXPECT_FALSE(infrastructure::base64Decode("aGVs*G8="));
    EXPECT_FALSE(infrastructure::base64Decode("YQ==YQ=="));
    EXPECT_FALSE(infrastructure::base64Decode("Y"));
}

namespace {

// ca = "-----CA-----", crt = "-----CRT-----", key = "-----KEY-----"
constexpr std::string_view kTalosConfig = R"(
context: prod
contexts:
    prod:
        endpoints:
            - 192.168.1.10
            - 192.168.1.11
        nodes:
            - 192.168.1.20
        ca: LS0tLS1DQS0tLS0t
        crt: LS0tLS1DUlQtLS0tLQ==
        key: LS0tLS1LRVktLS0tLQ==
    lab:
        endpoints:
            - 10.5.0.2
        ca: LS0tLS1DQS0tLS0t
        crt: LS0tLS1DUlQtLS0tLQ==
        key: LS0tLS1LRVktLS0tLQ==
)";

} // namespace

TEST(ParseTalosConfig, ReadsContextsAndDecodesCredentials) {
    const auto config = infrastructure::parseTalosConfig(kTalosConfig);

    ASSERT_TRUE(config) << config.error().describe();
    EXPECT_EQ(config->currentContext, "prod");
    ASSERT_EQ(config->contexts.size(), 2u);

    const auto* prod = config->find("prod");
    ASSERT_NE(prod, nullptr);
    EXPECT_THAT(prod->endpoints, ElementsAre("192.168.1.10", "192.168.1.11"));
    EXPECT_THAT(prod->nodes, ElementsAre("192.168.1.20"));
    EXPECT_EQ(prod->caPem, "-----CA-----");
    EXPECT_EQ(prod->certificatePem, "-----CRT-----");
    EXPECT_EQ(prod->privateKeyPem, "-----KEY-----");

    const auto* lab = config->find("lab");
    ASSERT_NE(lab, nullptr);
    EXPECT_TRUE(lab->nodes.empty());
}

TEST(ParseTalosConfig, SortsContextsByName) {
    const auto config = infrastructure::parseTalosConfig(kTalosConfig);
    ASSERT_TRUE(config);
    EXPECT_EQ(config->contexts.front().name, "lab");
}

TEST(ParseTalosConfig, ReportsMissingCredentials) {
    const auto config = infrastructure::parseTalosConfig(R"(
context: a
contexts:
  a:
    endpoints: [1.2.3.4]
    ca: LS0tLS1DQS0tLS0t
    crt: LS0tLS1DUlQtLS0tLQ==
)");
    ASSERT_FALSE(config);
    EXPECT_EQ(config.error().code, domain::ErrorCode::InvalidConfiguration);
    EXPECT_THAT(config.error().message, HasSubstr("'key'"));
}

TEST(ParseTalosConfig, ReportsInvalidBase64) {
    const auto config = infrastructure::parseTalosConfig(R"(
contexts:
  a:
    ca: "%%%"
    crt: LS0tLS1DUlQtLS0tLQ==
    key: LS0tLS1LRVktLS0tLQ==
)");
    ASSERT_FALSE(config);
    EXPECT_THAT(config.error().message, HasSubstr("base64"));
}

TEST(ParseTalosConfig, RejectsMalformedDocuments) {
    EXPECT_FALSE(infrastructure::parseTalosConfig("contexts: [1, 2"));
    EXPECT_FALSE(infrastructure::parseTalosConfig("- just\n- a list\n"));
    EXPECT_FALSE(infrastructure::parseTalosConfig("context: x\n"));
}

TEST(YamlTalosConfigSource, ReportsMissingFile) {
    infrastructure::YamlTalosConfigSource source;
    const auto config = source.load(std::filesystem::temp_directory_path() / "talmon-does-not-exist.yaml");
    ASSERT_FALSE(config);
    EXPECT_EQ(config.error().code, domain::ErrorCode::NotFound);
}

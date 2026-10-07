#include "talmon/application/ServiceHealth.hpp"

#include <gtest/gtest.h>

using namespace talmon::application;
using talmon::domain::ServiceHealth;
using talmon::domain::ServiceStatus;

namespace {

ServiceStatus service(std::string state, bool unknown = true, bool healthy = false) {
    return ServiceStatus{.id = "svc", .state = std::move(state), .health = ServiceHealth{unknown, healthy, {}}, .events = {}};
}

} // namespace

struct ClassifyCase {
    ServiceStatus status;
    ServiceCondition expected;
};

class ClassifyTest : public ::testing::TestWithParam<ClassifyCase> {};

TEST_P(ClassifyTest, MapsStateAndHealth) { EXPECT_EQ(classify(GetParam().status), GetParam().expected); }

INSTANTIATE_TEST_SUITE_P(
    States, ClassifyTest,
    ::testing::Values(ClassifyCase{service("Running", false, true), ServiceCondition::Healthy},
                      ClassifyCase{service("Running", true), ServiceCondition::Running},
                      ClassifyCase{service("Running", false, false), ServiceCondition::Failed},
                      ClassifyCase{service("Failed"), ServiceCondition::Failed},
                      ClassifyCase{service("Finished"), ServiceCondition::Inactive},
                      ClassifyCase{service("Skipped"), ServiceCondition::Inactive},
                      ClassifyCase{service("Waiting"), ServiceCondition::Pending},
                      ClassifyCase{service("Preparing"), ServiceCondition::Pending}));

TEST(Summarize, CountsEveryCondition) {
    const std::vector services{service("Running", false, true), service("Running", false, true),
                               service("Running"),              service("Waiting"),
                               service("Finished"),             service("Failed")};

    const auto summary = summarize(services);

    EXPECT_EQ(summary, (ServicesSummary{.total = 6, .healthy = 2, .running = 1, .pending = 1, .inactive = 1, .failed = 1}));
    EXPECT_EQ(summary.up(), 3u);
}

TEST(Summarize, HandlesEmptyInput) { EXPECT_EQ(summarize({}), ServicesSummary{}); }

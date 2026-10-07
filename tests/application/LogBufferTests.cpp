#include "talmon/application/LogBuffer.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace talmon::application;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(LineAssembler, SplitsCompleteLines) {
    LineAssembler assembler;
    EXPECT_THAT(assembler.feed("a\nb\n"), ElementsAre("a", "b"));
}

TEST(LineAssembler, JoinsLinesSplitAcrossChunks) {
    LineAssembler assembler;
    EXPECT_THAT(assembler.feed("hel"), IsEmpty());
    EXPECT_THAT(assembler.feed("lo\r\nwor"), ElementsAre("hello"));
    EXPECT_THAT(assembler.feed("ld\n"), ElementsAre("world"));
}

TEST(LineAssembler, KeepsEmptyLines) {
    LineAssembler assembler;
    EXPECT_THAT(assembler.feed("\n\nx\n"), ElementsAre("", "", "x"));
}

TEST(LineAssembler, FlushReturnsPartialLineOnce) {
    LineAssembler assembler;
    (void)assembler.feed("tail");
    EXPECT_THAT(assembler.flush(), ElementsAre("tail"));
    EXPECT_THAT(assembler.flush(), IsEmpty());
}

TEST(BoundedLineBuffer, EvictsOldestLines) {
    BoundedLineBuffer buffer{3};
    EXPECT_EQ(buffer.append({"1", "2"}), 0u);
    EXPECT_EQ(buffer.append({"3", "4", "5"}), 2u);
    EXPECT_THAT(buffer.lines(), ElementsAre("3", "4", "5"));
    EXPECT_EQ(buffer.joined(), "3\n4\n5\n");
}

TEST(BoundedLineBuffer, CapacityIsAtLeastOne) {
    BoundedLineBuffer buffer{0};
    EXPECT_EQ(buffer.capacity(), 1u);
    buffer.append({"a", "b"});
    EXPECT_THAT(buffer.lines(), ElementsAre("b"));
}

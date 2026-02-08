#include <gtest/gtest.h>
#include "buttons/impl/SwitchGroup.h"
#include "soc/mock/MockDigitalInput.h"

using namespace testing;
using namespace soc::mock;

class SwitchGroupTest : public Test {
protected:
    MockDigitalInput bit0{0, true};
    MockDigitalInput bit1{1, true};
    MockDigitalInput bit2{2, true};
    SwitchGroup switchGroup{bit0, bit1, bit2};
};

TEST_F(SwitchGroupTest, ReadReturnsZeroWhenAllAreLow) {
    bit0.setActive(false);
    bit1.setActive(false);
    bit2.setActive(false);
    
    EXPECT_EQ(0, switchGroup.read());
}

TEST_F(SwitchGroupTest, ReadReturnsSevenWhenAllAreHigh) {
    bit0.setActive(true);
    bit1.setActive(true);
    bit2.setActive(true);
    
    EXPECT_EQ(7, switchGroup.read());
}

TEST_F(SwitchGroupTest, ReadReturnsOneWhenOnlyBit0IsHigh) {
    bit0.setActive(true);
    bit1.setActive(false);
    bit2.setActive(false);
    
    EXPECT_EQ(1, switchGroup.read());
}

TEST_F(SwitchGroupTest, ReadReturnsTwoWhenOnlyBit1IsHigh) {
    bit0.setActive(false);
    bit1.setActive(true);
    bit2.setActive(false);
    
    EXPECT_EQ(2, switchGroup.read());
}

TEST_F(SwitchGroupTest, ReadReturnsFourWhenOnlyBit2IsHigh) {
    bit0.setActive(false);
    bit1.setActive(false);
    bit2.setActive(true);
    
    EXPECT_EQ(4, switchGroup.read());
}

TEST_F(SwitchGroupTest, ReadReturnsThreeWhenBit0AndBit1AreHigh) {
    bit0.setActive(true);
    bit1.setActive(true);
    bit2.setActive(false);
    
    EXPECT_EQ(3, switchGroup.read());
}

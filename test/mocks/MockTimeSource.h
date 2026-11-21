#pragma once

#include <gmock/gmock.h>
#include "domain/TimeSource.h"

namespace aviator::test {

class MockTimeSource : public domain::TimeSource {
public:
    MOCK_METHOD(uint64_t, now, (), (override));
    MOCK_METHOD(bool, asTimeComponents, (domain::TimeComponents& components), (override));
};

} // namespace aviator::test

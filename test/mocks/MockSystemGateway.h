#pragma once

#include <gmock/gmock.h>
#include "domain/SystemGateway.h"

namespace aviator::test {

class MockSystemGateway : public domain::SystemGateway {
public:
    MOCK_METHOD(void, restart, (), (override));
    MOCK_METHOD(void, resetWifiCredentials, (), (override));
};

} // namespace aviator::test

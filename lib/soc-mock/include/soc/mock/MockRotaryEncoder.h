#pragma once

#include <soc/api/IRotaryEncoder.h>

namespace soc {
namespace mock {

class MockRotaryEncoder : public soc::api::IRotaryEncoder {
public:
    void update() override {}

    int32_t getDelta() override {
        int32_t d = _delta;
        _delta = 0;
        return d;
    }

    void reset() override {
        _delta = 0;
    }

    // Checking/Setting for tests
    void setDelta(int32_t delta) {
        _delta = delta;
    }

private:
    int32_t _delta{0};
};

} // namespace mock
} // namespace soc

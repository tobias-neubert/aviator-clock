#pragma once

#include "domain/Time.h"
#include "domain/TimeSource.h"

namespace aviator::test {

/**
 * @brief In-memory test double (Fake) for domain::TimeSource with controllable time components and epoch.
 */
class FakeTimeSource : public domain::TimeSource {
public:
    FakeTimeSource(uint64_t epochMilliseconds = 0, bool isSynchronized = true)
        : _epochMilliseconds(epochMilliseconds), _isSynchronized(isSynchronized) {}

    uint64_t now() override {
        return _isSynchronized ? _epochMilliseconds : 0;
    }

    bool asTimeComponents(domain::TimeComponents& components) override {
        if (!_isSynchronized) {
            return false;
        }
        components = _components;
        return true;
    }

    void setTime(const domain::TimeComponents& components, uint64_t epochMilliseconds = 0) {
        _components = components;
        _epochMilliseconds = epochMilliseconds;
        _isSynchronized = true;
    }

    void setSynchronized(bool isSynchronized) {
        _isSynchronized = isSynchronized;
    }

    bool isSynchronized() const {
        return _isSynchronized;
    }

private:
    domain::TimeComponents _components{0, 0, 0};
    uint64_t _epochMilliseconds{0};
    bool _isSynchronized{true};
};

} // namespace aviator::test

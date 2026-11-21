#include "domain/OperatingStateMachine.h"
#include "domain/SystemGateway.h"

namespace aviator::domain {

OperatingStateMachine::OperatingStateMachine(OperatingMode initialMode)
    : _currentMode(initialMode) {}

OperatingMode OperatingStateMachine::getCurrentMode() const {
    return _currentMode.load(std::memory_order_acquire);
}

bool OperatingStateMachine::isInitial() const {
    return _currentMode.load(std::memory_order_acquire) == OperatingMode::Initial;
}

bool OperatingStateMachine::isNormalRunning() const {
    return _currentMode.load(std::memory_order_acquire) == OperatingMode::NormalRunning;
}

bool OperatingStateMachine::isSetupMode() const {
    return _currentMode.load(std::memory_order_acquire) == OperatingMode::SetupMode;
}

void OperatingStateMachine::setObserver(OperatingModeObserver* observer) {
    _observer = observer;
}

void OperatingStateMachine::setSystemGateway(SystemGateway* systemGateway) {
    _systemGateway = systemGateway;
}

void OperatingStateMachine::transitionTo(OperatingMode targetMode) {
    OperatingMode previousMode = _currentMode.load(std::memory_order_acquire);
    while (previousMode != targetMode) {
        if (_currentMode.compare_exchange_weak(previousMode, targetMode,
                                               std::memory_order_acq_rel,
                                               std::memory_order_acquire)) {
            if (_observer != nullptr) {
                _observer->onOperatingModeChanged(previousMode, targetMode);
            }
            break;
        }
    }
}

void OperatingStateMachine::onStartupCompleted() {
    if (_currentMode.load(std::memory_order_acquire) == OperatingMode::Initial) {
        transitionTo(OperatingMode::NormalRunning);
    }
}

void OperatingStateMachine::onSetupButtonLongPressed() {
    const OperatingMode mode = _currentMode.load(std::memory_order_acquire);
    if (mode == OperatingMode::NormalRunning) {
        transitionTo(OperatingMode::SetupMode);
    } else if (mode == OperatingMode::SetupMode) {
        transitionTo(OperatingMode::NormalRunning);
    }
}

void OperatingStateMachine::onSetupButtonFactoryReset() {
    if (_systemGateway != nullptr) {
        _systemGateway->resetWifiCredentials();
        _systemGateway->restart();
    }
}

void OperatingStateMachine::enterSetupMode() {
    transitionTo(OperatingMode::SetupMode);
}

void OperatingStateMachine::exitSetupMode() {
    transitionTo(OperatingMode::NormalRunning);
}

} // namespace aviator::domain

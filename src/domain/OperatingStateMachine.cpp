#include "domain/OperatingStateMachine.h"
#include "domain/SystemGateway.h"

namespace aviator::domain {

OperatingStateMachine::OperatingStateMachine(OperatingMode initialMode)
    : _currentMode(initialMode) {}

OperatingMode OperatingStateMachine::getCurrentMode() const {
    return _currentMode;
}

bool OperatingStateMachine::isInitial() const {
    return _currentMode == OperatingMode::Initial;
}

bool OperatingStateMachine::isNormalRunning() const {
    return _currentMode == OperatingMode::NormalRunning;
}

bool OperatingStateMachine::isSetupMode() const {
    return _currentMode == OperatingMode::SetupMode;
}

void OperatingStateMachine::setObserver(OperatingModeObserver* observer) {
    _observer = observer;
}

void OperatingStateMachine::setSystemGateway(SystemGateway* systemGateway) {
    _systemGateway = systemGateway;
}

void OperatingStateMachine::transitionTo(OperatingMode targetMode) {
    if (_currentMode != targetMode) {
        const OperatingMode previousMode = _currentMode;
        _currentMode = targetMode;
        if (_observer != nullptr) {
            _observer->onOperatingModeChanged(previousMode, _currentMode);
        }
    }
}

void OperatingStateMachine::onStartupCompleted() {
    if (_currentMode == OperatingMode::Initial) {
        transitionTo(OperatingMode::NormalRunning);
    }
}

void OperatingStateMachine::onSetupButtonLongPressed() {
    if (_currentMode == OperatingMode::NormalRunning) {
        transitionTo(OperatingMode::SetupMode);
    } else if (_currentMode == OperatingMode::SetupMode) {
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

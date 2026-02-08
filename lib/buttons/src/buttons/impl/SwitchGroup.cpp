#include "buttons/impl/SwitchGroup.h"

SwitchGroup::SwitchGroup(soc::api::IDigitalInput& bit0Input, soc::api::IDigitalInput& bit1Input, soc::api::IDigitalInput& bit2Input)
    : _bit0Input(bit0Input), _bit1Input(bit1Input), _bit2Input(bit2Input)
{
}

uint8_t SwitchGroup::read() const {
    uint8_t value = 0;
    if (_bit0Input.isActive()) {
        value |= 1;
    }
    if (_bit1Input.isActive()) {
        value |= 2;
    }
    if (_bit2Input.isActive()) {
        value |= 4;
    }
    return value;
}

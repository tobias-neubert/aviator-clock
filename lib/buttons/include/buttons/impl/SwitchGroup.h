#pragma once

#include <soc/api/IDigitalInput.h>
#include <cstdint>

/**
 * @brief Represents a group of 3 switches acting as a 3-bit selector.
 */
class SwitchGroup {
public:
    /**
     * @brief Construct a new Switch Group object
     * 
     * @param bit0Input The input for the least significant bit (2^0 = 1).
     * @param bit1Input The input for the middle bit (2^1 = 2).
     * @param bit2Input The input for the most significant bit (2^2 = 4).
     */
    SwitchGroup(soc::api::IDigitalInput& bit0Input, soc::api::IDigitalInput& bit1Input, soc::api::IDigitalInput& bit2Input);

    /**
     * @brief Reads the current state of the switches and returns the value.
     * 
     * @return uint8_t A value between 0 and 7 representing the binary state of the switches.
     */
    uint8_t read() const;

private:
    soc::api::IDigitalInput& _bit0Input;
    soc::api::IDigitalInput& _bit1Input;
    soc::api::IDigitalInput& _bit2Input;
};

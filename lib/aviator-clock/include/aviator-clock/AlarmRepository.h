#pragma once

#include <soc/api/IPersistentStorage.h>
#include "aviator-clock/Time.h"

namespace aviator_clock
{

    class AlarmRepository
    {
    public:
        explicit AlarmRepository(soc::api::IPersistentStorage &storage);

        void begin(); // Initializes storage connection

        Time getAlarm(uint8_t slot);
        void setAlarm(uint8_t slot, const Time &time);

    private:
        soc::api::IPersistentStorage &_storage;

        std::string getHourKey(uint8_t slot) const;
        std::string getMinuteKey(uint8_t slot) const;
    };

} // namespace aviator_clock

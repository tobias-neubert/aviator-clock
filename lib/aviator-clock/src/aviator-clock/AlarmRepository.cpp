#include "aviator-clock/AlarmRepository.h"
#include <cstdio>

namespace aviator_clock
{

    AlarmRepository::AlarmRepository(soc::api::IPersistentStorage &storage)
        : _storage(storage)
    {
    }

    void AlarmRepository::begin()
    {
        _storage.begin("alarms", false);
    }

    Time AlarmRepository::getAlarm(uint8_t slot)
    {
        if (slot > 7) 
            return {0, 0, 0}; // Boundary check

        uint32_t hour = _storage.getUInt(getHourKey(slot), 0); // Default to 00:00
        uint32_t minute = _storage.getUInt(getMinuteKey(slot), 0);

        return {static_cast<uint8_t>(hour), static_cast<uint8_t>(minute), 0};
    }

    void AlarmRepository::setAlarm(uint8_t slot, const Time &time)
    {
        if (slot > 7)
            return;

        _storage.putUInt(getHourKey(slot), time.hours);
        _storage.putUInt(getMinuteKey(slot), time.minutes);
    }

    std::string AlarmRepository::getHourKey(uint8_t slot) const
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "a_%d_h", slot);
        return std::string(buf);
    }

    std::string AlarmRepository::getMinuteKey(uint8_t slot) const
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "a_%d_m", slot);
        return std::string(buf);
    }

} // namespace aviator_clock

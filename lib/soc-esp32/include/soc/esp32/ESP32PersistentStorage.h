#pragma once

#include <soc/api/IPersistentStorage.h>
#include <Preferences.h>

namespace soc
{
    namespace esp32
    {
        class ESP32PersistentStorage : public soc::api::IPersistentStorage
        {
        public:
            bool begin(const std::string &namespaceName, bool readOnly = false) override;
            size_t putString(const std::string &key, const std::string &value) override;
            std::string getString(const std::string &key, const std::string &defaultValue = "") override;
            size_t putUInt(const std::string &key, uint32_t value) override;
            uint32_t getUInt(const std::string &key, uint32_t defaultValue = 0) override;

        private:
            Preferences _prefs;
        };

    } // namespace esp32
} // namespace soc

#pragma once

#include <soc/api/IPersistentStorage.h>
#include <map>
#include <string>

namespace soc
{
    namespace mock
    {

        class MockPersistentStorage : public soc::api::IPersistentStorage
        {
        public:
            bool begin(const std::string &namespaceName, bool readOnly = false) override
            {
                currentNamespace = namespaceName;
                return true;
            }

            size_t putString(const std::string &key, const std::string &value) override
            {
                stringStorage[key] = value;
                return value.length();
            }

            std::string getString(const std::string &key, const std::string &defaultValue = "") override
            {
                if (stringStorage.find(key) != stringStorage.end())
                {
                    return stringStorage[key];
                }
                return defaultValue;
            }

            size_t putUInt(const std::string &key, uint32_t value) override
            {
                uintStorage[key] = value;
                return 4; // Size of uint32
            }

            uint32_t getUInt(const std::string &key, uint32_t defaultValue = 0) override
            {
                if (uintStorage.find(key) != uintStorage.end())
                {
                    return uintStorage[key];
                }
                return defaultValue;
            }

            // Checking methods for tests
            bool hasUInt(const std::string &key) const
            {
                return uintStorage.find(key) != uintStorage.end();
            }

        private:
            std::string currentNamespace;
            std::map<std::string, std::string> stringStorage;
            std::map<std::string, uint32_t> uintStorage;
        };

    } // namespace mock
} // namespace soc

#pragma once

#include <string>
#include <cstdint>

namespace soc
{
    namespace api
    {

        /**
         * @brief Interface for persistent storage (Non-Volatile Storage).
         *
         * Allows storing and retrieving simple data types across reboots.
         * This abstracts underlying mechanisms like ESP32 NVS (Preferences) or EEPROM.
         */
        class IPersistentStorage
        {
        public:
            virtual ~IPersistentStorage() = default;

            /**
             * @brief Prepare the namespace (partition) for read/write.
             * Some implementations might need explicit opening.
             * @param namespaceName The name of the storage namespace.
             * @param readOnly true to open in read-only mode.
             * @return true if successful.
             */
            virtual bool begin(const std::string &namespaceName, bool readOnly = false) = 0;

            /**
             * @brief Stores a string value.
             * @param key The key to store the value under.
             * @param value The value to store.
             * @return Number of types written.
             */
            virtual size_t putString(const std::string &key, const std::string &value) = 0;

            /**
             * @brief Retrieves a string value.
             * @param key The key to retrieve.
             * @param defaultValue The value to return if the key does not exist.
             * @return The stored value or the default value.
             */
            virtual std::string getString(const std::string &key, const std::string &defaultValue = "") = 0;

            /**
             * @brief Stores an unsigned integer value (32-bit).
             * @param key The key to store the value under.
             * @param value The value to store.
             * @return Number of types written.
             */
            virtual size_t putUInt(const std::string &key, uint32_t value) = 0;

            /**
             * @brief Retrieves an unsigned integer value (32-bit).
             * @param key The key to retrieve.
             * @param defaultValue The value to return if the key does not exist.
             * @return The stored value or the default value.
             */
            virtual uint32_t getUInt(const std::string &key, uint32_t defaultValue = 0) = 0;
        };

    } // namespace api
} // namespace soc

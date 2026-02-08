#include "soc/esp32/ESP32PersistentStorage.h"

namespace soc {
namespace esp32 {

bool ESP32PersistentStorage::begin(const std::string& namespaceName, bool readOnly) {
    return _prefs.begin(namespaceName.c_str(), readOnly);
}

size_t ESP32PersistentStorage::putString(const std::string& key, const std::string& value) {
    return _prefs.putString(key.c_str(), value.c_str());
}

std::string ESP32PersistentStorage::getString(const std::string& key, const std::string& defaultValue) {
    return _prefs.getString(key.c_str(), defaultValue.c_str()).c_str();
}

size_t ESP32PersistentStorage::putUInt(const std::string& key, uint32_t value) {
    return _prefs.putUInt(key.c_str(), value);
}

uint32_t ESP32PersistentStorage::getUInt(const std::string& key, uint32_t defaultValue) {
    return _prefs.getUInt(key.c_str(), defaultValue);
}

} // namespace esp32
} // namespace soc

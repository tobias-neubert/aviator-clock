#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <cstring>

/**
 * @brief Native host mock implementation of ESP32 Arduino Preferences.
 * 
 * Provides an in-memory key-value store simulating ESP32 Non-Volatile Storage (NVS)
 * for native host unit tests (GoogleTest).
 */
class Preferences {
public:
    Preferences() = default;
    ~Preferences() {
        end();
    }

    bool begin(const char* name, bool readOnly = false, const char* /* partition_label */ = nullptr) {
        if (s_simulateBeginFailure) {
            return false;
        }
        _currentNamespace = (name != nullptr) ? name : "";
        _readOnly = readOnly;
        _isOpen = true;
        return true;
    }

    void end() {
        _isOpen = false;
    }

    bool clear() {
        if (!_isOpen || _readOnly) {
            return false;
        }
        s_storage[_currentNamespace].clear();
        return true;
    }

    bool remove(const char* key) {
        if (!_isOpen || _readOnly || key == nullptr) {
            return false;
        }
        return s_storage[_currentNamespace].erase(key) > 0;
    }

    size_t putUChar(const char* key, uint8_t value) {
        if (!_isOpen || _readOnly || key == nullptr || s_simulateWriteFailure) {
            return 0;
        }
        s_storage[_currentNamespace][key] = std::to_string(value);
        return sizeof(value);
    }

    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
        if (!_isOpen || key == nullptr) {
            return defaultValue;
        }
        auto& currentNamespaceStorage = s_storage[_currentNamespace];
        auto entryIterator = currentNamespaceStorage.find(key);
        if (entryIterator == currentNamespaceStorage.end()) {
            return defaultValue;
        }
        return static_cast<uint8_t>(std::stoul(entryIterator->second));
    }

    size_t putUShort(const char* key, uint16_t value) {
        if (!_isOpen || _readOnly || key == nullptr || s_simulateWriteFailure) {
            return 0;
        }
        s_storage[_currentNamespace][key] = std::to_string(value);
        return sizeof(value);
    }

    uint16_t getUShort(const char* key, uint16_t defaultValue = 0) {
        if (!_isOpen || key == nullptr) {
            return defaultValue;
        }
        auto& currentNamespaceStorage = s_storage[_currentNamespace];
        auto entryIterator = currentNamespaceStorage.find(key);
        if (entryIterator == currentNamespaceStorage.end()) {
            return defaultValue;
        }
        return static_cast<uint16_t>(std::stoul(entryIterator->second));
    }

    size_t putBool(const char* key, bool value) {
        if (!_isOpen || _readOnly || key == nullptr || s_simulateWriteFailure) {
            return 0;
        }
        s_storage[_currentNamespace][key] = value ? "1" : "0";
        return sizeof(value);
    }

    bool getBool(const char* key, bool defaultValue = false) {
        if (!_isOpen || key == nullptr) {
            return defaultValue;
        }
        auto& currentNamespaceStorage = s_storage[_currentNamespace];
        auto entryIterator = currentNamespaceStorage.find(key);
        if (entryIterator == currentNamespaceStorage.end()) {
            return defaultValue;
        }
        return entryIterator->second == "1";
    }

    size_t putUInt(const char* key, uint32_t value) {
        if (!_isOpen || _readOnly || key == nullptr || s_simulateWriteFailure) {
            return 0;
        }
        s_storage[_currentNamespace][key] = std::to_string(value);
        return sizeof(value);
    }

    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
        if (!_isOpen || key == nullptr) {
            return defaultValue;
        }
        auto& currentNamespaceStorage = s_storage[_currentNamespace];
        auto entryIterator = currentNamespaceStorage.find(key);
        if (entryIterator == currentNamespaceStorage.end()) {
            return defaultValue;
        }
        return static_cast<uint32_t>(std::stoul(entryIterator->second));
    }

    size_t putString(const char* key, const char* value) {
        if (!_isOpen || _readOnly || key == nullptr || value == nullptr || s_simulateWriteFailure) {
            return 0;
        }
        s_storage[_currentNamespace][key] = value;
        return strlen(value);
    }

    std::string getString(const char* key, const std::string& defaultValue = "") {
        if (!_isOpen || key == nullptr) {
            return defaultValue;
        }
        auto& currentNamespaceStorage = s_storage[_currentNamespace];
        auto entryIterator = currentNamespaceStorage.find(key);
        if (entryIterator == currentNamespaceStorage.end()) {
            return defaultValue;
        }
        return entryIterator->second;
    }

    bool isKey(const char* key) {
        if (!_isOpen || key == nullptr) {
            return false;
        }
        auto& currentNamespaceStorage = s_storage[_currentNamespace];
        return currentNamespaceStorage.find(key) != currentNamespaceStorage.end();
    }

    static void resetStorage() {
        s_storage.clear();
        s_simulateBeginFailure = false;
        s_simulateWriteFailure = false;
    }

    static void setSimulateBeginFailure(bool simulateFailure) {
        s_simulateBeginFailure = simulateFailure;
    }

    static void setSimulateWriteFailure(bool simulateFailure) {
        s_simulateWriteFailure = simulateFailure;
    }

private:
    std::string _currentNamespace;
    bool _readOnly{false};
    bool _isOpen{false};

    inline static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> s_storage;
    inline static bool s_simulateBeginFailure{false};
    inline static bool s_simulateWriteFailure{false};
};

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include "domain/AudioPlayer.h"

#if defined(ARDUINO) || defined(ESP32)
#include <HardwareSerial.h>
#endif

namespace aviator::platform::audio {

/**
 * @brief Configuration parameters for JQ6500-16P MP3 sound module serial communication.
 */
struct Jq6500Configuration {
    uint8_t receivePin{16};
    uint8_t transmitPin{17};
    uint32_t baudRate{9600};
    uint8_t defaultVolume{25};
};

using Jq6500Config = Jq6500Configuration;

/**
 * @brief Platform adapter implementing domain::AudioPlayer for the JQ6500-16P MP3 module.
 * 
 * Communicates with the JQ6500-16P module over hardware UART (Serial2 on ESP32 by default)
 * using the standard JQ6500 command frame protocol:
 *   [0x7E, Length, Command, Parameters..., 0xEF]
 * 
 * Supports dependency injection of a custom data sender function for native host testing.
 */
class Jq6500AudioPlayer : public domain::AudioPlayer {
public:
    using SerialDataSender = std::function<size_t(const uint8_t* buffer, size_t length)>;

    static constexpr uint8_t MAXIMUM_VOLUME = 30;

    explicit Jq6500AudioPlayer(
        const Jq6500Configuration& configuration = Jq6500Configuration{},
        SerialDataSender customDataSender = nullptr);

#if defined(ARDUINO) || defined(ESP32)
    explicit Jq6500AudioPlayer(
        HardwareSerial& serial,
        const Jq6500Configuration& configuration = Jq6500Configuration{});
#endif

    ~Jq6500AudioPlayer() override = default;

    /**
     * @brief Initializes UART communication and sets the configured default volume.
     */
    void initialize();

    /**
     * @brief Plays the MP3 sound track corresponding to trackNumber (1-based index).
     * @param trackNumber 1-based index of the MP3 track to play.
     */
    void playTrack(uint16_t trackNumber) override;

    /**
     * @brief Stops currently playing audio.
     */
    void stop() override;

    /**
     * @brief Sets playback volume level.
     * @param volume Level ranging from 0 (mute) to 30 (maximum).
     */
    void setVolume(uint8_t volume) override;

    uint8_t getVolume() const;
    uint16_t getCurrentTrack() const;
    bool isPlaying() const;

private:
    void sendCommand(uint8_t command, const uint8_t* parameters = nullptr, uint8_t parameterLength = 0);

    Jq6500Configuration _configuration;
    SerialDataSender _customDataSender{nullptr};
#if defined(ARDUINO) || defined(ESP32)
    HardwareSerial* _serial{nullptr};
#endif
    uint8_t _currentVolume{25};
    uint16_t _currentTrack{0};
    bool _isPlaying{false};
    mutable std::mutex _transmissionMutex;
};

} // namespace aviator::platform::audio

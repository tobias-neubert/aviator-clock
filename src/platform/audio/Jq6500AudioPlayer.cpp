#include "platform/audio/Jq6500AudioPlayer.h"

#include <algorithm>
#include <array>

namespace aviator::platform::audio {

namespace {
constexpr uint8_t FRAME_HEADER = 0x7E;
constexpr uint8_t FRAME_TERMINATOR = 0xEF;
constexpr uint8_t COMMAND_PLAY_INDEX = 0x03;
constexpr uint8_t COMMAND_STOP = 0x16;
constexpr uint8_t COMMAND_SET_VOLUME = 0x31;
} // namespace

Jq6500AudioPlayer::Jq6500AudioPlayer(
    const Jq6500Configuration& configuration,
    SerialDataSender customDataSender)
    : _configuration(configuration),
      _customDataSender(std::move(customDataSender)),
      _currentVolume(std::min(configuration.defaultVolume, MAXIMUM_VOLUME)) {
}

#if defined(ARDUINO) || defined(ESP32)
Jq6500AudioPlayer::Jq6500AudioPlayer(
    HardwareSerial& serial,
    const Jq6500Configuration& configuration)
    : _configuration(configuration),
      _serial(&serial),
      _currentVolume(std::min(configuration.defaultVolume, MAXIMUM_VOLUME)) {
}
#endif

void Jq6500AudioPlayer::initialize() {
#if defined(ARDUINO) || defined(ESP32)
    if (_serial != nullptr && _customDataSender == nullptr) {
        _serial->begin(_configuration.baudRate, SERIAL_8N1, _configuration.receivePin, _configuration.transmitPin);
    }
#endif
    setVolume(_configuration.defaultVolume);
}

void Jq6500AudioPlayer::playTrack(uint16_t trackNumber) {
    if (trackNumber == 0) {
        stop();
        return;
    }

    const uint8_t parameters[2] = {
        static_cast<uint8_t>((trackNumber >> 8) & 0xFF),
        static_cast<uint8_t>(trackNumber & 0xFF)
    };

    sendCommand(COMMAND_PLAY_INDEX, parameters, 2);
    _currentTrack = trackNumber;
    _isPlaying = true;
}

void Jq6500AudioPlayer::stop() {
    sendCommand(COMMAND_STOP);
    _isPlaying = false;
}

void Jq6500AudioPlayer::setVolume(uint8_t volume) {
    _currentVolume = std::min(volume, MAXIMUM_VOLUME);
    const uint8_t parameters[1] = {_currentVolume};
    sendCommand(COMMAND_SET_VOLUME, parameters, 1);
}

uint8_t Jq6500AudioPlayer::getVolume() const {
    return _currentVolume;
}

uint16_t Jq6500AudioPlayer::getCurrentTrack() const {
    return _currentTrack;
}

bool Jq6500AudioPlayer::isPlaying() const {
    return _isPlaying;
}

void Jq6500AudioPlayer::sendCommand(uint8_t command, const uint8_t* parameters, uint8_t parameterLength) {
    std::lock_guard<std::mutex> lock(_transmissionMutex);

    // Protocol frame layout: [0x7E, Length, Command, Parameters..., 0xEF]
    // Length byte = Command (1 byte) + parameterLength + 1 = parameterLength + 2
    // Total frame size = parameterLength + 4
    std::array<uint8_t, 8> frame{};
    frame[0] = FRAME_HEADER;
    frame[1] = static_cast<uint8_t>(parameterLength + 2);
    frame[2] = command;

    for (uint8_t index = 0; index < parameterLength; ++index) {
        frame[3 + index] = parameters[index];
    }

    const size_t totalFrameSize = parameterLength + 4;
    frame[totalFrameSize - 1] = FRAME_TERMINATOR;

    if (_customDataSender != nullptr) {
        _customDataSender(frame.data(), totalFrameSize);
    }
#if defined(ARDUINO) || defined(ESP32)
    else if (_serial != nullptr) {
        _serial->write(frame.data(), totalFrameSize);
    }
#endif
}

} // namespace aviator::platform::audio

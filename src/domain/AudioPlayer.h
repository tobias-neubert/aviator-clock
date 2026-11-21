#pragma once

#include <cstdint>

namespace aviator::domain {

/**
 * @brief Abstract outbound port for controlling the sound module (JQ6500-16P).
 */
class AudioPlayer {
public:
    virtual ~AudioPlayer() = default;

    /**
     * @brief Plays the MP3 sound track corresponding to trackNumber.
     * @param trackNumber Index of the track to play (1-based index).
     */
    virtual void playTrack(uint16_t trackNumber) = 0;

    /**
     * @brief Stops currently playing audio.
     */
    virtual void stop() = 0;

    /**
     * @brief Sets playback volume level.
     * @param volume Level ranging from 0 (mute) to 30 (maximum).
     */
    virtual void setVolume(uint8_t volume) = 0;
};

} // namespace aviator::domain

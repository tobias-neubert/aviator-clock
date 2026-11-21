# 06 Implement Platform Web Portal, Audio Adapter (JQ6500-16P) & Web UI

Status: closed
Blocked by: 02

## Context
Per the Version 1.0 scope, configuration of both Wi-Fi and Alarms (including selecting an alarm tone stored on the JQ6500-16P MP3 module) is managed entirely through the web interface. We need a web server adapter serving static LittleFS assets and providing REST APIs, an audio adapter controlling the JQ6500-16P module via UART, and an updated Svelte web frontend.

## Requirements
1. Create `src/platform/audio/Jq6500AudioPlayer.h/.cpp` in namespace `aviator::platform::audio`:
   - Implements domain port `aviator::domain::AudioPlayer`.
   - Controls the JQ6500-16P MP3 module via hardware UART (e.g. `HardwareSerial(2)` / Serial2).
   - Functions: `playTrack(uint16_t trackNumber)`, `stop()`, `setVolume(uint8_t volume)`.
2. Create `src/platform/web/WebPortalServer.h/.cpp` in namespace `aviator::platform::web`:
   - Host `ESPAsyncWebServer` on port 80.
   - Serve pre-gzipped assets from LittleFS (`/index.html.gz`, `/assets/...`) with `Content-Encoding: gzip`.
   - REST API endpoints:
     - Wi-Fi: `GET /api/wifi/scan`, `POST /api/wifi/connect`, `GET /api/system/status`, `POST /api/system/reboot`.
     - Alarm: `GET /api/alarm` (fetch current alarm time, sound track, enabled state), `POST /api/alarm` (update alarm).
     - Audio: `GET /api/audio/tracks` (list available MP3 tracks on JQ6500), `POST /api/audio/test` (play preview of selected track).
3. Connect REST operations to domain port `ClockConfigurationApi` and `SystemGateway`.
4. Update `web-client/` (Svelte 4 SPA):
   - Add an Alarm Configuration card/section: set alarm time (HH:MM), enable/disable toggle, select MP3 sound from list with a "Test Play" button.
   - Build assets with `npm run build` so that compressed artifacts are updated in `data/`.

## Verification
- Verify that `web-client` assets build cleanly and gzip into `data/`.
- Verify compilation with `pio run -e nodemcu-32s`.

## Comments
- Implemented `Jq6500AudioPlayer` in `src/platform/audio/` implementing `aviator::domain::AudioPlayer` with JQ6500-16P serial protocol framing and unit test data sender injection.
- Implemented `ClockConfigurationService` in `src/domain/` implementing inbound domain port `ClockConfigurationApi`.
- Implemented `WebPortalServer` in `src/platform/web/` hosting `ESPAsyncWebServer` on port 80, serving static LittleFS assets and REST API endpoints.
- Updated `PinConfig.h` with UART audio pins for JQ6500-16P.
- Updated `web-client` Svelte 4 SPA with Alarm Configuration card, tone preview, and responsive styling; built compressed assets into `data/`.
- Added unit tests in `test_audio` and `test_domain` covering audio and configuration service logic (14 new unit tests, all passing).

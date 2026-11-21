# 06 Implement Platform Web Portal Adapter (LittleFS, AsyncWebServer)

Status: ready-for-agent
Blocked by: 02

## Context
Per ADR 0004, the web application (`web-client/`) builds static assets directly into `data/`. The firmware needs an HTTP web server adapter that serves these static assets and handles REST API endpoints without tangling web concerns into the domain core.

## Requirements
1. Create `src/platform/web/WebPortalServer.h/.cpp` in namespace `aviator::platform::web`.
2. Migrate relevant web server functionality from `lib/wifi-manager/`:
   - Host `ESPAsyncWebServer` on port 80.
   - Serve pre-gzipped assets from LittleFS (`/index.html.gz`, `/assets/...`) with `Content-Encoding: gzip`.
   - Implement REST endpoints (`GET /api/wifi/scan`, `POST /api/wifi/connect`, `GET /api/system/status`, `POST /api/system/reboot`).
3. Connect REST operations to domain ports:
   - Status, time, and alarm configuration queries call `ClockConfigurationApi`.
   - Device restart and credential wipe call `SystemGateway`.
4. Ensure all web request callbacks are non-blocking and memory-safe.

## Verification
- Verify that `web-client` assets in `data/` are correctly referenced.
- Verify compilation with `pio run -e nodemcu-32s`.

## Comments

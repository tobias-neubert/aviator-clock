# Web Client Isolation and Asset Distribution Pipeline

## Status
accepted

## Context
The Aviator Clock provides an interactive web-based configuration portal for scanning Wi-Fi networks and configuring device parameters. The frontend is implemented as a modern Single Page Application (Svelte 4, TypeScript, Vite). Mixing Node.js build assets and npm dependencies directly into C++ firmware source trees causes tooling conflicts, linter confusion, and build pipeline pollution. Conversely, serving uncompressed assets or bundling HTML as C++ raw string literals bloats firmware binaries and prevents rich UI development.

## Decision
We decouple the Web UI lifecycle into an external frontend pipeline and a firmware adapter:
1. **Frontend Isolation (`web-client/`):** The web application remains in its dedicated root directory `web-client/`. It maintains its own `package.json`, Vite configuration, and TypeScript tooling.
2. **Binary Asset Output (`data/`):** The Vite build outputs compressed, pre-gzipped static assets (`index.html.gz`, JS/CSS bundles) directly into the PlatformIO LittleFS asset directory (`data/`).
3. **Firmware Web Adapter (`aviator::platform::web`):** The C++ firmware hosts an HTTP web server adapter (such as `AsyncWebServer`) residing in `src/platform/web/`. It serves static LittleFS files on port 80 and translates REST API endpoints into domain port calls (`WifiProvisioning`, `ClockConfigurationApi`).
4. **Clean Decoupling:** The core domain logic remains completely unaware of HTTP, JSON, Svelte, or LittleFS.

## Consequences
- Clean separation of concerns between web tooling (npm/Vite) and embedded firmware tooling (PlatformIO/GCC).
- High performance and low RAM footprint on the ESP32 by serving pre-compressed `.gz` assets directly from flash via LittleFS.
- The web API surface is cleanly bound to hexagonal driving ports, making the device controllable through both web interfaces and physical controls identically.

# Konzept: Web UI (Svelte Single Page Application)

## 1. Einleitung
Dieses Dokument beschreibt die Architektur und den Umsetzungsplan für die webbasierte Konfigurationsoberfläche der Aviator Clock. Ziel ist es, die bisherige (oder rudimentäre) HTML-String-basierte Generierung durch eine moderne **Single Page Application (SPA)** zu ersetzen. Dies ermöglicht eine reaktive, benutzerfreundliche Oberfläche für WLAN-Setup, Statusanzeige und fortgeschrittene Gerätekonfiguration (z.B. Zeiger-Kalibrierung), ohne den Speicher des ESP32 mit komplexem Rendering-Code zu belasten.

Als Technologie für das Frontend wird **Svelte** gewählt, da es keinen Laufzeit-Overhead (Virtual DOM) hat und sehr kompakten JavaScript-Code erzeugt, was für Embedded-Systeme ideal ist.

## 2. Architektur

### 2.1 Übersicht
Das System folgt einer klaren Trennung zwischen Frontend (Browser) und Backend (ESP32).

*   **Frontend (Client):** Eine Svelte-App, die komplett im Browser läuft. Sie wird einmalig als statisches Asset (HTML, JS, CSS) vom ESP32 geladen.
*   **Backend (ESP32):** Fungiert primär als REST-API-Server. Er liefert keine dynamisch generierten HTML-Seiten mehr aus, sondern antwortet auf Anfragen mit JSON-Daten.
*   **Kommunikation:** Asynchron über HTTP (Fetch API).

### 2.2 Dateisystem & Hosting
*   Die kompilierten Frontend-Dateien (`index.html`, `bundle.js`, `bundle.css`) werden im Flash-Speicher des ESP32 (LittleFS) abgelegt.
*   Um Speicherplatz zu sparen und die Ladezeiten zu verkürzen, werden Textdateien (JS/CSS/HTML) **Gzip-komprimiert** (`.gz`) gespeichert.

### 2.3 Offline-Fähigkeit (AP Modus)
Da die Uhr während des initialen Setups einen Access Point (AP) aufspannt, hat der versundene Client (Smartphone/Laptop) **keine Internetverbindung**.
*   **Constraint:** Die App muss "Self-Contained" sein. Es dürfen keine externen Ressourcen (Google Fonts, CDNs für Bootstrap/Tailwind) geladen werden. Alle Assets müssen im Build-Artefakt enthalten sein.

## 3. Verzeichnisstruktur

Das Web-Projekt wird als Unterordner im Haupt-Repository geführt, aber logisch getrennt behandelt.

```text
aviator-clock/
├── platformio.ini
├── src/                    # C++ Firmware Quellcode
├── lib/                    # C++ Bibliotheken
├── data/                   # Zielordner für PlatformIO (LittleFS Image)
│   └── (Kompilierte Web-Assets: index.html.gz, assets/...)
└── web-client/             # NEU: Svelte Quellcode
    ├── package.json
    ├── vite.config.js
    ├── src/
    │   ├── main.ts
    │   ├── App.svelte
    │   ├── lib/            # Svelte Komponenten & Stores
    │   │   ├── api.ts      # API-Client & Mocks
    │   │   └── ...
    │   └── assets/
    └── public/
```

## 4. API-Spezifikation (Entwurf)

Die Kommunikation erfolgt über REST-ähnliche Endpoints.

### 4.1 WLAN Management
*   **`GET /api/wifi/scan`**
    *   Startet einen Scan (oder liefert gecachte Ergebnisse).
    *   Response: `[{"ssid": "HomeNet", "rssi": -60, "auth": "WPA2"}, ...]`
*   **`POST /api/wifi/connect`**
    *   Sendet Zugangsdaten.
    *   Body: `{"ssid": "HomeNet", "password": "secret"}`
    *   Response: `{"status": "connecting"}`
*   **`GET /api/wifi/status`**
    *   Prüft Verbindungsstatus.
    *   Response: `{"connected": true, "ip": "192.168.1.50"}`

### 4.2 System & Status
*   **`GET /api/system/info`**
    *   Allgemeine Infos (Uptime, Version, Heap).
*   **`POST /api/system/reboot`**
    *   Startet den ESP32 neu.

### 4.3 Design & UX ("Island Style")
Wir orientieren uns an einem modernen, reduzierten "Flat Design" ähnlich dem neuen IntelliJ UI.

#### Grundprinzipien
1.  **Island Architecture (Karten-Optik):**
    *   Konfigurationsgruppen (z.B. "WLAN Einstellungen", "Uhr kalibrieren") leben auf eigenen "Inseln" (Cards).
    *   Hintergrund der Seite ist leicht eingefärbt (z.B. hellgrau im Light Mode).
    *   Die Inseln sind weiß (im Light Mode) mit abgerundeten Ecken (`border-radius: 8px` oder `12px`) und sehr subtilem Schatten.

2.  **Farbpalette (Minimalistisch):**
    *   **Light Mode:**
        *   Background: `#F5F5F7` (Sehr helles Grau)
        *   Surface (Island): `#FFFFFF` (Reinweiß)
        *   Primary Action: `#007AFF` (Klassisches Blau, gut lesbar)
        *   Text: `#1C1C1E` (Fast Schwarz)
    *   **Dark Mode:**
        *   Automatische Erkennung via CSS `prefers-color-scheme`.
        *   Background: `#1E1E1E`
        *   Surface: `#2D2D2D`
        *   Text: `#E0E0E0`

3.  **WLAN-Liste (Konkretes Beispiel):**
    *   Jedes gefundene Netzwerk ist eine Zeile innerhalb der "WLAN-Insel".
    *   Klick auf eine Zeile expandiert diese (Accordion-Effekt) und zeigt das Passwort-Feld und den "Verbinden"-Button.
    *   Signalstärke wird durch ein simples Icon rechts angezeigt.

4.  **Technische Umsetzung:**
    *   Kein CSS-Framework (wie Bootstrap), zu viel Overhead.
    *   Nutzung von CSS-Variablen (`:root { --bg-color: ... }`) für einfaches Umschalten zwischen Light/Dark Mode.
    *   System Fonts (`-apple-system`, `Roboto`, `Segoe UI`) nutzen -> spart Ladezeit und sieht auf jedem Gerät "nativ" aus.

#### Skizze (Wireframe)

```text
+-----------------------------------+
|  [Logo] Aviator Clock        ( )  |  <-- ( ) = Dark/Light Toggle
+-----------------------------------+
|                                   |
|  +-----------------------------+  |
|  |  WLAN Verbindung            |  |  <-- Island 1
|  |                             |  |
|  |  [WLAN A]          (Signal) |  |
|  |  [WLAN B]          (Signal) |  |
|  |  > Passwort: [...........]  |  |  <-- Expandiert bei Klick
|  |    [ Verbinden ]            |  |
|  |                             |  |
|  |  [WLAN C]          (Signal) |  |
|  +-----------------------------+  |
|                                   |
|  +-----------------------------+  |
|  |  Status                     |  |  <-- Island 2
|  |                             |  |
|  |  IP: 192.168.178.45         |  |
|  |  Uptime: 00:12:43           |  |
|  +-----------------------------+  |
|                                   |
+-----------------------------------+
```

## 5. Entwicklungsworkflow

### 5.1 Frontend-Entwicklung (Host PC)
Entwickelt wird auf dem PC mit `npm run dev`. Da der ESP32 währenddessen evtl. nicht erreichbar ist, wird eine **Mocking-Strategie** benötigt.
*   In `src/lib/api.ts` wird erkannt, ob `import.meta.env.DEV` true ist.
*   Falls ja, werden statische JSON-Daten zurückgegeben (z.B. eine gefakte WLAN-Liste).
*   Dies ermöglicht UI-Entwicklung ohne Hardware.

### 5.2 Build & Integration
Ein `npm script` (z.B. `npm run build:firmware`) führt folgende Schritte aus:
1.  `vite build`: Kompiliert Svelte zu statischen Dateien in `web-client/dist`.
2.  Gzip-Komprimierung der Assets.
3.  Kopieren der Ergebnisse nach `../data/`.

### 5.3 Firmware-Deployment
1.  PlatformIO Build Filesystem Image (`pio run --target buildfs`).
2.  Upload Filesystem Image (`pio run --target uploadfs`).
3.  Upload Firmware (falls C++ Code geändert wurde).

## 6. Maßnahmenplan (Roadmap)

### Phase 1: Setup & Hello World
- [ ] Ordner `web-client` initialisieren (Vite + Svelte + TypeScript).
- [ ] Basiskonfiguration Vite (Proxy oder Mocking Vorbereitung).
- [ ] Hello World Page erstellen.
- [ ] Build-Skript erstellen, das Dateien nach `data/` kopiert.

### Phase 2: C++ Webserver Vorbereitung
- [ ] Auswahl einer Webserver-Library (empfohlen: `ESPAsyncWebServer` für asynchrone API-Calls).
- [ ] Einrichtung des Webservers in `Esp32WifiManager` (oder neuer Komponente).
- [ ] Route für Static Files (`/`) implementieren (aus LittleFS servieren).
- [ ] Gzip-Support aktivieren.

### Phase 3: WLAN Scan Feature (Full Stack)
- [ ] **Frontend:** UI-Komponente für WLAN-Liste und Passwort-Eingabe bauen.
- [ ] **Frontend:** API-Client mit Mock-Daten für Scan implementieren.
- [ ] **Backend:** Endpoint `/api/wifi/scan` implementieren (scannt asynchron Netzwerke).
- [ ] **Backend:** Endpoint `/api/wifi/connect` implementieren.
- [ ] Integrationstest: Frontend steuert echtes Backend.

### Phase 4: Styling & UX
- [ ] UI aufhübschen (CSS, evtl. kleines Framework wie Tailwind oder handgeschriebenes CSS für minimale Größe).
- [ ] Loading States, Fehlermeldungen visualisieren.

### Phase 5: Erweiterung
- [ ] Hinzufügen weiterer Konfigurationsseiten (Uhrzeit-Offset, Kalibrierung, NTP-Server wahl).

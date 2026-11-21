# Security Concept: Aviator Clock

This document outlines the security measures and principles to be incorporated into the Aviator Clock firmware and hardware design, particularly when operating on a local wireless network.

---

## 1. Secure Over-the-Air (OTA) Updates

Firmware update delivery is a critical attack vector and requires robust safeguards:

* **Firmware Signing:** All production firmware binaries must be cryptographically signed. The ESP32 bootloader must validate this digital signature prior to flashing an update. Only binaries signed with a trusted private key will execute, preventing arbitrary or malicious firmware execution.
* **Encrypted Transport (HTTPS/TLS):** Firmware images must be fetched strictly over TLS connections from an authentic server. Root certificates embedded in the binary validate server authenticity, protecting against man-in-the-middle (MitM) alterations during transit.

---

## 2. Network Security

Local network exposure must be kept to an absolute minimum:

* **Minimal Exposed Ports:** In Normal Running Mode, the clock must not expose open network ports or listening sockets. The embedded web server required for WiFi provisioning operates **exclusively** during active WiFi Setup Mode (triggered via physical pushbutton, with an automatic inactivity timeout). NTP time synchronization relies purely on outbound UDP requests.
* **Network Segmentation:** The Aviator Clock should be deployed within an isolated IoT or guest network segment, preventing lateral movement to sensitive network devices in the event of compromise.
* **Disabling Unused Services:** Telnet, unencrypted file transfer, and unauthenticated debug interfaces must be excluded from production release builds.

---

## 3. On-Device Sensitive Data Protection

Credentials stored locally on the microcontroller (e.g., WiFi passwords) must resist physical extraction:

* **Flash Encryption (ESP32 Flash Encryption):** Hardware-level AES encryption of external SPI flash prevents plaintext extraction of firmware code and stored credentials via physical memory dumps. Activation is irreversible via hardware eFuses and should be scheduled for production release.
* **Secure Boot (ESP32 Secure Boot):** Ensures a hardware-enforced chain of trust from ROM bootloader to application code, preventing unsigned bootloader replacements.
* **Non-Volatile Storage (NVS):** Sensitive credentials must reside in encrypted NVS partitions (`Preferences`), never hardcoded in source control.

---

## 4. Secure Development Practices

* **No Hardcoded Credentials:** Passwords, API tokens, and development credentials must never be committed to source control.
* **Strict Input Validation:** Form fields and REST endpoints (such as `/api/wifi/connect`) must enforce strict bounds checking and validation on SSID and passphrase payloads to eliminate buffer overruns and malformed payload exploits.
* **Dependency Auditing:** Upstream libraries (FreeRTOS, ESPAsyncWebServer, ArduinoJson) must be monitored and updated regularly against known CVEs.

---

## 5. Physical Device Security

* **Disabling Hardware Debug Interfaces:** For finalized production units, physical debugging interfaces (such as JTAG) can be permanently disabled via ESP32 eFuses to prevent unauthorized hardware-level inspection.

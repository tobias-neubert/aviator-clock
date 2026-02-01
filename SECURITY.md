# SECURITY.md - Sicherheitskonzept für die Aviator Clock

Dieses Dokument umreißt die wichtigsten Sicherheitsmaßnahmen und -prinzipien, die bei der Entwicklung der Aviator Clock berücksichtigt werden müssen, um das Gerät vor potenziellen Angriffen zu schützen, sobald es in einem WLAN-Netzwerk betrieben wird.

## 1. Sichere Over-the-Air (OTA) Updates

Der Prozess des Firmware-Updates ist ein kritisches Ziel für Angreifer. Hier müssen strenge Maßnahmen ergriffen werden:

*   **Firmware-Signierung:** Alle Firmware-Updates müssen kryptographisch signiert sein. Der Bootloader des ESP32 muss diese Signatur vor der Installation eines Updates validieren. Nur Firmware, die mit dem bekannten privaten Schlüssel signiert wurde, darf installiert werden. Dies verhindert, dass ein Angreifer manipulierte oder bösartige Firmware auf das Gerät flasht.
*   **Verschlüsselte Übertragung (HTTPS/TLS):** Firmware-Updates müssen immer über eine verschlüsselte Verbindung (z.B. HTTPS) von einem vertrauenswürdigen Server heruntergeladen werden. Dies schützt vor Man-in-the-Middle (MitM)-Angriffen, bei denen ein Angreifer die Update-Datei während der Übertragung manipulieren könnte. Die Uhr muss die Identität des Update-Servers anhand von Root-Zertifikaten validieren.

## 2. Netzwerksicherheit

Die Angriffsfläche vom lokalen Netzwerk aus muss minimiert werden.

*   **Minimale Offene Ports:** Im Normalbetrieb darf die Uhr keine unnötigen Netzwerkdienste oder offenen Ports betreiben. Der für das WiFi-Setup benötigte Webserver muss **ausschließlich** im aktiven Setup-Modus (nach Tasterbetätigung und vor Timeout) aktiv sein und wird im Normalbetrieb vollständig deaktiviert. NTP-Zeitsynchronisation erfordert lediglich ausgehende Verbindungen.
*   **Netzwerksegmentierung:** Für maximale Sicherheit sollte die Aviator Clock, wie auch andere IoT-Geräte, in einem isolierten Netzwerksegment (z.B. einem separaten VLAN oder einem Gäste-WLAN) betrieben werden. Dies stellt sicher, dass im Falle einer Kompromittierung der Uhr keine anderen sensiblen Geräte im Hauptnetzwerk des Benutzers erreicht werden können.
*   **Deaktivierung unnötiger Dienste:** Dienste wie Telnet, unverschlüsselte FTP-Server oder Debug-Schnittstellen (falls vorhanden) müssen in der Produktionsfirmware deaktiviert sein.

## 3. Schutz sensibler Daten auf dem Gerät

Daten, die auf dem Gerät gespeichert sind (z.B. WLAN-Zugangsdaten), müssen auch bei physischem Zugriff geschützt sein.

*   **Flash-Verschlüsselung (ESP32 Flash Encryption):** Der ESP32 bietet die Hardware-Funktion, den gesamten externen Flash-Speicher zu verschlüsseln. Dies verhindert, dass ein Angreifer durch direktes Auslesen des Flash-Chips an Klartext-Informationen wie WLAN-Zugangsdaten oder die Firmware selbst gelangt. Die Aktivierung ist ein irreversibler Prozess über eFuses des ESP32 und sollte sorgfältig geplant werden.
*   **Secure Boot (ESP32 Secure Boot):** Dies ist eine erweiterte Sicherheitsfunktion, die sicherstellt, dass der ESP32 nur Firmware ausführt, die mit einem bestimmten, bekannten Schlüssel signiert wurde. Es bildet eine Vertrauenskette von der Hardware bis zur Anwendungssoftware und verhindert, dass ein Angreifer eine bösartige Firmware über den Bootloader einschleust. Auch dies ist eine irreversible eFuse-Operation.
*   **Non-Volatile Storage (NVS):** Sensible Daten wie WLAN-Zugangsdaten müssen im NVS (Non-Volatile Storage) des ESP32 gespeichert werden, nicht als hartcodierte Werte im Quelltext.

## 4. Sichere Softwareentwicklungspraktiken

Allgemeine Praktiken zur Minimierung von Schwachstellen in der Codebasis.

*   **Keine hartcodierten Zugangsdaten:** Konfigurationsdaten, API-Schlüssel oder Passwörter dürfen niemals direkt im Quelltext hinterlegt werden. Sie müssen extern (z.B. über das Konfigurationsportal) bereitgestellt und sicher gespeichert werden (NVS).
*   **Eingabevalidierung:** Auch wenn die Benutzeroberfläche minimal ist, muss jede Art von Benutzereingabe (z.B. WLAN-Passwort im Konfigurationsportal) sorgfältig validiert werden, um Injektionsangriffe, Pufferüberläufe oder andere Parsing-Fehler zu verhindern.
*   **Aktuelle Abhängigkeiten:** Verwendete Bibliotheken und Frameworks (z.B. FreeRTOS, ESP-IDF-Komponenten, Arduino-Bibliotheken) müssen regelmäßig auf Sicherheitsupdates geprüft und auf dem neuesten Stand gehalten werden. Dies wird durch PlatformIO unterstützt.

## 5. Physische Sicherheit (Produktionsgeräte)

*   **Deaktivierung von Debugging-Schnittstellen:** In einem finalen Produktionsgerät sollten Debugging-Schnittstellen (z.B. JTAG) über eFuses deaktiviert werden, um zu verhindern, dass Angreifer diese Schnittstellen für unautorisierten Zugriff nutzen.

---

**Hinweis:** Dieses Dokument dient als Referenz und Leitfaden. Die Implementierung dieser Maßnahmen muss sorgfältig geplant und getestet werden.
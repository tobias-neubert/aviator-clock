# Konzept: WiFi-Setup über Konfigurationsportal

Dieses Dokument beschreibt das Konzept und den Umsetzungsplan für die WLAN-Einrichtung der "Aviator Clock".

## 1. Anforderungen

- **Automatischer Verbindungsaufbau:** Nach dem Einschalten versucht die Uhr, sich automatisch mit einem zuvor gespeicherten WLAN-Netzwerk zu verbinden.
- **Manueller Setup-Modus:** Ein dedizierter Taster ermöglicht das Starten eines Konfigurationsmodus.
- **Konfigurationsportal:** Im Setup-Modus startet die Uhr einen eigenen Access Point (AP) und stellt eine Webseite bereit, die auf einem externen Gerät (z.B. Smartphone) aufgerufen werden kann.
- **Netzwerkauswahl:** Die Webseite listet alle in Reichweite befindlichen WLAN-Netzwerke (SSIDs) auf.
- **Passworteingabe:** Der Benutzer kann ein Netzwerk auswählen und das zugehörige Passwort eingeben.
- **Persistente Speicherung:** Die Uhr speichert die SSID und das Passwort dauerhaft, sodass sie Neustarts überstehen.
- **Automatischer Timeout:** Der Setup-Modus wird nach 3 Minuten Inaktivität automatisch beendet, um den normalen Betrieb wieder aufzunehmen und Strom zu sparen.
- **Feedback:** Der Benutzer erhält über die Webseite eine Rückmeldung, ob die Verbindung erfolgreich war.

## 2. Konzept

Der Betrieb der Uhr wird in zwei Modi unterteilt: den **Normalbetrieb** und den **Setup-Modus**.

### 2.1. Normalbetrieb

Dies ist der Standardmodus nach dem Einschalten.

1.  Die Uhr lädt die gespeicherten WLAN-Zugangsdaten aus dem nicht-flüchtigen Speicher (NVS).
2.  Sind Zugangsdaten vorhanden, versucht sie, eine Verbindung zum gespeicherten WLAN herzustellen.
3.  **Erfolgreich:** Die Uhr verbindet sich und synchronisiert die Uhrzeit über einen NTP-Server. Der normale Betrieb wird fortgesetzt.
4.  **Fehlgeschlagen (oder keine Daten vorhanden):** Die Uhr kann keine Verbindung herstellen (z.B. falsches Passwort, Netz nicht in Reichweite). Sie läuft im "Offline-Modus" weiter, ohne die Zeit zu synchronisieren. Sie wird in regelmäßigen Abständen erneut versuchen, die Verbindung herzustellen.

### 2.2. Setup-Modus

Dieser Modus dient ausschließlich der Konfiguration des WLANs.

1.  **Aktivierung:** Der Benutzer drückt und hält den Setup-Taster für eine definierte Zeit (z.B. 3 Sekunden), um den Modus zu starten.
2.  **Access Point:** Die Uhr beendet alle bestehenden WLAN-Verbindungsversuche und startet einen eigenen WLAN Access Point (AP) mit einer festen SSID (z.B. "Aviator-Clock-Setup"). Das Netzwerk ist ungesichert.
3.  **Webserver:** Gleichzeitig startet ein kleiner Webserver, der auf eine feste IP-Adresse (z.B. 192.168.4.1) hört.
4.  **Konfigurations-Webseite:**
    -   Wenn ein Benutzer sich mit dem AP verbindet und die IP-Adresse im Browser aufruft, wird eine Webseite ausgeliefert.
    -   Die Seite scannt im Hintergrund nach verfügbaren WLAN-Netzen und stellt diese in einer Liste (Dropdown oder Radio-Buttons) dar.
    -   Ein Textfeld ermöglicht die Eingabe des Passworts für das ausgewählte Netz.
    -   Ein "Verbinden"-Button schickt die Formulardaten (SSID und Passwort) an den Webserver auf der Uhr.
5.  **Verarbeitung & Speicherung:**
    -   Der Server empfängt die Daten.
    -   Er speichert die Zugangsdaten im nicht-flüchtigen Speicher.
    -   Anschließend beendet er den AP-Modus und versucht, sich mit den neuen Daten mit dem Ziel-WLAN zu verbinden.
6.  **Feedback an den Benutzer:**
    -   **Erfolgreich:** Die Webseite meldet den Erfolg der Verbindung. Die Uhr startet anschließend neu oder wechselt direkt in den Normalbetrieb.
    -   **Fehlgeschlagen:** Die Webseite meldet den Fehler (z.B. "Verbindung fehlgeschlagen, Passwort falsch?"). Der Benutzer kann es erneut versuchen. Die Uhr bleibt im Setup-Modus.
7.  **Timeout:** Wenn 3 Minuten lang keine Interaktion über die Webseite erfolgt, beendet die Uhr den Setup-Modus automatisch und kehrt in den Normalbetrieb zurück (und versucht ggf. eine Verbindung mit alten, gültigen Daten).

## 3. Umsetzungsplan

Die Implementierung wird in logische Schritte unterteilt.

### Phase 1: Basis-AP und Webserver
- **Ziel:** Einen Access Point auf dem ESP32 starten und eine einfache HTML-Seite ausliefern.
- **Bibliotheken:** `WiFi.h`, `WebServer.h` (oder `AsyncTCP` und `ESPAsyncWebServer` für eine performantere, asynchrone Variante).
- **Schritte:**
    1.  Code erstellen, der den ESP32 in den `WIFI_AP` Modus versetzt.
    2.  Einen Webserver instanziieren, der auf Port 80 lauscht.
    3.  Eine Handler-Funktion für die Wurzel-URL ("/") erstellen, die eine simple HTML-Seite als String zurückgibt.

### Phase 2: WLAN-Scan und dynamische Webseite
- **Ziel:** Die Webseite soll verfügbare Netzwerke dynamisch anzeigen.
- **Schritte:**
    1.  Implementierung einer Funktion, die einen WLAN-Scan durchführt und eine Liste von SSIDs zurückgibt.
    2.  Anpassung der Handler-Funktion aus Phase 1, sodass sie vor dem Senden der HTML-Seite einen Scan durchführt und die HTML-Struktur dynamisch mit den gefundenen Netzwerknamen generiert.
    3.  Das HTML um ein Formular mit Dropdown-Menü für die SSIDs und ein Passwort-Eingabefeld erweitern.

### Phase 3: Verarbeitung und Speicherung der Zugangsdaten
- **Ziel:** Die vom Benutzer gesendeten Daten empfangen, speichern und verwenden.
- **Bibliotheken:** `Preferences.h` für die einfache Nutzung des NVS.
- **Schritte:**
    1.  Einen zweiten Handler für den Webserver erstellen, der die POST-Anfrage des Formulars verarbeitet (z.B. unter "/connect").
    2.  Die SSID und das Passwort aus der Anfrage extrahieren.
    3.  Die `Preferences`-Bibliothek nutzen, um beide Werte im NVS zu speichern.
    4.  Den AP-Modus beenden und den `WIFI_STA` Modus starten, um eine Verbindung mit den neuen Daten zu testen.
    5.  Eine einfache Feedback-Seite implementieren, die den Erfolg oder Misserfolg anzeigt.

### Phase 4: Integration in den Gesamt-Workflow
- **Ziel:** Den Setup-Modus mit dem Normalbetrieb und der Taster-Logik verbinden.
- **Schritte:**
    1.  Die Startlogik der Uhr implementieren: Beim Booten werden die `Preferences` geprüft. Sind Daten vorhanden, wird der Normalbetrieb (Verbindungsversuch) gestartet. Andernfalls könnte die Uhr direkt in den Setup-Modus gehen.
    2.  Eine Interrupt-Service-Routine (ISR) für den Setup-Taster implementieren. Diese setzt ein Flag, das im Haupt-Loop abgefragt wird, um den Setup-Modus zu starten.
    3.  Den 3-Minuten-Timeout für den Setup-Modus mit `millis()` oder einem Software-Timer umsetzen.

## 4. Zu klärende Punkte

- **Bibliothek für Webserver:** `ESPAsyncWebServer` ist robuster gegenüber mehreren Anfragen und blockiert den Loop nicht, hat aber mehr Abhängigkeiten (`AsyncTCP`). Für dieses einfache Szenario könnte der Standard-`WebServer` ausreichen.
- **Pin für Setup-Taster:** Welcher GPIO-Pin soll für den Taster verwendet werden?
- **Verhalten bei Verbindungsverlust:** Wie oft und in welchen Intervallen soll im Normalbetrieb eine Wiederverbindung versucht werden, wenn das WLAN verloren geht?


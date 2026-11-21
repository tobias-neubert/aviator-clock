# Konzept: Weckzeiten einstellen

## Status
**Entwurf**

## 1. Einführung
Dieses Dokument beschreibt das Bedienkonzept zum Einstellen der Weckzeiten für die Aviator Clock.

## 2. Hardware-Schnittstellen

### 2.1 Wahlschalter für Alarmspeicher
*   **Komponenten**: 3 silberne Kippschalter.
*   **Funktion**: Adressierung der verschiedenen Alarmspeicher. Die 3 Schalter bilden zusammen ein 3-Bit-Muster, womit **8 verschiedene Speicherplätze (Slots 0-7)** adressiert werden können. Im Folgenden wird angenommen, dass die Schalterstellung einen spezifischen Speicherplatz ("Slot") auswählt.

### 2.2 Eingabegerät
*   **Komponente**: Dual Rotary Encoder (siehe `docs/hardware/dual-rotary-encoder.pdf`).
*   **Funktion**:
    *   **Drehung (Ring/Knopf)**: Einstellung von Stunden und Minuten.
    *   **Drucktaster**: Bestätigung und Moduswechsel.

## 3. Workflow

### 3.1 Einstieg in den Setup-Modus
1.  Der Nutzer drückt den Encoder für **3 Sekunden**.
2.  Die Uhr wechselt vom "Normalen Betriebsmodus" (Uhrzeitanzeige) in den "Alarm Setup Modus".
3.  **Visuelles Feedback**:
    *   Die Zeiger fahren schnell auf die im aktuell gewählten Alarmspeicher hinterlegte Zeit.
    *   Der Sekundenzeiger fährt auf die 0-Position (bzw. siehe Punkt 4 zur 24h-Anzeige).

### 3.2 Alarmzeit einstellen
1.  Der Nutzer wählt über die **Kippschalter** den gewünschten Alarmspeicher aus.
    *   Die Zeiger aktualisieren sich sofort und zeigen die in diesem Slot gespeicherte Zeit an.
2.  Der Nutzer verstellt über den **Dual Rotary Encoder** die Zeit.
    *   Ein Ring/Knopf steuert die Stunden.
    *   Der andere Ring/Knopf steuert die Minuten.

### 3.3 Speichern und Beenden
1.  Der Nutzer drückt den Encoder erneut für **3 Sekunden**.
2.  Die eingestellte Zeit wird in den nicht-flüchtigen Speicher (NVS) für den gewählten Slot geschrieben.
3.  Die Uhr kehrt in den "Normalen Betriebsmodus" zurück und zeigt die aktuelle Uhrzeit an.

## 4. Offene Punkte & Lösungen

### 4.1 Die 24-Stunden-Problematik (AM/PM)
Da es sich um eine analoge 12-Stunden-Uhr handelt, ist der Unterschied zwischen 08:00 (Morgens) und 20:00 (Abends) nicht direkt ablesbar.

**Vorschlag zur Lösung:**
Nutzung des Sekundenzeigers als AM/PM-Indikator während des Einstellens.

*   Der Sekundenzeiger verharrt nicht statisch auf 0.
*   **Position 12 Uhr (0s)**: Zeigt **AM** (00:00 - 11:59) an.
*   **Position 6 Uhr (30s)**: Zeigt **PM** (12:00 - 23:59) an.

Beim Drehen des Stundenzeigers über die 12 hinaus springt der Sekundenzeiger automatisch von oben (0s) nach unten (30s) und umgekehrt. So ist eindeutig, ob 8 Uhr oder 20 Uhr eingestellt ist.
# Implementierungsplan: Weckzeiten Einstellen (Alarm Setup)

## 1. Übersicht
Dieser Plan beschreibt die Schritte zur Umsetzung der "Alarm Setup"-Funktionalität basierend auf der Spezifikation `KONZEPT_ALARM_SETUP.md`. Ziel ist ein iteratives Vorgehen, das Tests auf dem Host (Unit Tests) und auf dem Breadboard (Integrationstests) ermöglicht.

## 2. Hardware-Abstraktionsschicht (SOC) erweitern

Bevor die Anwendungslogik geschrieben werden kann, müssen die notwendigen Schnittstellen zur Hardware definiert werden.

### 2.1 Persistenz (NVS)
Um die Weckzeiten dauerhaft zu speichern, benötigen wir Zugriff auf den nicht-flüchtigen Speicher.
*   **Interface**: `lib/soc-api/include/soc/api/IPersistentStorage.h`
    *   Methoden: `putUInt(key, value)`, `getUInt(key, default)`, `putString(...)`, etc.
*   **Implementierung (ESP32)**: `lib/soc-esp32/.../ESP32PersistentStorage.cpp` (Wrapper um `Preferences.h`).
*   **Mock**: `lib/soc-mock/.../MockPersistentStorage.h` (In-Memory Map für Tests).

### 2.2 Rotary Encoder
Ein Treiber für den Dual-Encoder wird benötigt. Da wir die Button-Funktion separat implementieren (siehe unten), konzentriert sich dies auf die Drehbewegung.
*   **Interface**: `lib/soc-api/include/soc/api/IRotaryEncoder.h`
    *   Methoden: `getDelta()`, `reset()`.
*   **Implementierung (ESP32)**: `lib/soc-esp32/.../ESP32RotaryEncoder.cpp` (Nutzung einer Standard-Bibliothek wie `RotaryEncoder` oder eigene ISR-basiert).
*   **Mock**: `lib/soc-mock/.../MockRotaryEncoder.h`.

### 2.3 Input-Gruppierung (Kippschalter)
Die 3 Kippschalter bilden logisch eine Einheit.
*   **Komponente**: `lib/buttons/include/buttons/impl/SwitchGroup.h` (oder ähnlich).
*   **Logik**: Nimmt 3 `IDigitalInput` und liefert einen `uint8_t` (0-7) zurück.
*   *Breadboard-Hinweis*: Simulation durch Jumper-Kabel (VCC/GND) an 3 GPIOs.

## 3. Anwendungslogik & Domäne

Hier wird die eigentliche Intelligenz implementiert, unabhängig von der Hardware.

### 3.1 Alarm-Repository
Verwaltet die Alarmdaten.
*   **Klasse**: `AviatorClock::AlarmRepository`
*   **Verantwortung**: Speichert/Lädt Stunden und Minuten für 8 Slots aus dem `IPersistentStorage`.

### 3.2 Time Editor Logik
Kapselt die Regeln für das Verstellen der Zeit (z.B. Überlauf von 59 auf 0 Minuten, AM/PM Logik).
*   **Klasse**: `AviatorClock::TimeEditor`
*   **Input**: Delta Steps vom Encoder.
*   **Output**: Aktualisierte `Time`-Struktur (Stunde, Minute, Sekunde für AM/PM Indikation).

### 3.3 State Machine (Betriebsmodus)
Die Uhr braucht Zustände.
*   **Zustände**: `NormalRunning`, `AlarmSetup`.
*   **Übergang**: Durch `LongPressButton` (3s).

## 4. Integration in `AviatorClock`

Zusammenfügen der Teile in der "Composition Root".

### 4.1 Wiring (Setup)
*   Initialisierung des `LongPressButton` mit dem Breadboard-Taster.
*   Initialisierung der `ESP32RotaryEncoder` Instanzen (Inner/Outer).
*   Initialisierung der `SwitchGroup` (für Breadboard ggf. Mock oder feste Pins).

### 4.2 Main Loop Logik
*   Check Button -> Toggle State Machine.
*   **If Setup Mode**:
    *   Lese Slot-ID von Schaltern.
    *   Lade Alarmzeit aus Repo (wenn Slot gewechselt).
    *   Lese Encoder Deltas -> Update `TimeEditor`.
    *   Schreibe Zielpositionen auf `ClockHand` (Stunde, Minute).
    *   Setze `ClockHand` (Sekunde) auf 0 (AM) oder 30 (PM).
    *   Bei Exit -> Speichere Zeit in Repo.
*   **If Normal Mode**:
    *   Laufende Zeit anzeigen (bestehende Logik).

## 5. Breadboard-Constraint Management

Da das Test-Setup limitiert ist, passen wir die Konfiguration an:

*   **1 Motor vorhanden**: Wir mappen diesen Motor auf den **Minutenzeiger**, da hier das direkteste Feedback beim Drehen des Encoders erfolgt.
*   **Stunden/Sekunden**: Werden im Code voll simuliert (Instanzen existieren), aber ihre physikalischen Pins sind entweder "Dummy" oder nicht verbunden. Der Serial Log bestätigt deren Bewegung.
*   **Separater Button**: Wird an einen freien GPIO angeschlossen und als `IDigitalInput` für den `LongPressButton` konfiguriert.

## Nächste Schritte
1.  **Schritt 1**: Interfaces (`IPersistentStorage`, `IRotaryEncoder`) definieren.
2.  **Schritt 2**: Implementierung der `SwitchGroup` Logik (Unit Test first).
3.  **Schritt 3**: `AlarmRepository` & `TimeEditor` Logik implementieren (Unit Test first).
4.  **Schritt 4**: Integration in `AviatorClock` und Test auf Breadboard.

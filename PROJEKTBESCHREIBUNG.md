# Projektbeschreibung: Aviator Clock

Dieses Dokument dient als zentraler Kontext für den Gemini-Assistenten, um die Ziele, die Architektur und die Vision des Projekts zu verstehen.

**Assistenten-Anweisung:** Lies diese Datei zu Beginn jeder Interaktion, um den vollständigen Projektkontext zu laden.

## 1. Vision

Das Ziel ist der Bau einer physischen Uhr, die von klassischen analogen Fluginstrumenten inspiriert ist. Die Uhr wird über drei separate Zeiger für Stunden, Minuten und Sekunden verfügen. Jeder dieser Zeiger ist wie ein eigenes Instrument gestaltet und wird von einem dedizierten Schrittmotor angetrieben.

## 2. Architektur-Ziele

*   **Modularität:** Eine saubere, schichtenbasierte Architektur, die die Anwendungslogik von der Hardware entkoppelt.
*   **Testbarkeit:** Die Anwendungslogik soll so weit wie möglich auf einem Host-PC (z.B. mittels GoogleTest) testbar sein, ohne dass die physische Hardware (ESP32) benötigt wird. Dies wird durch eine Hardware-Abstraktionsschicht erreicht.
*   **Echtzeitfähigkeit:** Das System soll auf einem Echtzeitbetriebssystem (RTOS, konkret FreeRTOS auf dem ESP32) laufen, um die präzise und nebenläufige Steuerung der drei Motoren sowie anderer Dienste (z.B. NTP-Zeitsynchronisation) zu gewährleisten.

## 3. Kernkomponenten (Bibliotheken in `lib/`)

*   `soc-api`: Definiert die Schnittstellen zur Hardware-Abstraktion (System-on-a-Chip API). Hierzu gehören Dienste wie Logging, Zeitgeber, GPIOs und zukünftig auch RTOS-Primitive (Tasks, Queues).
*   `soc-esp32`: Die konkrete Implementierung der `soc-api` für die ESP32-Plattform unter Verwendung des Arduino/ESP-IDF Frameworks.
*   `soc-mock`: Eine Mock-Implementierung der `soc-api` für die Host-Testumgebung. Sie simuliert das Verhalten der Hardware, um Tests ohne realen Chip zu ermöglichen.
*   `stepper-api`: Definiert die Schnittstellen für die Ansteuerung von Schrittmotoren (`IStepperMotor`, `IHomingStrategy`, etc.).
*   `accel-stepper-adapter`: Eine Adapter-Bibliothek, die die `AccelStepper`-Drittanbieter-Bibliothek kapselt und sie mit der `stepper-api` kompatibel macht.
*   `aviator-clock`: Die eigentliche Anwendungslogik. Sie enthält die `ClockHand`-Klassen und die Logik, um die Zeit auf den Motorpositionen abzubilden.

## 4. Aktueller Status

Das Projekt befindet sich in der Migration von einer Single-Loop-Architektur zu einer RTOS-basierten Architektur. Der Migrationsprozess wird in `MIGRATIONSPLAN.md` dokumentiert.

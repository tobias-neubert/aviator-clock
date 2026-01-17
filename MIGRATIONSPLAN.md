# Migrationsplan: Umstellung auf RTOS

Dieses Dokument verfolgt die technischen Schritte, die zur Migration der Aviator Clock auf eine RTOS-basierte Architektur notwendig sind.

## Phase 1: Vorbereitung und Erweiterung der Abstraktion

- [x] **Schritt 1.1: Die `ISoc`-Schnittstelle einführen**
    *   Ziel: Alle plattformspezifischen Dienste (Logger, Time, GPIO) in einer einzigen Fassaden-Schnittstelle (`ISoc`) bündeln, um die Komplexität in `main.cpp` zu reduzieren.

*   [x] **Schritt 1.2: RTOS-Abstraktionen in `soc-api` erstellen**
    *   Ziel: Schnittstellen für RTOS-Primitive definieren (`ITask`), um die Testbarkeit der nebenläufigen Logik zu gewährleisten. `ISoc` wird um eine Factory-Methode `createTask` erweitert.

## Phase 2: Implementierung und Refactoring

*   [x] **Schritt 2.1: RTOS-Implementierung für ESP32**
    *   Ziel: Eine `ESP32Soc`-Klasse erstellen, die `ISoc` implementiert und die FreeRTOS-API des ESP32 nutzt.

*   [x] **Schritt 2.2: Mock-Implementierung für Host-Tests**
    *   Ziel: Eine `MockSoc`-Klasse für die Testumgebung erstellen, die das Verhalten von Tasks für Host-basierte Tests simuliert.

- [x] **Schritt 2.3: `main.cpp` auf RTOS umstellen**
    *   Ziel: Die `setup()`-Funktion initialisiert nur noch das `Soc`-Objekt und startet die Tasks. Die `loop()`-Funktion wird geleert.

*   [x] **Schritt 2.4: `ClockHand` in eine Task umwandeln**
    *   Ziel: Die Logik von `ClockHand` in eine eigene, in sich geschlossene Task-Klasse überführen, die in einer Endlosschleife mit Pausen läuft.

## Phase 3: Testen

- [x] **Schritt 3.1: Bestehende Tests anpassen**
    *   Ziel: Die Google-Tests anpassen, sodass sie die neue `MockSoc` und die Task-basierte Ausführung testen.

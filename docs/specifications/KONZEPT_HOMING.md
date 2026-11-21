# Spezifikation der Homing-Strategie

Dieses Dokument beschreibt die offizielle Strategie und Methode zum Finden der Nullposition (Homing) für die Schrittmotoren der Zeiger.

## 1. Definition des Koordinatensystems

Um Mehrdeutigkeiten zu vermeiden, wird das folgende Koordinatensystem für alle Zeigerbewegungen festgelegt:

- **Blickrichtung:** Die Beschreibung gilt bei frontaler Betrachtung des Ziffernblattes.
- **Nullpunkt (0°):** Die Position, die durch den Homing-Sensor definiert wird. Idealerweise wird der Sensor so montiert, dass dies der 12-Uhr-Position entspricht.
- **Drehrichtung:**
    - **Positive Winkel (+)** entsprechen einer Drehung im **Uhrzeigersinn (CW)**.
    - **Negative Winkel (-)** entsprechen einer Drehung **gegen den Uhrzeigersinn (CCW)**.

## 2. Gewählte Homing-Strategie

Als Homing-Methode wird ein **magnetischer Sensor (Hall-Sensor)** verwendet. Die implementierte Strategie (`PreciseOneWayHomingStrategy`) führt eine präzise, einstufige Referenzfahrt durch.

## 3. Ablauf der Logik

### 3.1. Referenzfahrt (Homing)

Die Referenzfahrt wird bei jedem Systemstart ausgeführt, um eine verlässliche Nullposition zu finden.

1.  **Prüfung bei Start:** Die Strategie prüft zuerst, ob der Hall-Sensor bereits beim Start aktiv ist.
    - **Falls ja (Start auf dem Sensor):** Der Motor bewegt den Zeiger zuerst ein kurzes, definiertes Stück vom Sensor weg (gegen die Suchrichtung), bis der Sensor inaktiv wird.
2.  **Start der Suchfahrt:** Der Zeiger beginnt, sich langsam in einer vordefinierten Richtung (im Uhrzeigersinn) zu drehen.
3.  **Sensor-Erkennung:** Die Bewegung wird fortgesetzt, bis der Hall-Sensor durch den Magneten aktiviert wird.
4.  **Stopp-Befehl:** Sobald der Sensor aktiviert wird, sendet die Steuerung einen `stop()`-Befehl. Der Motor bremst daraufhin kontrolliert bis zum Stillstand ab. Er hält nicht instantan an, sondern hat einen kurzen, physikalisch bedingten Bremsweg (Overshoot).
5.  **Nullpunkt setzen:** Die finale Position, an der der Motor nach dem Abbremsen zum Stehen kommt, wird als logischer Nullpunkt (`0`) im Koordinatensystem des Motors gesetzt.

### 3.2. Anfahren von Zielpositionen

Die Logik zum Anfahren von Zielpositionen nach dem Homing ist in der `ClockHand`-Klasse implementiert und nicht Teil der Homing-Strategie selbst.

## 4. Hardware-Implementierung im Detail

Dieses Kapitel dient als Referenz für die Auswahl und den Einbau der Hardware-Komponenten, basierend auf den Erkenntnissen aus der Praxis.

### 4.1. Hall-Effekt-Sensor

Die Auswahl des richtigen Sensortyps ist entscheidend.
- **Typ:** Ein **digitaler, unipolarer Hall-Effekt-Sensor (Switch)**, idealerweise auf einem Modul verbaut (z.B. "MH-Sensor Series").
    - **Digital:** Liefert ein klares An/Aus-Signal (HIGH/LOW).
    - **Unipolar:** Reagiert nur auf einen Magnetpol, was für einen präzisen Schaltpunkt sorgt.
- **Betriebsspannung:** Die stabilste Lösung wurde mit einem **5V-Modul** erzielt. 3.3V-Sensoren sind oft nicht spezifikationsgemäß an einem 3.3V-ESP32 zu betreiben und können zu Fehlern führen.
- **Beispiel-Bauteil:** Ein Modul mit einem `A3144`-Sensor oder Ähnlichem.

### 4.2. Magnet

- **Typ:** Ein kleiner **Neodym-Zylindermagnet (NdFeB)**.
- **Magnetisierung:** Die Magnetisierung kann **axial** (Pole an den flachen Enden) oder **radial** (Pole an den Seiten) sein. Es ist entscheidend, durch Tests die korrekte Ausrichtung für den Sensor zu finden.
- **Größe:** Kleine Magnete (z.B. 1-3mm) erzeugen ein scharfes Feld für hohe Präzision.

### 4.3. Mechanische Montage

- **Prinzip:** Eine mit der Motorachse rotierende Scheibe trägt den Magneten. Der Hall-Sensor ist feststehend am Gehäuse montiert.
- **Abstand:** Der Luftspalt zwischen Magnet und Sensor muss **so gering wie nur möglich** sein (weniger als 1-2 mm). Jeder Bruchteil eines Millimeters zählt.
- **Ausrichtung:** Die Position und Ausrichtung von Magnet und Sensor zueinander müssen experimentell ermittelt werden, um eine zuverlässige Schaltung zu gewährleisten.

### 4.4. Elektrische Beschaltung (Spannungsteiler für 5V-Sensor)

Da wir einen 5V-Sensor an einem 3.3V-Mikrocontroller (ESP32) betreiben, ist ein **Spannungsteiler am `DO` (Digital Out) Pin des Sensormoduls zwingend erforderlich**, um den GPIO-Pin des ESP32 nicht zu beschädigen.

**Funktionsweise:**
Das 5V-Ausgangssignal des Sensors wird auf ein für den ESP32 sicheres 3.3V-Niveau reduziert.
1.  **Sensor inaktiv:** Der Sensor-Ausgang ist HIGH (5V). Der Spannungsteiler reduziert dies auf ca. 2.5V - 3.3V. Der ESP32 liest ein klares **HIGH**.
2.  **Sensor aktiv:** Der Sensor-Ausgang ist LOW (0V). Der Spannungsteiler gibt die 0V direkt weiter. Der ESP32 liest ein klares **LOW**.

**Schaltungs-Schema (mit zwei 10kΩ-Widerständen):**
```
    Sensor-Modul                        ESP32
   +------------+
   |        VCC |----------------------> 5V
   |        GND |----------------------> GND
   |            |
   |         DO |---+------------------> GPIO Pin (z.B. 25)
   +------------+   |
                    |
                   | |
                   | | 10 kΩ (Pull-down zum Spannungsteiler)
                   | |
                    |
                   GND
```
**Wichtiger Hinweis:** Das obige Schema ist vereinfacht. Ein Pull-up-Widerstand (ebenfalls 10kΩ) von DO nach VCC (5V) ist oft bereits auf dem Sensormodul vorhanden. Wenn nicht, muss er hinzugefügt werden, um den Spannungsteiler zu vervollständigen. Die Kombination aus 10kΩ Pull-up und 10kΩ Pull-down halbiert die Spannung von 5V auf 2.5V.

## 5. Evaluierte Alternativen

Andere Methoden wurden in Betracht gezogen, aber aus folgenden Gründen verworfen:

### 5.1. Mechanischer Endschalter
- **Bewertung:** Funktional, aber erfordert mechanische Präzision und unterliegt Verschleiß.

### 5.2. Optischer Sensor (Gabellichtschranke)
- **Bewertung:** Exzellente Präzision, aber empfindlicher gegenüber Staub.

### 5.3. Sensorless Homing (StallGuard™)
- **Bewertung:** Für 360°-Anwendungen ungeeignet und für eine präzise Zeigerdarstellung zu ungenau.

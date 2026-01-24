# Spezifikation der Homing-Strategie

Dieses Dokument beschreibt die offizielle Strategie und Methode zum Finden der Nullposition (Homing) für die Schrittmotoren der Zeiger.

## 1. Definition des Koordinatensystems

Um Mehrdeutigkeiten zu vermeiden, wird das folgende Koordinatensystem für alle Zeigerbewegungen festgelegt:

- **Blickrichtung:** Die Beschreibung gilt bei frontaler Betrachtung des Ziffernblattes.
- **Nullpunkt (0°):** Die Position, bei der der Zeiger exakt senkrecht nach unten zeigt (entspricht der 6-Uhr-Position einer normalen Uhr).
- **Drehrichtung:**
    - **Positive Winkel (+)** entsprechen einer Drehung im **Uhrzeigersinn (CW)**.
    - **Negative Winkel (-)** entsprechen einer Drehung **gegen den Uhrzeigersinn (CCW)**.
- **Beispiel:** Eine Position von `+10°` bedeutet, der Zeiger hat sich von der senkrechten unteren Position um 10 Grad im Uhrzeigersinn gedreht (nach **rechts**, aus Sicht des Betrachters).

## 2. Gewählte Homing- und Bewegungsstrategie

Als Homing-Methode wird ein **magnetischer Sensor (Hall-Sensor)** verwendet. Die übergeordnete Bewegungsstrategie für alle Zeiger ist die **ausschließliche Drehung im Uhrzeigersinn (CW)**. Dies vereinfacht die Ansteuerung maximal und eliminiert jeglichen Einfluss von Getriebespiel (Backlash).

## 3. Ablauf der Logik

### 3.1. Referenzfahrt (Homing)

Die Referenzfahrt wird bei jedem Systemstart ausgeführt und besteht aus einer einzigen, kontinuierlichen Bewegung, die alle Startbedingungen abdeckt.

1.  **Start der Bewegung:** Der Zeiger beginnt, sich langsam im Uhrzeigersinn (CW) zu drehen.
2.  **Warten auf Sensor:** Die Bewegung wird fortgesetzt, bis der Hall-Sensor durch den Magneten aktiviert wird.
    - *Anmerkung: Dieser Ansatz funktioniert für alle Start-Szenarien. Falls der Sensor beim Start bereits aktiv ist, dreht der Zeiger einfach eine volle 360°-Umdrehung, bis er den Sensor erneut erreicht.*
3.  **Home-Position erkennen:** Sobald der Sensor aktiviert wird, wird der Motor **sofort gestoppt**.
4.  **Nullpunkt setzen:** Die interne Schritt-Zählung des Motors wird an diesem Punkt auf `0` gesetzt. Diese Position gilt ab sofort als der absolute Referenz-Nullpunkt.

### 3.2. Anfahren von Zielpositionen

Alle Bewegungen zu einer neuen Zielposition erfolgen **ausschließlich im Uhrzeigersinn (CW)**.

- **Logik:** Um von einer `aktuellenPosition` zu einer `zielPosition` zu gelangen, wird immer nur vorwärts gefahren.
- **Beispiel:** Soll der Zeiger von 90° auf 45° bewegt werden, dreht er nicht 45° zurück, sondern 315° vorwärts (über 180°, 270°, 0° hinweg bis zur 45°-Marke).
- **Vorteil:** Die Ansteuerungslogik benötigt keine Fallunterscheidungen für die Richtung, was sie extrem einfach und robust macht.

## 4. Hardware-Implementierung im Detail

Dieses Kapitel dient als Referenz für die Auswahl und den Einbau der Hardware-Komponenten.

### 4.1. Hall-Effekt-Sensor

Die Auswahl des richtigen Sensortyps ist entscheidend.
- **Typ:** Es muss ein **digitaler, unipolarer Hall-Effekt-Sensor (Switch)** sein.
    - **Digital:** Liefert ein klares An/Aus-Signal (HIGH/LOW), das der ESP32 direkt verarbeiten kann. Analoge/lineare Sensoren sind hierfür ungeeignet.
    - **Unipolar:** Reagiert nur auf einen Magnetpol (z.B. Südpol). Dies sorgt für einen präzisen Schaltpunkt und macht den Aufbau unempfindlicher für magnetische Störfelder.
- **Betriebsspannung:** Der Sensor muss für die **3.3V** des ESP32 spezifiziert sein.
- **Beispiel-Bauteil:** Ein mögliches Bauteil ist der [Honeywell SS460S](https://www.reichelt.de/de/de/shop/produkt/hallsensor_unipolar_digital_to-92-151345).
    - **WICHTIGER HINWEIS:** Laut Datenblatt hat dieser Sensor eine Mindestbetriebsspannung von **3.8V**. Er ist daher **nicht ideal** für den direkten 3.3V-Betrieb mit dem ESP32 und seine Funktion ist nicht garantiert. Es wird empfohlen, einen Sensor zu wählen, dessen Betriebsspannungsbereich 3.3V explizit einschließt (z.B. die **Allegro A110x Serie**).

### 4.2. Magnet

- **Typ:** Ein kleiner **Neodym-Scheibenmagnet (NdFeB)** mit **axialer** Magnetisierung (Pole auf den flachen Seiten).
- **Größe:** Für eine hohe Präzision ist ein kleiner Magnet ideal, da er ein schärfer definiertes Magnetfeld erzeugt. Eine Größe von ca. **3mm Durchmesser und 1-2mm Höhe** ist empfehlenswert.
- **Stärke:** Eine Standard-Güte wie N35 oder N42 ist völlig ausreichend.

### 4.3. Mechanische Montage

Die Präzision des gesamten Systems hängt entscheidend von der sauberen mechanischen Montage ab.
- **Prinzip:** Eine mit der Motorachse rotierende Scheibe trägt den Magneten. Der Hall-Sensor ist feststehend am Gehäuse montiert.
- **Abstand:** Der Luftspalt zwischen Magnet und Sensor sollte so gering wie möglich sein (ca. 1-2 mm), ohne dass es zu einer Kollision kommen kann.
- **Ausrichtung für höchste Präzision:** Die beste Schärfe des Signals wird erreicht, wenn der Magnet **seitlich an der flachen Vorderseite des Sensors vorbeigeführt** wird. Eine frontale Annäherung an den Sensor sollte vermieden werden. Diese "streifende" Bewegung erzeugt die schnellste Feldänderung und damit den wiederholgenauesten Schaltimpuls.

### 4.4. Elektrische Beschaltung (Pull-up-Widerstand)

Die meisten Hall-Sensoren (auch die empfohlenen) besitzen einen **"Open Collector"-Ausgang**. Das bedeutet, der Sensor kann den Ausgangspin aktiv nur mit Ground (LOW) verbinden. Er kann ihn aber nicht aktiv auf 3.3V (HIGH) setzen. Lässt man den Pin unbeschaltet, "schwebt" er in einem undefinierten Zustand, wenn der Sensor inaktiv ist.

Um dies zu verhindern, wird ein **Pull-up-Widerstand** benötigt. Er sorgt für einen definierten HIGH-Zustand, wenn der Sensor inaktiv ist.

**Funktionsweise:**
1.  **Magnet ist weit weg:** Der Sensor ist inaktiv. Der Pull-up-Widerstand zieht die Spannung am Pin sanft auf 3.3V. Der ESP32 liest ein klares **HIGH**.
2.  **Magnet ist nah:** Der Sensor wird aktiv und schaltet eine starke Verbindung nach Ground. Diese Verbindung ist stärker als der schwache Pull-up-Widerstand und zieht die Spannung auf 0V. Der ESP32 liest ein klares **LOW**.

**Schaltungs-Schema:**
```
      +3.3V
        |
       | |
       | | 10 kΩ (Pull-up)
       | |
        |
        +------> GPIO Pin am ESP32
        |
    +-------+
    | Sensor|-- (Ausgang)
    +-------+
        |
       GND
```

## 5. Evaluierte Alternativen

Andere Methoden wurden in Betracht gezogen, aber aus folgenden Gründen verworfen:

### 5.1. Mechanischer Endschalter

- **Prinzip:** Ein Nocken auf der Motorachse betätigt einmal pro Umdrehung einen Mikroschalter.
- **Bewertung:** Diese Methode ist funktional, erfordert aber die präzise Fertigung und Montage eines Nockens. Zudem unterliegt sie mechanischem Verschleiß. Die gewählte berührungslose magnetische Lösung ist hier langfristig überlegen.

### 5.2. Optischer Sensor (Gabellichtschranke)

- **Prinzip:** Eine Lochscheibe auf der Achse unterbricht einmal pro Umdrehung den Strahl einer Lichtschranke.
- **Bewertung:** Eine exzellente, hochpräzise Alternative zur magnetischen Lösung. Die finale Entscheidung für den Hall-Sensor fiel aufgrund seiner höheren Robustheit gegenüber Staub und potenziell einfacherer Montage im Gehäuse.

### 5.3. Sensorless Homing (StallGuard™)

- **Prinzip:** Erkennung des Motorstillstands an einem harten Anschlag über die TMC2209-Treiber.
- **Bewertung:** Ist für eine 360°-Anwendung nicht geeignet und die erreichte Genauigkeit und Wiederholbarkeit wurde generell als **unzureichend** für eine präzise Zeigerdarstellung bewertet.

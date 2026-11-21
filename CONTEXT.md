# Aviator Clock

A physical desktop clock inspired by analog aircraft instrument dials, featuring three independent hands driven by dedicated stepper motors.

## Language

### Dials and Hands

**Clock Face**:
The front instrument panel holding three distinct analog dials.
_Avoid_: Display, dashboard, screen

**Dial**:
A circular scale on the clock face dedicated to a single time measurement (hours, minutes, or seconds).
_Avoid_: Gauge, faceplate, sub-dial

**Clock Hand**:
A rotating mechanical pointer that sweeps across a dial to indicate a value.
_Avoid_: Pointer, needle, arm

**Hour Hand**:
The clock hand that indicates the hour of the day.
_Avoid_: Short hand, hour pointer

**Minute Hand**:
The clock hand that indicates minutes within the current hour.
_Avoid_: Long hand, minute pointer

**Second Hand**:
The clock hand that advances each second during normal timekeeping.
_Avoid_: Tick hand, second pointer

**Active Angle**:
The total angular arc of a dial scale traversed by a clock hand (for instance, 330 degrees).
_Avoid_: Sweep angle, dial range, angular span

**Zero Point**:
The calibrated reference position on a dial corresponding to the zero/top mark.
_Avoid_: Home position, origin, null point

**Homing**:
The physical self-alignment procedure executed by a hand to find its zero point using a magnetic sensor.
_Avoid_: Zeroing, referencing, calibration run

### Operating Modes

**Operating Mode**:
A primary behavioral state governing how the clock interacts with time, hardware inputs, and network services.
_Avoid_: State machine, system status, execution phase

**Normal Running Mode**:
The operating mode where hands advance continuously or step-by-step according to real-time clock data.
_Avoid_: Clock mode, live mode, run mode

**Setup Mode**:
The operating mode where the clock hosts a wireless network to configure Wi-Fi credentials and alarm settings through a web browser.
_Avoid_: AP mode, provisioning mode, portal mode

### Alarm & Audio

**Alarm**:
A scheduled alert configured via the web interface specifying an hour, minute, and sound track.
_Avoid_: Reminder, alert clock

**Sound Module**:
The dedicated JQ6500-16P MP3 audio module used to store and play alarm sound tracks.
_Avoid_: Buzzer, speaker board, audio player

**Alarm Sound**:
An MP3 audio track stored on the sound module played when an alarm is triggered.
_Avoid_: Ringtone, beep, audio file

### Connectivity & Web

**Configuration Portal**:
The embedded web interface served by the device for scanning networks, submitting credentials, and configuring alarms.
_Avoid_: Setup website, admin interface, portal app

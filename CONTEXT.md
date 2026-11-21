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
The clock hand that advances each second during normal timekeeping and indicates AM or PM during alarm setup.
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

**Alarm Setup Mode**:
The interactive operating mode for inspecting and altering saved alarm times using manual inputs.
_Avoid_: Edit mode, alarm config, setting mode

**WiFi Setup Mode**:
The operating mode where the clock hosts a wireless network to receive network credentials through a browser.
_Avoid_: AP mode, provisioning mode, portal mode

### Alarm Configuration

**Alarm Slot**:
One of eight persistent memory locations (indexed 0 to 7) storing an hour and minute alarm timestamp.
_Avoid_: Memory bank, alarm channel, preset

**Slot Selector**:
A 3-bit binary toggle switch group that selects the active alarm slot.
_Avoid_: DIP switch, address switch, binary selector

**Dual Rotary Encoder**:
A dual-axis concentric rotary control with an integrated push button used for setting hours and minutes and toggling modes.
_Avoid_: Knob, rotary dial, pulse wheel

**Time Editor**:
The logical unit responsible for adjusting time values incrementally with 12/24-hour arithmetic and overflow wrapping.
_Avoid_: Adjuster, time counter, time calculator

**AM/PM Indicator**:
The visual state displayed by the second hand during alarm adjustment (top for AM, bottom for PM).
_Avoid_: Day/night flag, meridian indicator

### Connectivity & Web

**Configuration Portal**:
The embedded web interface served by the device for scanning networks and submitting credentials.
_Avoid_: Setup website, admin interface, portal app

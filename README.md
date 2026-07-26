# MCP23017 Keypad demo (Orange Pi Zero 2W)

This C++ demo scans a membrane matrix keypad connected to either MCP23017 GPIO
port. Port B is selected by default; use `--port A` to select port A at
runtime. Because the same connector is used for both ports, port B pin
numbering is mirrored automatically. The keyboard layout and wiring are
selected at build time:

- **VID 14-key keyboard** (default)
- **Legacy 4x4 keypad**

Only one layout is included in each executable.

## GPIO port mapping

Keyboard layouts use logical pins P0 through P7. Port A maps them directly,
while port B reverses them for the mirrored connector:

| Logical pin | Port A | Port B |
|---|---|---|
| P0 | PA0 | PB7 |
| P1 | PA1 | PB6 |
| P2 | PA2 | PB5 |
| P3 | PA3 | PB4 |
| P4 | PA4 | PB3 |
| P5 | PA5 | PB2 |
| P6 | PA6 | PB1 |
| P7 | PA7 | PB0 |

## VID 14-key keyboard

The eight Crimpflex contacts are numbered left-to-right when the keyboard is
viewed from the front, as in the manufacturer's drawing.

| Contact | Matrix net | Selected port | Direction |
|---:|---|---|---|
| 1 | C1 | P0 | Input with pull-up |
| 2 | R4 | P1 | Output |
| 3 | C2 | P2 | Input with pull-up |
| 4 | R3 | P3 | Output |
| 5 | C3 | P4 | Input with pull-up |
| 6 | R2 | P5 | Output |
| 7 | C4 | P6 | Input with pull-up |
| 8 | R1 | P7 | Output |

The VID matrix is:

| | C1 | C2 | C3 | C4 |
|---|---|---|---|---|
| R1 | `1` | `2` | `3` | `START/ENTER` |
| R2 | `4` | `5` | `6` | `STOP/CANCEL` |
| R3 | `7` | `8` | `9` | Unused |
| R4 | `RUS/ENG` | `0` | `BACKSPACE` | Unused |

## Legacy 4x4 keypad

The legacy wiring remains unchanged:

- P0..P3: row outputs R1..R4 on the selected port
- P4..P7: column inputs C1..C4 with internal pull-ups
- Ribbon pins 1..4: R1..R4
- Ribbon pins 5..8: C1..C4

| | C1/P4 | C2/P5 | C3/P6 | C4/P7 |
|---|---|---|---|---|
| R1/P0 | `1` | `2` | `3` | `A` |
| R2/P1 | `4` | `5` | `6` | `B` |
| R3/P2 | `7` | `8` | `9` | `C` |
| R4/P3 | `*` | `0` | `#` | `D` |

### I²C

- MCP23017 VCC -> 3.3V
- MCP23017 GND -> GND
- MCP23017 SDA -> Orange Pi SDA (I2C bus you enable)
- MCP23017 SCL -> Orange Pi SCL

**Address**
Default MCP23017 address is **0x20** when A0=A1=A2=0.

## Software prerequisites

On Armbian/Debian/Ubuntu:
```bash
sudo apt update
sudo apt install -y g++ cmake make i2c-tools
```

Enable I2C in Armbian (one of):
- `sudo armbian-config` -> System -> Hardware -> enable i2c
- or edit `/boot/armbianEnv.txt` to enable the correct overlay for your board

Verify the device is visible:

```bash
sudo i2cdetect -y 3
```

You should see `20` at the expected address.

## Build

The VID keyboard is the default:

```bash
cmake -S . -B build
cmake --build build -j
```

Build the legacy keypad variant with:

```bash
cmake -S . -B build-legacy -DKEYPAD_LAYOUT_VID=OFF
cmake --build build-legacy -j
```

The long-press threshold defaults to 1000 ms. Set
`KEYPAD_LONG_PRESS_MS` while configuring to choose another positive
millisecond value:

```bash
cmake -S . -B build -DKEYPAD_LONG_PRESS_MS=1500
cmake --build build -j
```

Use separate build directories for the two variants so it is always clear
which keyboard layout an executable contains. CMake also prints the selected
layout and long-press threshold while configuring.

## Run

The default I²C device is `/dev/i2c-3`, the default address is `0x20`, and
GPIO port B is selected by default:

```bash
sudo ./build/kbd
```

Options:
```bash
sudo ./build/kbd --dev /dev/i2c-3 --addr 0x20 --port A --poll-ms 5
```

Short presses are reported after the key is released. A long press is reported
once as soon as the key has remained down for at least the configured
threshold; releasing it does not produce an additional short-press event.
Press and release debounce are sample-based: transient empty reads do not end
a press, and changing to a different key starts a new timing cycle.

Example output:

```
Pressed: 5 (short)
Pressed: START/ENTER (long)
```

The startup banner includes the selected layout name: `VID 14-key` or
`legacy 4x4`, the selected port and pin-map direction, and the compiled
long-press threshold.

## Notes / troubleshooting

- If you get `Permission denied` opening `/dev/i2c-*`, run with `sudo` or add your user to the `i2c` group.
- If keys are incorrect, check the runtime port, selected CMake layout, and ribbon orientation.
- The scanner reports one key at a time; it does not implement multi-key rollover or ghosting prevention.
- Port A uses direct numbering; port B automatically applies the mirrored `0↔7`, `1↔6`, `2↔5`, `3↔4` mapping.

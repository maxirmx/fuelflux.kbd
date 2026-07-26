# MCP23017 Keypad demo (Orange Pi Zero 2W)

This C++ demo scans a membrane matrix keypad connected to MCP23017 port A.
The keyboard layout and wiring are selected at build time:

- **VID 14-key keyboard** (default)
- **Legacy 4x4 keypad**

Only one layout is included in each executable.

## VID 14-key keyboard

The eight Crimpflex contacts are numbered left-to-right when the keyboard is
viewed from the front, as in the manufacturer's drawing.

| Contact | Matrix net | MCP23017 | Direction |
|---:|---|---|---|
| 1 | C1 | PA0 | Input with pull-up |
| 2 | R4 | PA1 | Output |
| 3 | C2 | PA2 | Input with pull-up |
| 4 | R3 | PA3 | Output |
| 5 | C3 | PA4 | Input with pull-up |
| 6 | R2 | PA5 | Output |
| 7 | C4 | PA6 | Input with pull-up |
| 8 | R1 | PA7 | Output |

The VID matrix is:

| | C1 | C2 | C3 | C4 |
|---|---|---|---|---|
| R1 | `1` | `2` | `3` | `START/ENTER` |
| R2 | `4` | `5` | `6` | `STOP/CANCEL` |
| R3 | `7` | `8` | `9` | Unused |
| R4 | `RUS/ENG` | `0` | `BACKSPACE` | Unused |

## Legacy 4x4 keypad

The legacy wiring remains unchanged:

- PA0..PA3: row outputs R1..R4
- PA4..PA7: column inputs C1..C4 with internal pull-ups
- Ribbon pins 1..4: R1..R4
- Ribbon pins 5..8: C1..C4

| | C1/PA4 | C2/PA5 | C3/PA6 | C4/PA7 |
|---|---|---|---|---|
| R1/PA0 | `1` | `2` | `3` | `A` |
| R2/PA1 | `4` | `5` | `6` | `B` |
| R3/PA2 | `7` | `8` | `9` | `C` |
| R4/PA3 | `*` | `0` | `#` | `D` |

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

Use separate build directories for the two variants so it is always clear
which keyboard layout an executable contains. CMake also prints the selected
layout while configuring.

## Run

The default I²C device is `/dev/i2c-3` and the default address is `0x20`:

```bash
sudo ./build/kbd
```

Options:
```bash
sudo ./build/kbd --dev /dev/i2c-3 --addr 0x20 --poll-ms 5
```

It prints key presses like:

```
Pressed: 5
Pressed: START/ENTER
```

The startup banner includes the selected layout name: `VID 14-key` or
`legacy 4x4`.

## Notes / troubleshooting

- If you get `Permission denied` opening `/dev/i2c-*`, run with `sudo` or add your user to the `i2c` group.
- If keys are mirrored or incorrect, check both the selected CMake layout and the ribbon orientation.
- The scanner reports one key at a time; it does not implement multi-key rollover or ghosting prevention.
- Both layouts use only MCP23017 port A (PA0..PA7).

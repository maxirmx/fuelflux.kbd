# MCP23017 4x4 Keypad demo (Orange Pi Zero 2W)

This bundle contains a small **C++** demo that scans a **4x4 membrane matrix keypad**
connected to an **MCP23017** I²C GPIO expander.

## Wiring (using MCP23017 port A only: PA0..PA7)

We use:
- **PA0..PA3** = ROW outputs (R1..R4)
- **PA4..PA7** = COL inputs  (C1..C4) with internal pull-ups enabled in MCP23017

Typical keypad ribbon pinout:
- Pin1..Pin4  = R1..R4
- Pin5..Pin8  = C1..C4

So connect:
- R1 -> PA0
- R2 -> PA1
- R3 -> PA2
- R4 -> PA3
- C1 -> PA4
- C2 -> PA5
- C3 -> PA6
- C4 -> PA7

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

```bash
mkdir -p build
cmake -S . -B build
cmake --build build -j
```

## Run

Default is `/dev/i2c-3` and address `0x20`:

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
Pressed: A
```

## Notes / troubleshooting

- If you get `Permission denied` opening `/dev/i2c-*`, run with `sudo` or add your user to the `i2c` group.
- If keys are "mirrored" or wrong, your keypad ribbon pin order may differ; swap row/column mapping accordingly in `main.cpp`.
- This demo uses only port A to match your requirement (PA0..PA7).

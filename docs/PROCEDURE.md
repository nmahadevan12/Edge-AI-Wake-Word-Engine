# Operating Procedure

Day-to-day build, flash, capture, and debug workflow.

---

## Quick loop

```text
Edit src/main.c
      │
      ▼
  make clean && make
      │
      ▼
  make flash   (or VS Code → Flash)
      │
      ▼
  serial_monitor.py   ──► mic_capture.csv + buffer_dump_NNN.csv
      │
      ▼
  serial_plot.py      ──► live view + latest dump
```

Always start the **monitor before the plotter**. The plotter reads files the monitor writes.

---

## Step-by-step

### 1. Build

```bash
make clean
make
```

Optional inspection:

```bash
arm-none-eabi-size build/firmware.elf
arm-none-eabi-objdump -d build/firmware.elf | less
arm-none-eabi-objdump -s -j .isr_vector build/firmware.elf
```

### 2. Flash

```bash
make flash
```

Wait for **Verified OK** / **Resetting Target**. The MCU reboots into `Reset_Handler` → `main()`.

### 3. Warm-up

Firmware ignores capture for the first **7000 ms** after boot (noise while the mic/DC path settles). Live UART frames still stream; the debounce flag stays 0.

### 4. Monitor

```bash
python serial_monitor.py
```

You should see lines like:

```text
.. .. .. ..  .. .. .. ..  .. .. .. ..  .. .. .. ..  sample=...  energy=...  loud=...  debounce=...  t=...
```

On a valid event:

```text
[DUMP] Valid buffer dump received
[DUMP] Saved 256 samples → buffer_dump_001.csv
```

### 5. Plotter

```bash
python serial_plot.py
```

| Control | Action |
|---------|--------|
| **Follow** | Stick to the live tail (green when active) |
| **Pause** | Freeze the top view; scrub with **View** |
| **View** slider | Scrub history while paused |

Bottom pane updates only for dumps created **after** the plotter was started (session-filtered).

### 6. Collect data

1. Flash a clean build.  
2. Restart monitor (clears or overwrites `mic_capture.csv` unless you pass `--append`).  
3. Say **“hey STM32”** clearly; wait for `[DUMP]`.  
4. Collect negatives: silence, taps, other words.  
5. Keep / rename dumps into `positive/` and `negative/` folders when you start training.

---

## Tuning parameters

Edit macros / constants in [`src/main.c`](../src/main.c), then rebuild and flash.

| Parameter | Typical location | Effect |
|-----------|------------------|--------|
| Energy threshold `500` | `Sound_Detect` | Higher → less sensitive |
| `DEBOUNCE_VAL` (30) | `#define` | More loud frames required to keep dump |
| Quiet abort `low_count > 250` | `Sound_Detect` | Abort weak events before buffer fills |
| `CAPACITY` (256) | `#define` | Fixed window length (1024 bytes of PCM) |
| EMA `alpha` (0.2) | `struct Update_Energy` | Higher → energy tracks |y| faster |
| DC `R` (0.995) | `struct DC_Blocker` | Closer to 1 → slower DC settle |

Use the plotter **quiet max** readout to set the energy threshold above ambient noise.

---

## Debugging (OpenOCD + VS Code)

Typical keys (Cortex-Debug / launch config):

| Key | Action |
|-----|--------|
| F5 | Start debug |
| F10 | Step over |
| F11 | Step into |
| Shift+F5 | Stop |

See [`.vscode/launch.json`](../.vscode/launch.json) if present.

---

## Common failures

| Symptom | Likely cause |
|---------|----------------|
| No serial data | Wrong port, wrong GND, or 8N1 instead of **8N2** |
| Garbage after ~7 s | Firmware not sending the 3rd UART field every sample (fixed in current `Sound_Detect`) |
| Plot “stuck” / nonsense energy | Monitor lost frame sync — restart monitor + plotter |
| No dumps on speech | Debounce not reached, or quiet abort firing; check plot loud vs debounce |
| Dump on every tap | Lower `DEBOUNCE_VAL` threshold effect — raise `DEBOUNCE_VAL` or energy gate |
| Flash fails | ST-Link cable, another OpenOCD still running, or wrong target cfg |

---

## Build pipeline (reference)

```text
src/main.c  +  startup_stm32l475xx.s
        │
        ▼
   arm-none-eabi-gcc  (-mcpu=cortex-m4 -mthumb)
        │
        ├── .c → build/main.o
        └── .s → build/startup_stm32l475xx.o
        │
        ▼
   link with stm32l475vg.ld
        │
        ▼
   build/firmware.elf
        │
        ▼
   OpenOCD → ST-Link → Flash → Reset
```

Related docs: [SETUP.md](SETUP.md) · [SIGNAL_PROCESSING.md](SIGNAL_PROCESSING.md) · [PROTOCOL.md](PROTOCOL.md)

# Edge AI Wake-Word Front End — STM32L475 (Bare Metal)

Register-level firmware and host tooling for an on-device wake-word pipeline on the **B-L475E-IOT01A** (`STM32L475VG`). No HAL/CMSIS wrappers — all peripherals are programmed via CMSIS-style register maps in [`src/main.c`](src/main.c).

**Goal:** capture clean, fixed-length audio windows from the onboard MEMS microphone for training and later on-device inference of a “hey STM32” wake word.

---

## Highlights

- **Bare-metal STM32L4** — RCC, GPIO, UART4, SysTick, and DFSDM configured at the register level
- **PDM → PCM audio path** — DFSDM channel 2 / filter 0 (Sinc3, FOSR 64) on PE7/PE9
- **Real-time signal processing** — DC-blocking IIR, EMA energy estimate, energy-gated capture with debounce and quiet-abort
- **Event-driven waveform dump** — 256-sample (`1024`-byte) PCM buffer transmitted over UART with framing markers
- **Host debug stack** — Python serial monitor + live dual-pane plotter for calibration and dataset collection

---

## Architecture

```text
Onboard MEMS mic (PDM)
        │
        ▼
   DFSDM (Sinc3)  ──►  PCM sample x
        │
        ▼
   DC_Blocker     ──►  y = x − x_prev + R·y_prev
        │
        ▼
   Update_Energy  ──►  energy = α·|y| + (1−α)·energy
        │
        ▼
   Sound_Detect
        ├─ start capture on first energy > 500
        ├─ store every PCM y for 256 samples
        ├─ loud_count  (energy > 500)
        ├─ low_count   (quiet streak; abort if weak event)
        └─ send dump iff loud_count ≥ DEBOUNCE_VAL
        │
        ▼
   UART4 @ 115200 8N2 (PA0 / Arduino D1)
        ├─ live 16-byte frames (sample, energy, flag, timer)
        └─ 1032-byte event dumps (start + 256×y + end)
```

---

## Capture logic

| Stage | Behavior |
|-------|----------|
| Trigger | First sample with `energy > 500` starts a capture window |
| Fill | Every subsequent PCM sample `y` is stored (loud and quiet) |
| Debounce | Count frames with `energy > 500`; keep only if count ≥ `DEBOUNCE_VAL` (30) |
| Quiet abort | If quiet streak `> 250` before debounce passes → discard and reset |
| Complete | At 256 samples: send dump if debounce passed, else discard |

This produces **fixed-length** training windows while rejecting short noise (e.g. taps that never sustain enough loud frames).

---

## UART protocol

### Live frame — 16 bytes / sample

| Offset | Type | Field |
|--------|------|-------|
| 0–3 | `int32_t` BE | DC-blocked PCM `y` |
| 4–7 | `uint32_t` BE | EMA energy |
| 8–11 | `uint32_t` BE | Debounce status (0/1) |
| 12–15 | `uint32_t` BE | SysTick timer (ms) |

### Event dump — 1032 bytes

| Offset | Content |
|--------|---------|
| 0–3 | `0xFFFFFFFF` start marker |
| 4–1027 | 256 × `int32_t` PCM samples |
| 1028–1031 | `0x00000000` end marker |

Wiring: USB-TTL **RX ← Arduino D1 (PA0)**, common **GND**. Serial: **115200, 8 data bits, 2 stop bits, no parity**.

---

## Skills demonstrated

| Area | Detail |
|------|--------|
| Embedded C | Bare-metal drivers, ISRs, linker script, startup assembly |
| Digital audio | PDM/DFSDM, DC removal, EMA energy, event segmentation |
| Protocols | Custom binary UART framing, host-side reassembly |
| Tooling | Make + OpenOCD flash flow; Python (`pyserial`, `matplotlib`) debug UI |
| Systems | End-to-end path from sensor → MCU → PC for ML data collection |

---

## Build & run

**Toolchain (macOS):** `brew install arm-none-eabi-gcc openocd`  
**Host deps:** `pip install pyserial matplotlib`

```bash
make clean && make
make flash          # or: openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
                    #      -c "program build/firmware.elf verify reset exit"

python serial_monitor.py          # CSV log + buffer dumps
python serial_plot.py             # live PCM / energy / flag + latest dump
```

VS Code task buttons: **Build**, **Flash**, **Monitor**, **Plotter**.

---

## Repository layout

| Path | Role |
|------|------|
| [`src/main.c`](src/main.c) | Firmware: DFSDM, DSP chain, capture, UART |
| [`startup_stm32l475xx.s`](startup_stm32l475xx.s) | Vector table, `.data`/`.bss`, `Reset_Handler` |
| [`stm32l475vg.ld`](stm32l475vg.ld) | Memory map |
| [`Makefile`](Makefile) | Build / flash |
| [`serial_monitor.py`](serial_monitor.py) | Frame parser → `mic_capture.csv` + `buffer_dump_NNN.csv` |
| [`serial_plot.py`](serial_plot.py) | Live dual-pane visualizer |

---

## Status & next steps

**Complete:** bare-metal audio front end, gated fixed-window capture, UART dataset export, host visualization.

**Next:** collect labeled “hey STM32” / negative dumps → feature extraction (e.g. MFCC) → train a compact classifier → quantize and deploy inference on the MCU.

---

## References

| Document | Use |
|----------|-----|
| [RM0351](https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) — STM32L47/L48 reference manual | RCC, GPIO, UART4, DFSDM |
| [STM32L475VG datasheet](https://www.st.com/en/microcontrollers-microprocessors/stm32l475vg.html) | Pin alternate functions |
| [UM2153](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html) — B-L475E-IOT01A | Board pinout, onboard mic |
| PM0214 — Cortex-M4 programming manual | SysTick, vector table |

# STM32L475 Discovery — bare-metal wake-word detector

Register-level firmware for the **B-L475E-IOT01A** (`STM32L475VG`). No HAL. Application code is [`src/main.c`](src/main.c).

Reads the onboard MEMS mic over DFSDM (PDM → PCM), runs a DC blocker + EMA energy tracker + debounced sound detector, captures a 256-sample waveform buffer on each noise event, and streams all data over **UART4** on Arduino **D1 (PA0)**.

---

## What `main.c` does

Bring-up, then a continuous loop:

1. **UART4 TX** on PA0 (AF8), 115200 baud, 8 data bits, **2 stop bits**
2. **SysTick** at 1 ms → global `timer++`
3. **GPIOE** PE9 = DFSDM1_CKOUT (~2 MHz), PE7 = DFSDM1_DATIN2 (mic data)
4. **DFSDM** channel 2 + filter 0 (Sinc3, oversampling 64), continuous regular conversions
5. Each sample: DC block → energy EMA → debounce flag → buffer capture
6. Send 16-byte live frame over UART every loop
7. When buffer fills (256 samples): send 1032-byte dump over UART

---

## Signal pipeline

```text
DFSDM (PDM → PCM)
  └─ DC_Blocker          removes DC offset (IIR: y = x - x_prev + R*y_prev, R=0.995)
       └─ Update_Energy   EMA of |y|: energy = 0.2*|y| + 0.8*energy
            └─ Sound_Detect  debounce: energy > 500 for 20+ samples → flag=1
                              quiet streak > 5000 → reset buffer
            └─ Sample_Buffer  write y to sample_buffer[] while flag=1
                              when full (256 samples) → Send_Sample_Buffer()
```

---

## UART wire formats

### Normal frame — 16 bytes, every loop

| Bytes | Type | Field |
|-------|------|-------|
| 0–3 | `int32_t` big-endian | DC-blocked sample `y` |
| 4–7 | `uint32_t` big-endian | EMA energy |
| 8–11 | `uint32_t` big-endian | debounce flag (0 or 1) |
| 12–15 | `uint32_t` big-endian | `timer` (ms since boot) |

### Buffer dump — 1032 bytes, on event

Sent when `sample_buffer[255]` is filled:

| Bytes | Content |
|-------|---------|
| 0–3 | `0xFFFFFFFF` start marker |
| 4–1027 | 256 × `int32_t` — DC-blocked `y` samples |
| 1028–1031 | `0x00000000` end marker |

Talk into the **onboard MEMS mic**. USB-TTL: **RX ← D1 (PA0)**, common GND. Serial: **115200 8N2**.

---

## `main.c` function map

| Function | Role |
|----------|------|
| `UART4_GPIO_Init` / `UART_Setup` | PA0 AF8, UART4 clock, MSI 16 MHz, baud 139 |
| `UART_Transmit` / `UART_Transmit_Ptr` | Wait TXE, send 1 byte / 4 bytes |
| `Convert_Int_To_Bytes` | `int32_t` → 4 big-endian bytes → UART |
| `Convert_Uint_To_Bytes` | `uint32_t` → 4 big-endian bytes → UART |
| `Mic_Setup` | GPIOE clock, PE7/PE9 AF6 |
| `DFSDM_Setup` | DFSDM1 clock, CKOUT divider, ch2, Sinc3 + FOSR=64, start conversion |
| `Get_Mic_Sample` | Poll `REOCF`, read `FLT0RDATAR`, call `DC_Blocker` |
| `DC_Blocker` | IIR high-pass, sends `y`, calls `Update_Energy` + `Sample_Buffer` |
| `Update_Energy` | EMA energy, sends energy, calls `Sound_Detect` |
| `Sound_Detect` | Debounce counter + quiet-streak reset, sends flag |
| `Sample_Buffer` | Writes `y` to global `sample_buffer[]` while flag=1 |
| `Send_Sample_Buffer` | Sends start marker + 256 samples + end marker |
| `SysTick_Init` / `SysTick_Handler` | 1 ms tick, `timer++` |
| `main` | Init all, then loop: `Get_Mic_Sample` → `Convert_Uint_To_Bytes(timer)` → delay |

---

## Global state

| Variable | Type | Purpose |
|----------|------|---------|
| `timer` | `uint32_t` | ms since boot (SysTick ISR) |
| `flag` | `uint8_t` | 1 when debounce active |
| `sample_buffer[256]` | `int32_t[]` | circular capture window |
| `index` | `uint8_t` | write head for `sample_buffer` |
| `dc` | `struct DC_Blocker` | R=0.995, x_prev, y_prev |
| `energy` | `struct Update_Energy` | alpha=0.2, running EMA |

---

## VS Code buttons

Status-bar buttons (extension **Task Buttons**, `spencerwmiles.vscode-task-buttons`): **Build**, **Flash**, **Monitor**, **Plotter**, **Git**. Git prompts for a commit message, then `git add .`, `commit`, and `push`. Start Monitor before Plotter.

---

## Terminal setup and run

You need: this repo, an ST-Link cable to the Discovery kit, and a USB-TTL adapter on Arduino D1.

### 1. Compiler and OpenOCD

**macOS (Homebrew):**

```bash
xcode-select --install
brew install arm-none-eabi-gcc openocd python3
```

**Linux (Debian/Ubuntu):**

```bash
sudo apt update
sudo apt install -y build-essential gcc-arm-none-eabi binutils-arm-none-eabi \
  openocd python3 python3-pip
```

### 2. Python host tools

```bash
cd /path/to/stm
python3 -m pip install pyserial matplotlib
```

### 3. Wire UART

| USB-TTL | Board |
|---------|--------|
| RX | Arduino **D1** (PA0, UART4_TX) |
| GND | GND |

Serial settings: **115200, 8 data bits, 2 stop bits, no parity**.

Find the adapter:

```bash
ls /dev/tty.usbserial* /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

### 4. Build

```bash
make clean && make
```

### 5. Flash

```bash
openocd \
  -f interface/stlink.cfg \
  -f target/stm32l4x.cfg \
  -c "program build/firmware.elf verify reset exit"
```

### 6. Serial monitor

Prints hex frames and writes `mic_capture.csv`. On buffer dump, saves `buffer_dump_NNN.csv`.

```bash
python3 serial_monitor.py
python3 serial_monitor.py /dev/tty.usbserial-XXXX   # custom port
```

### 7. Plotter (second terminal)

Two-pane live view. Top pane: live sample / energy / flag / quiet-max. Bottom pane: most recent buffer dump waveform, auto-updates on each event.

```bash
python3 serial_plot.py
```

Controls: **Pause** freezes live scroll. **View** slider scrubs history. **Follow** returns to live.

### 8. Tuning thresholds

| Parameter | Location | Effect |
|-----------|----------|--------|
| Energy threshold (`> 500`) | `Sound_Detect` | Raise to reject more noise, lower for more sensitivity |
| Debounce count (`>= 20`) | `Sound_Detect` | Higher = longer sound needed to trigger flag |
| Quiet streak (`> 5000`) | `Sound_Detect` | Higher = longer silence allowed mid-phrase |

Use the plotter's **quiet max** text box (top-right, updates after t > 20 s) to calibrate the energy threshold above ambient noise.

---

## Repo layout

| Path | Role |
|------|------|
| [`src/main.c`](src/main.c) | Firmware: mic, DC blocker, energy, buffer, UART |
| [`startup_stm32l475xx.s`](startup_stm32l475xx.s) | Vector table, `.data` / `.bss`, `Reset_Handler` |
| [`stm32l475vg.ld`](stm32l475vg.ld) | FLASH / RAM linker script |
| [`Makefile`](Makefile) | Build + `make flash` |
| [`serial_monitor.py`](serial_monitor.py) | 16-byte frame parser + buffer dump handler → CSV |
| [`serial_plot.py`](serial_plot.py) | Live waveform + buffer dump viewer |
| [`test_code/`](test_code/) | Older experiments (not built by Makefile) |

---

## References

| Document | ID | Used for | Link |
|----------|----|----------|------|
| STM32L47/L48 reference manual | **RM0351** | RCC, GPIO, UART4, DFSDM registers | [PDF](https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| STM32L475xx datasheet | **DS11585** | Pin AF table: PA0 AF8, PE7/PE9 AF6 | [STM32L475VG](https://www.st.com/en/microcontrollers-microprocessors/stm32l475vg.html) |
| B-L475E-IOT01A user manual | **UM2153** | Arduino D1 = PA0, onboard mic location | [B-L475E-IOT01A](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html) |
| Cortex-M4 programming manual | **PM0214** | SysTick registers, vector table | Search ST "PM0214" |

# STM32L475 Discovery — bare-metal PDM mic

Register-level firmware for the **B-L475E-IOT01A** (`STM32L475VG`). No HAL. Application code is [`src/main.c`](src/main.c).

It clocks the onboard MEMS mic (PDM), converts that bit stream with DFSDM, and sends each sample plus a millisecond timestamp over **UART4** on Arduino **D1 (PA0)**.

---

## What `main.c` does

Bring-up, then a loop:

1. **UART4 TX** on PA0 (AF8), 115200 baud, 8 data bits, **2 stop bits**
2. **SysTick** at 1 ms → global `timer++`
3. **GPIOE** PE9 = DFSDM1_CKOUT (~2 MHz), PE7 = DFSDM1_DATIN2 (mic data)
4. **DFSDM** channel 2 + filter 0 (Sinc3, oversampling 64), continuous regular conversions
5. Wait for a sample (`REOCF`), send a **32-bit unsigned** sample, then **32-bit unsigned `timer`**, then a busy-wait delay

### Wire format

Firmware stores the sample as `uint32_t` and sends 4 big-endian bytes. `Convert_To_Bytes` is unsigned 32-bit as well.

```c
uint32_t mic_data = FLT0RDATAR >> 8;
Convert_To_Bytes(mic_data);   /* 4 bytes, unsigned */
Convert_To_Bytes(timer);      /* 4 bytes, unsigned */
```

```text
[ b3 b2 b1 b0 | t3 t2 t1 t0 ]
  uint32_t        uint32_t ms
  (RDATAR >> 8)   since boot
```

DFSDM still places a 24-bit PCM value in `FLT0RDATAR[31:8]`. After an **unsigned** `>> 8`, a negative sample often appears as `00 ff xx xx` on the wire (high byte stays 0). [`serial_monitor.py`](serial_monitor.py) sign-extends bit 23 only for the CSV/plot so the graph looks like audio. The UART stream itself is unsigned 32-bit.

Talk into the **onboard MEMS mic**, not the ST-Link USB end. USB-TTL: **RX ← D1 (PA0)**, common GND. Host serial: **115200 8N2**. ST-Link virtual COM is a different USART (PB6/PB7).

### `main.c` map

| Function | Role |
|----------|------|
| `UART4_GPIO_Init` / `UART_Setup` | PA0 AF8, UART4 clock, MSI 16 MHz, baud 139 |
| `UART_Transmit` / `Convert_To_Bytes` | Wait TXE, send 4-byte big-endian `uint32_t` |
| `Mic_Setup` | GPIOE clock, PE7/PE9 AF6 |
| `DFSDM_Setup` | DFSDM1 clock, CKOUT divider, ch2, Sinc3 + FOSR, start conversion |
| `Get_Mic_Sample` | Poll `REOCF`, read `FLT0RDATAR`, UART the `uint32_t` sample |
| `SysTick_Init` / `SysTick_Handler` | 1 ms tick, `timer++` |
| `main` | Init, then sample + timer TX forever |

---

## VS Code buttons

Status-bar buttons (extension **Task Buttons**, `spencerwmiles.vscode-task-buttons`): **Build**, **Flash**, **Monitor**, **Plotter**, **Git**. Git prompts for a commit message, then `git add .`, `commit`, and `push`. Start Monitor before Plotter.

Same tasks: **Terminal → Run Task**. The rest of this README is the terminal-only path.

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

Check:

```bash
arm-none-eabi-gcc --version
openocd --version
python3 --version
```

Plug in the board over ST-Link USB. `lsusb` (Linux) or **System Information → USB** (macOS) should show an ST-Link.

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

Do **not** use the ST-Link `usbmodem` port for this firmware. Serial settings: **115200, 8 data bits, 2 stop bits, no parity**.

Find the adapter:

```bash
ls /dev/tty.usbserial* /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

### 4. Build

From the repo root (same steps as [`prodecure.txt`](prodecure.txt)):

```bash
make clean
make
```

You should get `build/firmware.elf`. Optional:

```bash
arm-none-eabi-size build/firmware.elf
arm-none-eabi-objdump -s -j .isr_vector build/firmware.elf
```

### 5. Flash

```bash
openocd \
  -f interface/stlink.cfg \
  -f target/stm32l4x.cfg \
  -c "program build/firmware.elf verify reset exit"
```

Or: `make flash`.

Success looks like:

```text
** Programming Finished **
** Verified OK **
** Resetting Target **
```

### 6. Serial monitor

Only this process opens the serial port. It prints hex and writes `mic_capture.csv`.

```bash
python3 serial_monitor.py
python3 serial_monitor.py /dev/tty.usbserial-XXXX   # if the default port is wrong
```

Example line: `00 ff fa 0c  00 00 12 34` — unsigned 32-bit sample, then unsigned 32-bit timer (ms).

Restart the monitor after a board reset. Close anything else using the port first.

### 7. Plotter (second terminal)

Leave the monitor running, then:

```bash
python3 serial_plot.py
```

The plotter does not open UART; it follows the CSV. **Pause** + **View** scrub history; **Follow** returns to live. The plot sign-extends 24-bit PCM for display only.

### 8. Older experiments

[`test_code/`](test_code/) is **not** built by the Makefile. To try one, copy it over `src/main.c` and repeat build + flash.

| File | What it was |
|------|-------------|
| [`test_code/blink/blink.c`](test_code/blink/blink.c) | GPIOA LED |
| [`test_code/uart_char/uart_char.c`](test_code/uart_char/uart_char.c) | UART character TX |
| [`test_code/uart_word/uart_word.c`](test_code/uart_word/uart_word.c) | UART multi-byte TX |
| [`test_code/uart_adc/uart_adc.c`](test_code/uart_adc/uart_adc.c) | ADC + UART |

More flash/debug notes (including F5 / F10 / F11) are in [`prodecure.txt`](prodecure.txt).

---

## Repo layout

| Path | Role |
|------|------|
| [`src/main.c`](src/main.c) | Mic + UART + SysTick firmware |
| [`startup_stm32l475xx.s`](startup_stm32l475xx.s) | Vector table, `.data` / `.bss`, `Reset_Handler` |
| [`stm32l475vg.ld`](stm32l475vg.ld) | FLASH / RAM |
| [`Makefile`](Makefile) | Build + `make flash` |
| [`prodecure.txt`](prodecure.txt) | Build/flash/debug checklist |
| [`serial_monitor.py`](serial_monitor.py) | Hex dump + CSV |
| [`serial_plot.py`](serial_plot.py) | History waveform from the CSV |
| [`test_code/`](test_code/) | Older experiments |

---

## References

| Document | ID | What it was used for | Link |
|----------|-----|----------------------|------|
| STM32L47/L48/L49/L4A reference manual | **RM0351** | Memory map; RCC; GPIO; UART4; DFSDM registers (`CHyCFGR1`, `FLTxCR1`, `FLTxFCR`, `FLTxISR`, `FLTxRDATAR`) | [PDF](https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| STM32L475xx datasheet | **DS11585** | Pin alternate functions: PA0 AF8 = UART4_TX; PE7 AF6 = DFSDM1_DATIN2; PE9 AF6 = DFSDM1_CKOUT | [STM32L475VG](https://www.st.com/en/microcontrollers-microprocessors/stm32l475vg.html) |
| B-L475E-IOT01A user manual | **UM2153** | Arduino D1 = PA0; onboard MEMS mic; ST-Link VCP is USART1 on PB6/PB7 (not this UART) | [B-L475E-IOT01A](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html) |
| Cortex-M4 programming manual | **PM0214** | SysTick (`STK_CTRL` / `LOAD` / `VAL`), vector table, exceptions | Search ST “PM0214” |

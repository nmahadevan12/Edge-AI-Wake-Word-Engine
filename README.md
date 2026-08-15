# STM32L475 Discovery — bare-metal PDM mic

Register-level firmware for the **B-L475E-IOT01A** (`STM32L475VG`). No HAL. Application code is [`src/main.c`](src/main.c).

It clocks the onboard MEMS mic (PDM), converts that bit stream to PCM with DFSDM, and sends each sample plus a millisecond timestamp over **UART4** on Arduino **D1 (PA0)**.

---

## What `main.c` does

Bring-up, then a loop:

1. **UART4 TX** on PA0 (AF8), 115200 baud, 8 data bits, **2 stop bits**
2. **SysTick** at 1 ms → global `timer++`
3. **GPIOE** PE9 = DFSDM1_CKOUT (~2 MHz), PE7 = DFSDM1_DATIN2 (mic data)
4. **DFSDM** channel 2 + filter 0 (Sinc3, oversampling 64), continuous regular conversions
5. Wait for a sample (`REOCF`), send **4-byte big-endian PCM**, then **4-byte big-endian `timer`**, then a busy-wait delay

Wire format (8 bytes per loop):

```text
[ mic3 mic2 mic1 mic0 | t3 t2 t1 t0 ]
  signed 24-bit PCM     ms since boot
  in a 32-bit BE field
```

The firmware does `FLT0RDATAR >> 8` on an unsigned value, so negatives often look like `00 ff xx xx` on the wire. [`serial_monitor.py`](serial_monitor.py) sign-extends bit 23 for the CSV/plot.

Talk into the **onboard MEMS mic**, not the ST-Link USB end. USB-TTL: **RX ← D1 (PA0)**, common GND. Host serial: **115200 8N2**. ST-Link virtual COM is a different USART (PB6/PB7).

---

## `main.c` map

| Function | Role |
|----------|------|
| `UART4_GPIO_Init` / `UART_Setup` | PA0 AF8, UART4 clock, MSI 16 MHz, baud 139 |
| `UART_Transmit` / `Convert_To_Bytes` | Wait TXE, send 4-byte big-endian |
| `Mic_Setup` | GPIOE clock, PE7/PE9 AF6 |
| `DFSDM_Setup` | DFSDM1 clock, CKOUT divider, ch2, Sinc3 + FOSR, start conversion |
| `Get_Mic_Sample` | Poll `REOCF`, read `FLT0RDATAR`, UART the sample |
| `SysTick_Init` / `SysTick_Handler` | 1 ms tick, `timer++` |
| `main` | Init, then sample + timer TX forever |

Bases and bits follow **RM0351** (RCC, GPIO, USART, DFSDM). SysTick is **PM0214**. Pin AF numbers are in the **STM32L475 datasheet**. Board pins are **UM2153**.

---

## Run it yourself

Needs: `arm-none-eabi-gcc`, OpenOCD, ST-Link, USB-TTL on D1, Python 3.

**Build and flash** (also listed in [`prodecure.txt`](prodecure.txt)):

```bash
make clean
make
openocd \
  -f interface/stlink.cfg \
  -f target/stm32l4x.cfg \
  -c "program build/firmware.elf verify reset exit"
```

Or `make flash` (uses `reset run` then `shutdown`).

Optional inspect commands are in `prodecure.txt` (`arm-none-eabi-size`, `objdump`). Debug keys (F5 / F10 / F11) are there too.

**Host (two terminals)** — only the monitor opens the serial port; the plotter follows its log:

```bash
pip3 install pyserial matplotlib
python serial_monitor.py                          # hex dump + mic_capture.csv
python serial_monitor.py /dev/tty.usbserial-XXXX  # if the default port is wrong
python serial_plot.py                             # waveform; Pause + View to scrub history
```

Restart the monitor after a board reset so framing stays aligned. Close anything else using the port first.

### Cursor / VS Code buttons

Install the recommended **Task Buttons** extension. Status bar:

| Button | Command |
|--------|---------|
| Build | `make clean && make` |
| Flash | OpenOCD program / verify / reset |
| Monitor | `python serial_monitor.py` |
| Plotter | `python serial_plot.py` |
| Git | `git add .`, prompt for commit message, `git commit`, `git push` |

Same tasks: **Terminal → Run Task**. Start Monitor before Plotter.

---

## Other firmware in `test_code/`

Earlier bring-up sketches. They are **not** built by the top-level Makefile. Copy one over `src/main.c` (or compile it the same way) if you want that experiment.

| File | What it was |
|------|-------------|
| [`test_code/blink/blink.c`](test_code/blink/blink.c) | GPIOA LED |
| [`test_code/uart_char/uart_char.c`](test_code/uart_char/uart_char.c) | UART character TX |
| [`test_code/uart_word/uart_word.c`](test_code/uart_word/uart_word.c) | UART multi-byte TX |
| [`test_code/uart_adc/uart_adc.c`](test_code/uart_adc/uart_adc.c) | ADC + UART |

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

## Docs

| Document | Use |
|----------|-----|
| **RM0351** | Registers (RCC, GPIO, UART4, DFSDM) |
| **DS11585** | Pin AF (PA0 AF8 UART4_TX, PE7/PE9 AF6 DFSDM) |
| **UM2153** | Board connectors, mic, D1 = PA0 |
| **PM0214** | SysTick |

More flash / vector / BSS notes from earlier UART bring-up live in git history if you need them. For day-to-day: `prodecure.txt` then the host scripts above.

# STM32L475 bare-metal — B-L475E-IOT01A

Technical notes for register-level firmware on the **STM32L475VG** Discovery IoT kit (`B-L475E-IOT01A`). Application code is in [`src/main.c`](src/main.c): **UART4 TX** on Arduino **D1 (PA0)** sending a binary stream of a sample index and a millisecond timestamp driven by **SysTick**.

---

## 1. Official sources

| Document | ID | Role in this project | Link |
|----------|-----|----------------------|------|
| Reference manual | **RM0351** | Memory map, RCC, GPIO, USART/UART4 | [PDF](https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| MCU datasheet | **DS11585** (STM32L475xx) | Pin AF tables (PA0 → AF8 = UART4_TX) | [STM32L475VG](https://www.st.com/en/microcontrollers-microprocessors/stm32l475vg.html) |
| Board user manual | **UM2153** | Arduino D1, ST-Link VCP vs header UART | [B-L475E-IOT01A](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html) |
| Cortex-M4 programming manual | **PM0214** | SysTick registers (`STK_CTRL` / `LOAD` / `VAL`), vector table, exceptions | Search ST “PM0214” |
| OpenOCD | — | Flash / `reset run` | [Flash commands](https://openocd.org/doc/html/Flash-Commands.html) |

**Which doc for what**

- **RM0351** — `#define` bases, bit masks, USART/RCC/GPIO register layouts  
- **PM0214** — SysTick (not fully described in RM0351; RM only notes HCLK vs HCLK/8 and points here)  
- **Datasheet AF table** — pin ↔ alternate-function number  
- **UM2153** — physical connector (D1 = PA0; ST-Link VCP is USART1 on PB6/PB7)

---

## 2. System overview

```text
MSI 16 MHz  →  SYSCLK  →  HCLK (AHB ÷1)
                              ├─ CPU / SysTick (CLKSOURCE=1)
                              └─ UART4 kernel clock (this bring-up)
                                    └─ PA0 / Arduino D1 → USB-TTL RX
```

Firmware loop:

1. Pack `number` (`uint16_t`) into `buffer[0..1]` (big-endian)  
2. Transmit **4** bytes from `buffer` (bytes 2–3 may be stale from the previous timer pack)  
3. Pack `timer` (`uint32_t`, ms since boot) into `buffer[0..3]` (big-endian)  
4. Transmit **4** bytes  
5. `number++`, busy-wait delay  
6. In parallel: SysTick IRQ every 1 ms → `timer++`

Host tool [`serial_monitor.py`](serial_monitor.py) reads **8-byte** frames, prints the **uint16** in bytes `[0:2]` and the **uint32** timer in bytes `[4:8]`, and ignores the two stale bytes in between.

---

## 3. Memory map and bases — RM0351 Section 2

| Symbol | Address | Bus |
|--------|---------|-----|
| `PERIPH_BASE` | `0x40000000` | Peripheral region |
| `APB1PERIPH_BASE` | `0x40000000` | APB1 |
| `AHB1PERIPH_BASE` | `0x40020000` | AHB1 |
| `AHB2PERIPH_BASE` | `0x48000000` | AHB2 (GPIO) |
| `UART4_BASE` | `0x40004C00` | APB1 + `0x4C00` |
| `RCC_BASE` | `0x40021000` | AHB1 + `0x1000` (**not** APB1 + `0x1000`) |
| `GPIOA_BASE` | `0x48000000` | AHB2 |
| `SYSTICK_BASE` | `0xE000E010` | Cortex-M System Control Space (PM0214) |

Register macros use `*(volatile uint32_t *)(base + offset)` so they can be read/written like variables.

---

## 4. Clock tree — RM0351 Section 6

### MSI → HCLK

On reset, SYSCLK is typically MSI with AHB prescaler ÷1, so **HCLK = MSI**.

In `UART_Setup()`:

| Step | Register / field | Meaning |
|------|------------------|---------|
| Clear range | `RCC_CR` MSI range bits | Prepare new range |
| `MSI_16MHZ` | `MSIRANGE` = `1000b` | Select ~16 MHz (RM0351 MSI range table) |
| `CLK_RANGE_SEL` | `MSIRGSEL` = 1 | Use `MSIRANGE` from `RCC_CR` **now** |

SysTick with **CLKSOURCE = 1** uses the **processor / AHB clock (HCLK)**. Naming:

- **MSI** — oscillator you configure  
- **SYSCLK** — system clock source (here = MSI)  
- **HCLK / AHB clock** — after AHB prescaler; same frequency as MSI while HPRE = ÷1  

RM0351 clock-tree text: SysTick external clock is HCLK/8; software may select HCLK or HCLK/8 in the SysTick control register — details in **PM0214**.

### Peripheral clocks

| Enable | Register | Bit |
|--------|----------|-----|
| UART4 | `RCC_APB1ENR1` (`0x58`) | `UART4EN` bit 19 |
| GPIOA | `RCC_AHB2ENR` (`0x4C`) | `GPIOAEN` bit 0 |

---

## 5. GPIO — UART4_TX on PA0 — RM0351 Section 8 + datasheet + UM2153

| Item | Setting | Source |
|------|---------|--------|
| Board pin | Arduino **D1** = **PA0** | UM2153 |
| Mode | `MODER[1:0] = 10` (alternate function) | RM0351 `GPIOx_MODER` |
| AF | **AF8** = UART4_TX | Datasheet AF table |
| Register | `GPIOA->AFRL` nibble for pin 0 | RM0351 AFRL (pins 0–7) |

Init order: enable GPIOA clock → clear/set PA0 `MODER` bits only (do not wipe the whole port; SWD pins share GPIOA) → program AF8.

---

## 6. UART4 — RM0351 Section 40 (USART)

UART4 uses the USART register map at `UART4_BASE`.

| Register | Offset | Use in this project |
|----------|--------|---------------------|
| `CR1` | `0x00` | 8 data bits (`M1`/`M0` = 0), `TE`, then `UE` |
| `CR2` | `0x04` | **2 stop bits** (`STOP` = `10`) |
| `BRR` | `0x0C` | Baud divider |
| `ISR` | `0x1C` | Wait for `TXE` (bit 7) before each write |
| `TDR` | `0x28` | Transmit data |

**Baud:** with OVER8 = 0 and `f_CK` ≈ 16 MHz:

```text
BRR ≈ 16_000_000 / 115200 ≈ 138.89 → 139
```

Configure with `UE = 0`, then set `UE = 1` (RM0351 USART init guidance).

**Host serial settings:** `115200 8N2` (match 2 stop bits). USB-TTL **RX ← D1**, common GND. ST-Link virtual COM is **not** this UART (USART1 / PB6–PB7 per UM2153).

---

## 7. SysTick — PM0214 (+ RM0351 clock note)

SysTick is a Cortex-M core peripheral, not an STM32 AHB/APB device block.

| Register | Address | Role |
|----------|---------|------|
| `STK_CTRL` | `0xE000E010` | ENABLE, TICKINT, CLKSOURCE, COUNTFLAG |
| `STK_LOAD` | `0xE000E014` | Reload value |
| `STK_VAL` | `0xE000E018` | Current down-counter |
| `STK_CALIB` | `0xE000E01C` | Calibration (chip-specific; RM0351 Section 13.2) |

### CTRL bits used (PM0214)

| Bit | Name | Setting here |
|-----|------|----------------|
| 0 | ENABLE | 1 — start counter |
| 1 | TICKINT | 1 — exception on wrap to 0 |
| 2 | CLKSOURCE | 1 — processor clock (**AHB / HCLK**), not HCLK/8 |

### 1 ms tick

```text
STK_LOAD = (HCLK_Hz / 1000) - 1
         = (16_000_000 / 1000) - 1
         = 15999  (0x3E7F)
```

`STK_VAL` counts **down** from LOAD to 0 over ~1 ms, then hardware reloads from LOAD. Software does **not** rewrite LOAD each interrupt.

### Software timestamp

- **`STK_VAL`** — position inside the current 1 ms period (not “ms since boot”)  
- **`timer`** — incremented in `SysTick_Handler` on each wrap → milliseconds since init  

Init (`SysTick_Init`) programs CTRL/LOAD once from `main`. The handler only runs `timer++`. Do not call `SysTick_Handler()` from the main loop; the CPU enters it via the exception.

### Vector table — PM0214 + [`startup_stm32l475xx.s`](startup_stm32l475xx.s)

Exception 15 (SysTick) must point at `SysTick_Handler`. This project’s table ends with:

```asm
.word SysTick_Handler          /* SysTick */
```

If that slot remains `Default_Handler` and TICKINT is set, the core hangs in an infinite loop on the first tick.

RM0351 Section 13.3 lists vectors; programming details are in PM0214.

---

## 8. Startup and C runtime — [`startup_stm32l475xx.s`](startup_stm32l475xx.s) + [`stm32l475vg.ld`](stm32l475vg.ld)

`Reset_Handler`:

1. Set `SP` = `_estack` (top of SRAM1, 96 KiB @ `0x20000000` per linker script)  
2. Copy `.data` from flash (`_sidata`) into RAM (`_sdata`…`_edata`)  
3. Zero `.bss` (`_sbss`…`_ebss`)  
4. Branch to `main`

**Why BSS zero matters:** STM32 **SRAM contents survive NRST**. Without BSS clear, `number = 0` / `timer = 0` in C source do not re-apply after the reset button — counters appear to continue from old RAM values.

Linker script places `.isr_vector` and `.text` in FLASH @ `0x08000000`, `.data`/`.bss` in RAM, and defines `end` / `_end` for newlib nano (`--specs=nano.specs --specs=nosys.specs` in the Makefile).

---

## 9. Binary wire format

### Endianness

Packing in `Number_To_Bytes` / `Timer_To_Bytes` places the **MSB in `buffer[0]`** (big-endian on the wire), even if comments still say “little endian.”

| Field | C type | Bytes written into `buffer` | On wire (BE examples) |
|-------|--------|-----------------------------|------------------------|
| `number` | `uint16_t` | `[0]`, `[1]` only | `00 05` for 5 |
| `timer` | `uint32_t` | `[0]`…`[3]` | `00 00 01 28` for 296 ms |

### Frame layout (8 bytes per loop)

```text
[ n1 n0 | stale stale | t3 t2 t1 t0 ]   (significance after BE pack)
  <––– 4-byte TX #1 –––><–– 4-byte TX #2 ––>
```

`UART_Transmit_Ptr` always sends **4** bytes. After a timer transmit, `buffer` still holds timer data. The next `Number_To_Bytes` overwrites only `[0]` and `[1]`, so **`[2]` and `[3]` are stale timer bytes** in the first half of the frame.

### Host decode — [`serial_monitor.py`](serial_monitor.py)

```text
read 8 bytes
print bytes[0:2] as number (uint16)
skip bytes[2:4]          (stale)
print bytes[4:8] as timer (uint32 BE hex)
```

```bash
pip3 install pyserial
python3 serial_monitor.py              # default port in script
python3 serial_monitor.py /dev/tty.usbserial-XXXX
```

Match **2 stop bits**. Close any other process using the port first. Restart the logger after reset so 8-byte framing resyncs. Attach the logger **before** reset/flash if you want to observe `number`/`timer` from zero.

---

## 10. Build and flash

```bash
make clean && make
make flash
```

Equivalent OpenOCD (use **`reset run`**, not halt-only reset):

```bash
openocd \
  -f interface/stlink.cfg \
  -f target/stm32l4x.cfg \
  -c "program build/firmware.elf verify; reset run; shutdown"
```

```text
src/main.c + startup_stm32l475xx.s
        → arm-none-eabi-gcc + stm32l475vg.ld
        → build/firmware.elf
        → OpenOCD / ST-Link
        → Reset_Handler → BSS/data init → main()
```

Optional inspect:

```bash
arm-none-eabi-size build/firmware.elf
arm-none-eabi-objdump -s -j .isr_vector build/firmware.elf
```

---

## 11. Gotchas from bring-up

1. **`RCC_BASE` = `0x40021000` (AHB1)** — using APB1+`0x1000` leaves clocks off; TXE never sets.  
2. **`MSIRGSEL = 1`** before trusting `MSIRANGE` in `RCC_CR`.  
3. **SysTick detail is in PM0214**, not the RM0351 register map; RM only describes clock source choice and CALIB.  
4. **Vector SysTick → `SysTick_Handler`** or TICKINT hangs in `Default_Handler`.  
5. **Zero `.bss` on reset** or counters won’t restart at 0 (SRAM retained).  
6. **Flash with `reset run`**.  
7. **Binary ≠ ASCII** — a text serial view shows garbage; use the hex logger or a logic analyzer.  
8. **4-byte TX of a 2-byte number** inserts stale bytes; the Python monitor skips them. Clearing unused `buffer` bytes or sending length 2 would remove the need to skip.  
9. **GPIOA `MODER`** — mask PA0 only; full-register writes can break SWD.  
10. **D1 / PA0** is the data path; ST-Link `usbmodem` is a different USART.

---

## 12. Repo layout

| Path | Role |
|------|------|
| [`src/main.c`](src/main.c) | Registers, UART4, SysTick, binary TX loop |
| [`startup_stm32l475xx.s`](startup_stm32l475xx.s) | Vector table, Reset_Handler, `.data`/`.bss` init |
| [`stm32l475vg.ld`](stm32l475vg.ld) | FLASH/RAM layout, section symbols |
| [`Makefile`](Makefile) | Build + OpenOCD flash |
| [`serial_monitor.py`](serial_monitor.py) | Host hex dump of uint16 + uint32 fields |
| [`test_code/`](test_code/) | Earlier experiments (blink, UART char/string) |

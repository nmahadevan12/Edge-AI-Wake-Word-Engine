# STM32L475 bare-metal (B-L475E-IOT01A)

Register-level practice on the Discovery IoT kit: GPIO, then **UART4 TX** on Arduino **D1 (PA0)**.

Code lives mainly in [`src/main.c`](src/main.c). Comments there point at the same manuals listed below.

---

## Official docs (start here)

| Doc | ID | What it’s for | Link |
|-----|-----|----------------|------|
| **Reference manual** | **RM0351** | Registers, clocks, USART, GPIO, memory map | [PDF](https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| **MCU datasheet** | **DS11585** (STM32L475xx) | Pinout, AF tables (which pin → AF8 UART4, etc.) | [Product / docs](https://www.st.com/en/microcontrollers-microprocessors/stm32l475vg.html) |
| **Board user manual** | **UM2153** | B-L475E-IOT01A connectors, Arduino D1, ST-Link VCP | [Board page](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html) |
| **Cortex-M4 prog. manual** | **PM0214** | Exceptions, vector table, Thumb, etc. | ST “PM0214” search |
| **OpenOCD** | — | `program` / `reset run` | [OpenOCD docs](https://openocd.org/doc/html/Flash-Commands.html) |

**How to use them**

1. **RM0351** — while writing `#define`s and bit masks  
2. **Datasheet AF table** — when picking pin + alternate function number  
3. **UM2153** — when wiring Arduino headers / ST-Link COM vs D1  

---

## Code ↔ manual map (`src/main.c`)

### Bases & memory map — RM0351 §2 (Memory map)

| In code | Value | Manual |
|---------|--------|--------|
| `PERIPH_BASE` | `0x40000000` | Peripheral bus start |
| `APB1PERIPH_BASE` | `0x40000000` | APB1 |
| `AHB1PERIPH_BASE` | `0x40020000` | AHB1 |
| `AHB2PERIPH_BASE` | `0x48000000` | AHB2 (GPIO…) |
| `UART4_BASE` | `0x40004C00` | APB1 → UART4 |
| `RCC_BASE` | `0x40021000` | AHB1 → RCC (**not** APB1+`0x1000`) |
| `GPIOA_BASE` | `0x48000000` | AHB2 → GPIOA |

### RCC — RM0351 §6 (Reset and clock control)

| In code | Register / field | Manual |
|---------|------------------|--------|
| `RCC_CR` | offset `0x00` | §6.4.1 Clock control register |
| `MSIRANGE` / `MSI_16MHZ` | `RCC_CR[7:4]` | MSI ranges (e.g. `1000` ≈ 16 MHz) |
| `MSI_RANGE_SEL` | `MSIRGSEL` bit 3 | Must be **1** to use `MSIRANGE` from `RCC_CR` |
| wait `MSIRDY` | bit 1 | Clock ready |
| `RCC_APB1ENR1` | offset `0x58` | §6.4.x APB1 peripheral clock enable 1 |
| `UART4_EN` | bit 19 `UART4EN` | Enable UART4 clock |
| `RCC_AHB2ENR` | offset `0x4C` | AHB2 enable |
| `GPIOA_CLOCK` | bit 0 `GPIOAEN` | Enable GPIOA clock |

### GPIO — RM0351 §8 (GPIO) + datasheet AF table

| In code | Meaning | Manual |
|---------|---------|--------|
| `GPIO_TypeDef` | `MODER`…`AFRH` layout | §8.4 GPIO registers (offsets `0x00`…`0x24`) |
| `MODER` PA0 → `10` | Alternate function | §8.4.1 `GPIOx_MODER` |
| `AFRL` | Pins 0–7 AF nibbles | §8.4.9 / 8.4.10 `AFRL`/`AFRH` |
| `AF8_UART4` = `8` | PA0 = UART4_TX | **Datasheet** alternate-function table (not RM) |
| D1 = PA0 | Board wiring | **UM2153** Arduino connector |

Pin rule: pin 0–7 → **AFRL**; pin 8–15 → **AFRH**. Each pin = one 4-bit AF field.

### USART/UART — RM0351 §40 (USART)

UART4 uses the same register map as USART (instance at `0x40004C00`).

| In code | Offset / bits | Manual |
|---------|----------------|--------|
| `USART_CR1` | `0x00` | §40.8.1 — `UE`, `TE`, `M1`/`M0` |
| `WORD_LENGTH` | `M[1:0]=00` | 8 data bits (`M1` bit 28, `M0` bit 12) |
| `TX_ENABLE` | `TE` bit 3 | Transmitter enable |
| `UART_ENABLE` | `UE` bit 0 | USART enable (set **after** config/`BRR`) |
| `USART_CR2` | `0x04` | §40.8.2 — `STOP[1:0]` |
| `STOP_BITS` | `10` | **2** stop bits (`00` = 1 stop) |
| `USART_BRR` | `0x0C` | §40.8.4/5 — write with `UE=0` |
| `BAUD_DIV_115200` | `139` | `OVER8=0`: `BRR ≈ f_CK / baud` → `16e6/115200` |
| `USART_ISR` | `0x1C` | §40.8.x status |
| `UART_TXE` | bit 7 | `TDR` empty / moved to shift register |
| `USART_TDR` | `0x28` | Write data to send |

**TXE vs TC:** TXE = room in `TDR` (pipeline next byte). TC = full frame finished on the wire (incl. stop). See ISR bit descriptions in §40.

### Init order (why `main` calls GPIO then UART)

1. Enable GPIOA clock → set `MODER` + `AFRL` (route UART4_TX to PA0)  
2. Enable UART4 clock → program CR1/CR2/`BRR` with `UE=0` → set MSI if needed → `UE=1`  
3. Wait **TXE**, write **TDR**  

Matches the “configure while disabled, then enable” notes in the USART chapter.

---

## Build

```bash
make clean
make
```

Optional:

```bash
arm-none-eabi-size build/firmware.elf
arm-none-eabi-objdump -d build/firmware.elf
arm-none-eabi-objdump -s -j .isr_vector build/firmware.elf
```

## Flash

Use **`reset run`** so the CPU runs after programming:

```bash
make flash
```

```bash
openocd \
  -f interface/stlink.cfg \
  -f target/stm32l4x.cfg \
  -c "program build/firmware.elf verify; reset run; shutdown"
```

```text
Source → make → build/firmware.elf → OpenOCD (ST-Link) → Reset_Handler → main()
```

## Serial monitor

| Item | Value |
|------|--------|
| Wire | USB–TTL **RX ← D1 (PA0)**, GND↔GND |
| Device | `/dev/cu.usbserial-…` (prefer `cu.` over `tty.`) |
| Settings | **115200 8N2** (firmware uses 2 stop bits) |
| Not this code | ST-Link `usbmodem` = **USART1** PB6/PB7 (**UM2153**) |

One process at a time on the port (`screen` vs Serial Monitor).

## Gotchas learned in this project

1. **`RCC_BASE` must be `0x40021000` (AHB1)** — `APB1+0x1000` is wrong; clocks never enable → stuck on TXE.  
2. **`MSIRGSEL=1`** before relying on `MSIRANGE` in `RCC_CR`.  
3. **Flash with `reset run`**, not a halt-only reset.  
4. **GPIOA `MODER` reset ≠ 0** — don’t assign the whole register; clear/set PA0 bits only (keeps SWD pins).  
5. **AFRL reset is 0** — full assign of AF8 for PA0 is OK at bring-up.

## Debug (IDE)

| Key | Action |
|-----|--------|
| F5 | Start / continue |
| F10 | Step over |
| F11 | Step into |
| Shift+F5 | Stop |

If `runToEntryPoint` is `main`, continue so GPIO/UART setup actually runs.

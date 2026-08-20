# UART Protocol

Binary framing between the STM32 and host tools (`serial_monitor.py`, `serial_plot.py`).

---

## Link settings

| Parameter | Value |
|-----------|--------|
| Peripheral | UART4 TX |
| Pin | PA0 (Arduino **D1**) |
| Baud | 115200 |
| Data | 8 bits |
| Parity | None |
| Stop bits | **2** (8N2) |
| Byte order | **Big-endian** for multi-byte fields |

---

## Stream A — live sample frame (16 bytes)

Sent once per `Get_Mic_Sample` / processing loop after warm-up. Field order on the wire:

```text
[ sample y ][ energy ][ flag ][ timer ]
   4 bytes    4 bytes   4 bytes  4 bytes
```

| Offset | Type | Source in firmware |
|--------|------|--------------------|
| 0–3 | `int32_t` | `Convert_Int_To_Bytes(y)` in `DC_Blocker` |
| 4–7 | `uint32_t` | `Convert_Uint_To_Bytes(energy)` in `Update_Energy` |
| 8–11 | `uint32_t` | `Convert_Uint_To_Bytes(0 or 1)` in `Sound_Detect` |
| 12–15 | `uint32_t` | `Convert_Uint_To_Bytes(timer)` in `main` |

### Flag meaning

| Value | Meaning |
|-------|---------|
| 0 | `loud_count < DEBOUNCE_VAL` (or warm-up / idle) |
| 1 | `loud_count ≥ DEBOUNCE_VAL` within the current capture |

Host CSV (`mic_capture.csv`):

```text
timer_ms,sample,energy,flag
```

The plotter may also derive **loud** as `energy > 500` independently of the UART flag (debounce).

---

## Stream B — buffer dump (1032 bytes)

Inserted into the UART stream when a capture window is **accepted** (`loud_count ≥ DEBOUNCE_VAL` at `index ≥ CAPACITY`).

```text
[ 0xFFFFFFFF ][ y0 ][ y1 ] ... [ y255 ][ 0x00000000 ]
   4 bytes      4 B    4 B         4 B      4 bytes
```

| Segment | Size | Content |
|---------|------|---------|
| Start marker | 4 | `0xFFFFFFFF` |
| Payload | 1024 | 256 × `int32_t` big-endian PCM `y` |
| End marker | 4 | `0x00000000` |
| **Total** | **1032** | |

### Host handling

`serial_monitor.py`:

1. Looks for a candidate start word `0xFFFFFFFF`.
2. Reads a full 1032-byte candidate.
3. Accepts only if the last word is `0x00000000` (rejects false starts from `sample == -1`).
4. Writes `buffer_dump_NNN.csv` as `index,y`.

---

## Alignment rules (important)

- Every live frame must be **exactly 16 bytes**. Omitting the flag field after warm-up desynchronizes the host (energy/timer look like huge garbage values).
- Dumps may appear between live frames; the monitor state machine must not treat dump payload as live frames.
- After flash, restart monitor (and plotter) so CSV offsets and dump session filters reset.

---

## Example decode (conceptual)

Live frame hex (illustrative):

```text
00 00 04 D2   00 00 05 DC   00 00 00 01   00 00 27 10
 sample=1234   energy=1500   flag=1         t=10000
```

Related: [SIGNAL_PROCESSING.md](SIGNAL_PROCESSING.md) · [PROCEDURE.md](PROCEDURE.md)

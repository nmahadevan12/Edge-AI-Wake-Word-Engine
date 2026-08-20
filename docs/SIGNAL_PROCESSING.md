# Signal Processing Notes

Equations and control logic used in [`src/main.c`](../src/main.c).

---

## 1. DFSDM (PDM → PCM)

The onboard MEMS microphone outputs a **PDM** bitstream. DFSDM filter 0 (Sinc3, oversampling ratio 64) produces a signed PCM word. Firmware reads `FLT0RDATAR` and uses the upper bits:

```c
mic_data = FLT0RDATAR;
x = mic_data >> 8;   // 24-bit PCM-style sample into DC blocker
```

Approximate full-scale magnitude for Sinc3 with FOSR = 64:

```text
full_scale ≈ FOSR³ = 64³ = 262144
```

The host plotter uses `FULL_SCALE = 262144` for axis limits.

---

## 2. DC blocker (high-pass IIR)

Removes a slowly varying offset from the mic path:

```text
y[n] = x[n] − x[n−1] + R · y[n−1]
```

| Symbol | Code | Value |
|--------|------|-------|
| `x[n]` | raw PCM into `DC_Blocker` | from DFSDM |
| `y[n]` | DC-blocked sample | UART field 0 |
| `R` | `dc.R` | `0.995` |

`R` close to 1 → long time constant (slow DC tracking, less distortion of low-frequency speech).

---

## 3. Energy (EMA of absolute amplitude)

```text
E[n] = α · |y[n]| + (1 − α) · E[n−1]
```

| Symbol | Code | Value |
|--------|------|-------|
| `α` | `energy.alpha` | `0.2` |
| `E[n]` | `energy.energy` | UART field 1 |

Larger `α` → energy follows loudness faster (noisier). Smaller `α` → smoother envelope, more lag.

**Loud condition** used in the capture FSM:

```text
loud  ⟺  E[n] > 500
```

---

## 4. Capture finite-state machine

Constants:

| Name | Value | Role |
|------|-------|------|
| `CAPACITY` | 256 | Fixed PCM window (1024 bytes) |
| `DEBOUNCE_VAL` | 30 | Minimum loud frames to keep dump |
| Quiet abort | `low_count > 250` | Discard if never reaches debounce |
| Warm-up | `timer < 7000` ms | No capture after reset |

### States

```text
IDLE
  │  energy > 500
  ▼
CAPTURING  ── store y[n] every sample
  │              loud  → loud_count++, low_count = 0
  │              quiet → low_count++
  │
  ├─ low_count > 250 AND loud_count < DEBOUNCE_VAL  →  discard → IDLE
  │
  └─ index ≥ CAPACITY
        ├─ loud_count ≥ DEBOUNCE_VAL  →  Send_Sample_Buffer() → IDLE
        └─ else                       →  discard → IDLE
```

### Counters

```text
if E[n] > 500:
    loud_count = loud_count + 1
    low_count  = 0
else:
    low_count  = low_count + 1
```

Debounce **pass** (UART flag = 1):

```text
debounce_passed = (loud_count ≥ DEBOUNCE_VAL)
```

The flag can go high **during** the window once 30 loud frames have been seen; the dump is only committed when the window completes (or is aborted).

---

## 5. Why fixed length?

ML training prefers **constant-size** examples. A 256-sample buffer is:

```text
256 samples × 4 bytes/sample = 1024 bytes of PCM
```

Plus UART markers → **1032 bytes** on the wire. Duration in real time depends on the sample loop rate (DFSDM wait + software delay in `main`) — calibrate with the plotter timer axis if you need exact seconds.

---

## 6. Design tradeoffs

| Choice | Benefit | Cost |
|--------|---------|------|
| Capture every `y` after trigger | Continuous waveform for models | Short window may truncate long phrases |
| Debounce on loud **count** | Simple; rejects weak blips | Strong taps can still pass |
| Quiet abort before debounce | Kills many taps early | Long mid-word pauses can abort if debounce not yet met |
| EMA energy | Cheap on MCU | Threshold 500 is board/room dependent |

Related: [PROTOCOL.md](PROTOCOL.md) · [PROCEDURE.md](PROCEDURE.md)

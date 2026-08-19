#!/usr/bin/env python3
"""Hex serial monitor for STM32 binary UART frames.

Normal frame (16 bytes, big-endian):
  [0:4]    signed int32   mic sample (DC-blocked)
  [4:8]    unsigned uint32 energy
  [8:12]   unsigned uint32 flag (0 or 1)
  [12:16]  unsigned uint32 timer_ms

Buffer dump (triggered when sample_buffer[255] is written):
  [0:4]    0xFFFFFFFF  start marker
  [N*4]    signed int32 x256  sample_buffer contents
  [last 4] 0x00000000  end marker
  Total: 4 + 256*4 + 4 = 1032 bytes
"""

import argparse
import sys
from pathlib import Path
from typing import Optional, Tuple

try:
    import serial
except ImportError:
    print("Install pyserial: pip3 install pyserial", file=sys.stderr)
    sys.exit(1)

DEFAULT_LOG = Path(__file__).resolve().parent / "mic_capture.csv"
DEFAULT_DUMP = Path(__file__).resolve().parent / "buffer_dump.csv"
FRAME_LEN = 16
MAX_TIMER_STEP_MS = 500

DUMP_START_MARKER = 0xFFFFFFFF
DUMP_END_MARKER   = 0x00000000
DUMP_SAMPLES      = 256
DUMP_TOTAL_BYTES  = 4 + DUMP_SAMPLES * 4 + 4  # 1032


def parse_frame(b: bytes):
    sample = int.from_bytes(b[0:4],  "big", signed=True)
    energy = int.from_bytes(b[4:8],  "big", signed=False)
    flag   = int.from_bytes(b[8:12], "big", signed=False)
    timer  = int.from_bytes(b[12:16],"big", signed=False)
    return sample, energy, flag, timer


def frame_ok(b: bytes, prev_timer: Optional[int]) -> bool:
    if len(b) != FRAME_LEN:
        return False
    flag = int.from_bytes(b[8:12], "big", signed=False)
    if flag not in (0, 1):
        return False
    if prev_timer is None:
        return True
    timer = int.from_bytes(b[12:16], "big", signed=False)
    dt = (timer - prev_timer) & 0xFFFFFFFF
    return dt <= MAX_TIMER_STEP_MS


def read_aligned_frame(
    ser, prev_timer: Optional[int]
) -> Tuple[bytes, Optional[int]]:
    """Read bytes one at a time until a valid 16-byte frame is found.
    If a dump start marker is detected, hand off to read_dump."""
    buf = bytearray()
    while True:
        chunk = ser.read(1)
        if not chunk:
            continue
        buf.extend(chunk)

        # Check if first 4 bytes look like the dump start marker
        if len(buf) >= 4:
            word = int.from_bytes(buf[:4], "big", signed=False)
            if word == DUMP_START_MARKER:
                return None, prev_timer  # signal caller to read dump

        while len(buf) >= FRAME_LEN:
            candidate = bytes(buf[:FRAME_LEN])
            if frame_ok(candidate, prev_timer):
                del buf[:FRAME_LEN]
                return candidate, int.from_bytes(candidate[12:16], "big", signed=False)
            del buf[0]


def read_dump(ser) -> Optional[list]:
    """Read the remaining dump bytes after the start marker was detected.
    Returns list of 256 signed int32 samples, or None on error."""
    # start marker (4 bytes) already consumed; read samples + end marker
    remaining = ser.read(DUMP_SAMPLES * 4 + 4)
    if len(remaining) != DUMP_SAMPLES * 4 + 4:
        print(f"[DUMP] Short read: got {len(remaining)} bytes, expected {DUMP_SAMPLES*4+4}")
        return None

    samples = []
    for i in range(DUMP_SAMPLES):
        val = int.from_bytes(remaining[i*4:(i+1)*4], "big", signed=True)
        samples.append(val)

    end_word = int.from_bytes(remaining[DUMP_SAMPLES*4:], "big", signed=False)
    if end_word != DUMP_END_MARKER:
        print(f"[DUMP] Bad end marker: 0x{end_word:08X}")

    return samples


def save_dump(samples: list, dump_path: Path, dump_count: int) -> None:
    path = dump_path.parent / f"{dump_path.stem}_{dump_count:03d}{dump_path.suffix}"
    with path.open("w", encoding="utf-8") as f:
        f.write("index,y\n")
        for i, v in enumerate(samples):
            f.write(f"{i},{v}\n")
    print(f"[DUMP] Saved {len(samples)} samples → {path.name}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="STM32 serial monitor: normal frames + buffer dumps"
    )
    parser.add_argument(
        "port",
        nargs="?",
        default="/dev/tty.usbserial-FTG8MPCG",
        help="serial port path",
    )
    parser.add_argument("-b", "--baud", type=int, default=115200)
    parser.add_argument(
        "--stopbits",
        type=int,
        choices=(1, 2),
        default=2,
        help="stop bits (firmware currently uses 2)",
    )
    parser.add_argument(
        "--log",
        default=str(DEFAULT_LOG),
        help="CSV log for normal frames (timer_ms,sample,energy,flag)",
    )
    parser.add_argument(
        "--dump",
        default=str(DEFAULT_DUMP),
        help="base path for buffer dump CSVs (buffer_dump_001.csv etc.)",
    )
    parser.add_argument(
        "--append",
        action="store_true",
        help="append to the log instead of starting a new capture",
    )
    args = parser.parse_args()

    stop = serial.STOPBITS_TWO if args.stopbits == 2 else serial.STOPBITS_ONE
    ser = serial.Serial(args.port, args.baud, stopbits=stop, timeout=1.0)
    ser.reset_input_buffer()

    log_path  = Path(args.log)
    dump_path = Path(args.dump)
    mode = "a" if args.append else "w"
    log = log_path.open(mode, encoding="utf-8")
    if mode == "w":
        log.write("timer_ms,sample,energy,flag\n")
        log.flush()

    print(f"Opened {args.port} @ {args.baud} stopbits={args.stopbits}")
    print(f"Normal frames → {log_path.name}")
    print(f"Buffer dumps  → {dump_path.stem}_NNN.csv")
    print("Syncing to frame boundary...")
    print("Ctrl+C to stop\n")

    prev_timer = None
    dump_count = 0

    try:
        while True:
            result, prev_timer = read_aligned_frame(ser, prev_timer)

            if result is None:
                # dump start marker detected
                print("[DUMP] Start marker received — reading buffer...")
                samples = read_dump(ser)
                if samples:
                    dump_count += 1
                    save_dump(samples, dump_path, dump_count)
                    print(f"[DUMP] {len(samples)} samples, "
                          f"peak={max(abs(v) for v in samples)}, "
                          f"min={min(samples)}, max={max(samples)}")
                prev_timer = None  # resync after dump
                continue

            b = result
            sample, energy, flag, timer = parse_frame(b)
            print(
                f"{b[0:4].hex(' ')}  {b[4:8].hex(' ')}  {b[8:12].hex(' ')}  {b[12:16].hex(' ')}  "
                f"sample={sample}  energy={energy}  flag={flag}  t={timer}"
            )
            log.write(f"{timer},{sample},{energy},{flag}\n")
            log.flush()

    except KeyboardInterrupt:
        print("\nClosed")
    finally:
        log.close()
        ser.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Hex serial monitor for STM32 binary UART frames.

Normal frame (16 bytes, big-endian):
  [0:4]    signed int32   mic sample (DC-blocked)
  [4:8]    unsigned uint32 energy
  [8:12]   unsigned uint32 debounce_passed (0 or 1)  — NOT energy>500
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
from typing import List, Optional, Tuple

try:
    import serial
except ImportError:
    print("Install pyserial: pip3 install pyserial", file=sys.stderr)
    sys.exit(1)

DEFAULT_LOG = Path(__file__).resolve().parent / "mic_capture.csv"
DEFAULT_DUMP = Path(__file__).resolve().parent / "buffer_dump.csv"
FRAME_LEN = 16
MAX_TIMER_STEP_MS = 500

DUMP_END_MARKER = 0x00000000
DUMP_SAMPLES = 256
DUMP_TOTAL_BYTES = 4 + DUMP_SAMPLES * 4 + 4  # 1032
DUMP_PAYLOAD_BYTES = DUMP_SAMPLES * 4 + 4


def parse_frame(b: bytes):
    sample = int.from_bytes(b[0:4], "big", signed=True)
    energy = int.from_bytes(b[4:8], "big", signed=False)
    flag = int.from_bytes(b[8:12], "big", signed=False)
    timer = int.from_bytes(b[12:16], "big", signed=False)
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


def _parse_dump_payload(payload: bytes) -> Optional[list]:
    if len(payload) != DUMP_PAYLOAD_BYTES:
        return None
    end_word = int.from_bytes(payload[DUMP_SAMPLES * 4 :], "big", signed=False)
    if end_word != DUMP_END_MARKER:
        return None
    samples = []
    for i in range(DUMP_SAMPLES):
        val = int.from_bytes(payload[i * 4 : (i + 1) * 4], "big", signed=True)
        samples.append(val)
    return samples


def read_aligned_frame(
    ser, prev_timer: Optional[int]
) -> Tuple[Optional[bytes], Optional[List[int]], Optional[int]]:
    """Read until a valid frame or a validated buffer dump.

    sample == -1 is 0xFFFFFFFF — treat as a normal frame unless a full 1032-byte
    dump with a valid end marker is present.
    """
    buf = bytearray()
    while True:
        chunk = ser.read(1)
        if not chunk:
            continue
        buf.extend(chunk)

        if len(buf) >= 4 and buf[:4] == b"\xff\xff\xff\xff":
            if len(buf) >= FRAME_LEN and frame_ok(bytes(buf[:FRAME_LEN]), prev_timer):
                candidate = bytes(buf[:FRAME_LEN])
                del buf[:FRAME_LEN]
                timer = int.from_bytes(candidate[12:16], "big", signed=False)
                return candidate, None, timer
            if len(buf) >= DUMP_TOTAL_BYTES:
                samples = _parse_dump_payload(bytes(buf[4:DUMP_TOTAL_BYTES]))
                if samples is not None:
                    del buf[:DUMP_TOTAL_BYTES]
                    return None, samples, None
                del buf[0]
                continue
            # Dump may be in progress — keep buffering, don't frame-sync yet.
            continue

        while len(buf) >= FRAME_LEN:
            candidate = bytes(buf[:FRAME_LEN])
            if frame_ok(candidate, prev_timer):
                del buf[:FRAME_LEN]
                timer = int.from_bytes(candidate[12:16], "big", signed=False)
                return candidate, None, timer
            del buf[0]


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

    log_path = Path(args.log)
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
            frame, dump_samples, prev_timer = read_aligned_frame(ser, prev_timer)

            if dump_samples is not None:
                print("[DUMP] Valid buffer dump received")
                dump_count += 1
                save_dump(dump_samples, dump_path, dump_count)
                print(
                    f"[DUMP] {len(dump_samples)} samples, "
                    f"peak={max(abs(v) for v in dump_samples)}, "
                    f"min={min(dump_samples)}, max={max(dump_samples)}"
                )
                continue

            b = frame
            sample, energy, flag, timer = parse_frame(b)
            print(
                f"{b[0:4].hex(' ')}  {b[4:8].hex(' ')}  {b[8:12].hex(' ')}  {b[12:16].hex(' ')}  "
                f"sample={sample}  energy={energy}  loud={int(energy > 500)}  "
                f"debounce={flag}  t={timer}"
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

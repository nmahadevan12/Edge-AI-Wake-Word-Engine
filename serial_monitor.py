#!/usr/bin/env python3
"""Hex serial monitor for STM32 binary UART frames.

Firmware frame (16 bytes, big-endian):
  [0:4]    signed int32   mic sample (DC-blocked)
  [4:8]    unsigned uint32 energy
  [8:12]   unsigned uint32 flag (0 or 1)
  [12:16]  unsigned uint32 timer_ms
Only this process opens the serial port.
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
FRAME_LEN = 16
MAX_TIMER_STEP_MS = 500  # max gap between consecutive frames


def parse_frame(b: bytes):
    """Decode one 16-byte frame. Returns (sample, energy, flag, timer)."""
    sample = int.from_bytes(b[0:4], "big", signed=True)
    energy = int.from_bytes(b[4:8], "big", signed=False)
    flag = int.from_bytes(b[8:12], "big", signed=False)
    timer = int.from_bytes(b[12:16], "big", signed=False)
    return sample, energy, flag, timer


def frame_ok(b: bytes, prev_timer: Optional[int]) -> bool:
    """Heuristic: flag must be 0/1 and timer should not jump backwards."""
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
    """Read bytes until a valid frame boundary is found."""
    buf = bytearray()
    while True:
        chunk = ser.read(1)
        if not chunk:
            continue
        buf.extend(chunk)
        while len(buf) >= FRAME_LEN:
            candidate = bytes(buf[:FRAME_LEN])
            if frame_ok(candidate, prev_timer):
                del buf[:FRAME_LEN]
                return candidate, int.from_bytes(candidate[12:16], "big", signed=False)
            del buf[0]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Dump UART: sample + energy + flag + timer (16-byte frame)"
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
        help="CSV log (timer_ms,sample,energy,flag)",
    )
    parser.add_argument(
        "--append",
        action="store_true",
        help="append to the log instead of starting a new capture",
    )
    args = parser.parse_args()

    stop = serial.STOPBITS_TWO if args.stopbits == 2 else serial.STOPBITS_ONE
    ser = serial.Serial(args.port, args.baud, stopbits=stop, timeout=0.5)
    ser.reset_input_buffer()
    log_path = Path(args.log)
    mode = "a" if args.append else "w"
    log = log_path.open(mode, encoding="utf-8")
    if mode == "w":
        log.write("timer_ms,sample,energy,flag\n")
        log.flush()

    print(f"Opened {args.port} @ {args.baud} stopbits={args.stopbits}")
    print(f"Logging {log_path} (16-byte: sample, energy, flag, timer)")
    print("Syncing to frame boundary...")
    print("Ctrl+C to stop\n")

    prev_timer = None
    try:
        while True:
            b, prev_timer = read_aligned_frame(ser, prev_timer)
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

#!/usr/bin/env python3
"""Hex serial monitor for STM32 binary UART frames.

Also writes mic_capture.csv so serial_plot.py can graph history in parallel.
Only this process opens the serial port.
"""

import argparse
import sys
from pathlib import Path

try:
    import serial
except ImportError:
    print("Install pyserial: pip3 install pyserial", file=sys.stderr)
    sys.exit(1)

DEFAULT_LOG = Path(__file__).resolve().parent / "mic_capture.csv"


def pcm24_from_be4(b: bytes) -> int:
    """24-bit signed PCM in a 4-byte BE field.

    Firmware does `RDATAR >> 8` on uint32, so negatives look like 00 FF xx xx
    instead of FF FF xx xx. Sign-extend from bit 23 so the CSV matches audio.
    """
    u = int.from_bytes(b, "big", signed=False) & 0xFFFFFF
    if u & 0x800000:
        return u - 0x1000000
    return u


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Dump UART: signed mic sample BE + timer BE (8-byte frame)"
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
        help="CSV log for serial_plot.py (timer_ms,sample)",
    )
    parser.add_argument(
        "--append",
        action="store_true",
        help="append to the log instead of starting a new capture",
    )
    args = parser.parse_args()

    stop = serial.STOPBITS_TWO if args.stopbits == 2 else serial.STOPBITS_ONE
    ser = serial.Serial(args.port, args.baud, stopbits=stop)
    log_path = Path(args.log)
    mode = "a" if args.append else "w"
    log = log_path.open(mode, encoding="utf-8")
    if mode == "w":
        log.write("timer_ms,sample\n")
        log.flush()

    print(f"Opened {args.port} @ {args.baud} stopbits={args.stopbits}")
    print(f"Logging {log_path}")
    print("Ctrl+C to stop\n")

    try:
        while True:
            b = ser.read(8)
            if len(b) != 8:
                continue
            sample = pcm24_from_be4(b[0:4])
            timer = int.from_bytes(b[4:8], "big", signed=False)
            print(f"{b[0:4].hex(' ')}  {b[4:8].hex(' ')}")
            log.write(f"{timer},{sample}\n")
            log.flush()
    except KeyboardInterrupt:
        print("\nClosed")
    finally:
        log.close()
        ser.close()


if __name__ == "__main__":
    main()

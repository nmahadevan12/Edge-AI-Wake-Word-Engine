#!/usr/bin/env python3
"""Hex serial monitor for STM32 binary UART frames.

Also writes mic_capture.csv so serial_plot.py can graph history in parallel.
Firmware frame (12 bytes, big-endian):
  [0:4]  signed int32   mic sample (DC-blocked)
  [4:8]  unsigned uint32 energy (last finished window)
  [8:12] unsigned uint32 timer_ms
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
FRAME_LEN = 12


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Dump UART: signed sample + uint32 energy + uint32 timer (12-byte frame)"
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
        help="CSV log for serial_plot.py (timer_ms,sample,energy)",
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
        log.write("timer_ms,sample,energy\n")
        log.flush()

    print(f"Opened {args.port} @ {args.baud} stopbits={args.stopbits}")
    print(f"Logging {log_path} (12-byte frames: sample, energy, timer)")
    print("Ctrl+C to stop\n")

    try:
        while True:
            b = ser.read(FRAME_LEN)
            if len(b) != FRAME_LEN:
                continue
            sample = int.from_bytes(b[0:4], "big", signed=True)
            energy = int.from_bytes(b[4:8], "big", signed=False)
            timer = int.from_bytes(b[8:12], "big", signed=False)
            print(
                f"{b[0:4].hex(' ')}  {b[4:8].hex(' ')}  {b[8:12].hex(' ')}  "
                f"sample={sample}  energy={energy}  t={timer}"
            )
            log.write(f"{timer},{sample},{energy}\n")
            log.flush()
    except KeyboardInterrupt:
        print("\nClosed")
    finally:
        log.close()
        ser.close()


if __name__ == "__main__":
    main()

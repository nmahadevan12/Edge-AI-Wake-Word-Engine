#!/usr/bin/env python3
"""Hex serial monitor for STM32 binary UART frames."""

import argparse
import sys

try:
    import serial
except ImportError:
    print("Install pyserial: pip3 install pyserial", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Dump UART: ADC pair BE + timer BE (8-byte frame)"
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
    args = parser.parse_args()

    stop = serial.STOPBITS_TWO if args.stopbits == 2 else serial.STOPBITS_ONE
    ser = serial.Serial(args.port, args.baud, stopbits=stop)
    print(f"Opened {args.port} @ {args.baud} stopbits={args.stopbits}")
    print("Ctrl+C to stop\n")

    try:
        while True:
            # 00 00 A1 A0 | t3 t2 t1 t0
            b = ser.read(8)
            print(f"{b[0:4].hex(' ')}  {b[4:8].hex(' ')}")
    except KeyboardInterrupt:
        print("\nClosed")
    finally:
        ser.close()


if __name__ == "__main__":
    main()

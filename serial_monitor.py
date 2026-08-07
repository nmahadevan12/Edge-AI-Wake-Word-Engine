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
    parser = argparse.ArgumentParser(description="Dump binary UART as hex")
    parser.add_argument(
        "port",
        nargs="?",
        default="/dev/tty.usbserial-FTG8MPCG",
        help="serial port path",
    )
    parser.add_argument("-b", "--baud", type=int, default=115200)
    parser.add_argument(
        "-n",
        "--frame",
        type=int,
        default=8,
        help="bytes per line (default 8 = 4 number + 4 timer)",
    )
    parser.add_argument(
        "--stopbits",
        type=int,
        choices=(1, 2),
        default=2,
        help="stop bits (firmware currently uses 2)",
    )
    parser.add_argument(
        "--decode",
        action="store_true",
        help="decode as uint16 number + uint32 timer (little-endian, 6-byte frame)",
    )
    args = parser.parse_args()

    stop = serial.STOPBITS_TWO if args.stopbits == 2 else serial.STOPBITS_ONE
    ser = serial.Serial(args.port, args.baud, stopbits=stop)
    print(f"Opened {args.port} @ {args.baud} stopbits={args.stopbits}")
    print("Ctrl+C to stop\n")

    try:
        while True:
            if args.decode:
                b = ser.read(6)
                number = int.from_bytes(b[0:2], "little")
                timer = int.from_bytes(b[2:6], "little")
                print(f"{b.hex(' ')}  number={number}  timer_ms={timer}")
            else:
                b = ser.read(args.frame)
                print(b.hex(" "))
    except KeyboardInterrupt:
        print("\nClosed")
    finally:
        ser.close()


if __name__ == "__main__":
    main()

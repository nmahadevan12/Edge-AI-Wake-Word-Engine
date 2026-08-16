#!/usr/bin/env python3
"""Waveform viewer for mic_capture.csv from serial_monitor.py.

Run this in a second terminal while the serial monitor owns the UART port.
Keeps the full capture in memory so you can scrub back through earlier audio.
"""

import argparse
import sys
import time
from collections import deque
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    from matplotlib.widgets import Button, Slider
except ImportError:
    print("Install matplotlib: pip3 install matplotlib", file=sys.stderr)
    sys.exit(1)

DEFAULT_LOG = Path(__file__).resolve().parent / "mic_capture.csv"
FULL_SCALE = 262144  # Sinc3, FOSR 64
U32 = 1 << 32


def u32_to_i32(u: int) -> int:
    """Interpret a uint32 bit pattern as signed int32."""
    u &= 0xFFFFFFFF
    if u >= 0x80000000:
        return u - U32
    return u


def parse_line(line: str):
    line = line.strip()
    if not line or line.startswith("timer_ms"):
        return None
    try:
        a_s, b_s = line.split(",", 1)
        a, b = int(a_s), int(b_s)
    except ValueError:
        return None

    # Normal: timer_ms, signed_sample
    # Older/swapped logs: uint32(sample) ≈ 2^32, timer in the second column
    if a > 0x40000000 and 0 <= b < 10_000_000:
        return b, u32_to_i32(a)
    return a, b


def load_all(path: Path, times: deque, samples: deque) -> int:
    """Load existing log. Returns byte offset to start tailing from."""
    if not path.exists():
        return 0
    offset = 0
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            parsed = parse_line(line)
            if parsed is not None:
                t, s = parsed
                times.append(t)
                samples.append(s)
        offset = f.tell()
    return offset


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Logic-analyzer style plot of logged DFSDM samples"
    )
    parser.add_argument(
        "--log",
        default=str(DEFAULT_LOG),
        help="CSV written by serial_monitor.py",
    )
    parser.add_argument(
        "--history",
        type=int,
        default=20000,
        help="max samples kept (older points are dropped)",
    )
    parser.add_argument(
        "--window",
        type=int,
        default=400,
        help="how many samples are visible at once",
    )
    args = parser.parse_args()

    log_path = Path(args.log)
    times: deque = deque(maxlen=args.history)
    samples: deque = deque(maxlen=args.history)
    follow = True
    offset = 0

    print(f"Waiting for {log_path} (start serial_monitor.py if needed)")
    while not log_path.exists():
        time.sleep(0.2)

    offset = load_all(log_path, times, samples)
    print(f"Loaded {len(samples)} samples. Close the window to quit.")

    plt.rcParams["toolbar"] = "toolbar2"
    fig, ax = plt.subplots(figsize=(12, 5))
    plt.subplots_adjust(bottom=0.22)
    (line,) = ax.plot([], [], lw=0.9, marker=".", ms=3, color="C0")
    ax.axhline(0, color="0.6", lw=0.6)
    ax.set_title("DFSDM mic sample vs firmware timer")
    ax.set_xlabel("timer (ms)")
    ax.set_ylabel("signed int32 PCM")
    ax.set_ylim(-8000, 8000)
    fig.text(
        0.01,
        0.01,
        f"Source: {log_path.name}  ·  drag slider to review history  ·  Follow to resume live",
        fontsize=8,
        color="0.4",
    )

    ax_slider = plt.axes([0.12, 0.10, 0.62, 0.04])
    slider = Slider(ax_slider, "View", 0, 1, valinit=1, valstep=1)

    ax_follow = plt.axes([0.78, 0.09, 0.09, 0.05])
    ax_pause = plt.axes([0.88, 0.09, 0.09, 0.05])
    btn_follow = Button(ax_follow, "Follow")
    btn_pause = Button(ax_pause, "Pause")

    def window_slice():
        n = len(samples)
        if n == 0:
            return [], []
        w = min(args.window, n)
        max_start = max(0, n - w)
        if follow:
            start = max_start
            slider.eventson = False
            slider.valmax = max(1, max_start)
            slider.set_val(start)
            slider.eventson = True
        else:
            start = int(min(slider.val, max_start))
        end = start + w
        return list(times)[start:end], list(samples)[start:end]

    def redraw(_=None):
        x, y = window_slice()
        line.set_data(x, y)
        if x:
            ax.set_xlim(x[0], x[-1] if x[-1] != x[0] else x[0] + 1)
        if y:
            peak = max(abs(min(y)), abs(max(y)), 4000)
            peak = min(peak * 1.3, FULL_SCALE)
            ax.set_ylim(-peak, peak)
        ax.figure.canvas.draw_idle()

    def on_slider(_val):
        nonlocal follow
        follow = False
        redraw()

    def on_follow(_event):
        nonlocal follow
        follow = True
        redraw()

    def on_pause(_event):
        nonlocal follow
        follow = False

    slider.on_changed(on_slider)
    btn_follow.on_clicked(on_follow)
    btn_pause.on_clicked(on_pause)

    def on_timer():
        nonlocal offset
        try:
            with log_path.open("r", encoding="utf-8") as f:
                f.seek(offset)
                extra = f.read()
                offset = f.tell()
        except OSError:
            return
        if not extra:
            return
        for line in extra.splitlines():
            parsed = parse_line(line)
            if parsed is not None:
                t, s = parsed
                times.append(t)
                samples.append(s)
        n = len(samples)
        w = min(args.window, max(n, 1))
        max_start = max(1, n - w)
        slider.valmax = max_start
        slider.ax.set_xlim(0, max_start)
        redraw()

    timer = fig.canvas.new_timer(interval=50)
    timer.add_callback(on_timer)
    timer.start()
    redraw()
    plt.show()


if __name__ == "__main__":
    main()

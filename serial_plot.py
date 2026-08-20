#!/usr/bin/env python3
"""Waveform viewer for mic_capture.csv + buffer_dump_NNN.csv from serial_monitor.py.

CSV: timer_ms,sample,energy,flag
flag column = UART debounce_passed (30 loud samples), not energy>500.
16-byte UART frame: sample, energy, debounce_passed, timer
Buffer dump CSV: index,y  (256 signed int32 samples per event)
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
DEFAULT_DUMP_DIR = Path(__file__).resolve().parent
FULL_SCALE = 262144  # Sinc3, FOSR 64
QUIET_MAX_START_MS = 1000  # ignore warm-up before tracking quiet max
ENERGY_LOUD = 500  # firmware Sound_Detect threshold


def load_dump(path: Path):
    """Load a buffer_dump_NNN.csv → list of y values."""
    ys = []
    try:
        with path.open("r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("index"):
                    continue
                parts = line.split(",")
                if len(parts) >= 2:
                    try:
                        ys.append(int(parts[1]))
                    except ValueError:
                        pass
    except OSError:
        pass
    return ys


def latest_dump(dump_dir: Path):
    """Return the most recently modified buffer_dump_*.csv, or None."""
    dumps = sorted(dump_dir.glob("buffer_dump_*.csv"), key=lambda p: p.stat().st_mtime)
    return dumps[-1] if dumps else None


def parse_line(line: str):
    """Return (timer_ms, sample, energy, flag) or None."""
    line = line.strip()
    if not line or line.startswith("timer_ms"):
        return None
    parts = line.split(",")
    try:
        if len(parts) >= 4:
            return int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])
        if len(parts) == 3:
            return int(parts[0]), int(parts[1]), int(parts[2]), 0
        if len(parts) == 2:
            return int(parts[0]), int(parts[1]), 0, 0
    except ValueError:
        return None
    return None


def load_all(path: Path, times, samples, energies, flags) -> int:
    if not path.exists():
        return 0
    offset = 0
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            parsed = parse_line(line)
            if parsed is not None:
                t, s, e, fl = parsed
                times.append(t)
                samples.append(s)
                energies.append(e)
                flags.append(fl)
        offset = f.tell()
    return offset


def quiet_energy_stats(times, energies, flags):
    """Return (session_max, current_streak_max) for flag == 0 after warm-up."""
    session_max = None
    streak_max = None
    for timer_ms, energy, flag in zip(times, energies, flags):
        if timer_ms <= QUIET_MAX_START_MS:
            streak_max = None
            continue
        if flag == 0:
            session_max = energy if session_max is None else max(session_max, energy)
            streak_max = energy if streak_max is None else max(streak_max, energy)
        else:
            streak_max = None
    return session_max, streak_max


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Overlay DFSDM sample, energy, and detect flag vs timer"
    )
    parser.add_argument("--log", default=str(DEFAULT_LOG))
    parser.add_argument("--dump-dir", default=str(DEFAULT_DUMP_DIR))
    parser.add_argument("--history", type=int, default=20000)
    parser.add_argument("--window", type=int, default=400)
    args = parser.parse_args()
    dump_dir = Path(args.dump_dir)
    session_start = time.time()

    log_path = Path(args.log)
    times = deque(maxlen=args.history)
    samples = deque(maxlen=args.history)
    energies = deque(maxlen=args.history)
    flags = deque(maxlen=args.history)
    follow = True
    offset = 0
    log_inode = None

    print(f"Waiting for {log_path} (start serial_monitor.py if needed)")
    while not log_path.exists():
        time.sleep(0.2)

    def reload_log() -> None:
        nonlocal offset, log_inode
        times.clear()
        samples.clear()
        energies.clear()
        flags.clear()
        offset = load_all(log_path, times, samples, energies, flags)
        try:
            log_inode = log_path.stat().st_ino
        except OSError:
            log_inode = None

    reload_log()
    print(f"Loaded {len(samples)} samples. Close the window to quit.")

    last_dump_path = [None]

    plt.rcParams["toolbar"] = "toolbar2"
    fig = plt.figure(figsize=(14, 8))
    ax_live  = fig.add_subplot(2, 1, 1)  # top: live stream
    ax_dump  = fig.add_subplot(2, 1, 2)  # bottom: latest buffer dump
    plt.subplots_adjust(left=0.08, bottom=0.18, right=0.78, top=0.78, hspace=0.50)

    # aliases for the live plot axes
    ax = ax_live

    ax_e = ax_live.twinx()
    ax_f = ax_live.twinx()
    ax_f.spines["right"].set_position(("outward", 55))

    ax.set_zorder(1)
    ax_e.set_zorder(2)
    ax_f.set_zorder(3)
    ax.patch.set_visible(False)

    (line_pcm,) = ax.plot(
        [], [], lw=0.9, marker=".", ms=3, color="C0", label="sample (PCM)"
    )
    (line_e,) = ax_e.plot(
        [], [], lw=1.0, marker=".", ms=3, color="C1", label="energy"
    )
    (line_f,) = ax_f.plot(
        [], [], lw=1.4, drawstyle="steps-post", color="C2", label="debounce (UART)"
    )
    (line_loud,) = ax_f.plot(
        [],
        [],
        lw=1.2,
        drawstyle="steps-post",
        color="C3",
        alpha=0.85,
        label="loud (energy>500)",
    )
    (line_quiet,) = ax_e.plot(
        [],
        [],
        ls=":",
        lw=1.4,
        color="C4",
        label="quiet max (flag=0)",
    )
    ax_e.axhline(ENERGY_LOUD, color="C1", ls="--", lw=0.8, alpha=0.6)

    ax.axhline(0, color="0.6", lw=0.6)
    ax.set_xlabel("timer (ms)")

    ax.spines["left"].set_color("C0")
    ax.spines["right"].set_visible(False)
    ax_e.spines["left"].set_visible(False)
    ax_e.spines["right"].set_color("C1")
    ax_f.spines["left"].set_visible(False)
    ax_f.spines["right"].set_color("C2")

    ax.set_ylabel("signed int32 PCM", color="C0")
    ax_e.set_ylabel("energy", color="C1")
    ax_f.set_ylabel("loud / debounce (0/1)", color="C2")
    ax.tick_params(axis="y", labelcolor="C0", colors="C0")
    ax_e.tick_params(axis="y", labelcolor="C1", colors="C1")
    ax_f.tick_params(axis="y", labelcolor="C2", colors="C2")
    ax_f.set_ylim(-0.1, 1.1)
    ax_f.set_yticks([0, 1])
    ax_e.yaxis.label.set_color("C1")
    ax_f.yaxis.label.set_color("C2")

    lines = [line_pcm, line_e, line_loud, line_f, line_quiet]
    fig.legend(
        lines,
        [ln.get_label() for ln in lines],
        loc="lower left",
        bbox_to_anchor=(0.08, 0.79),
        ncol=5,
        fontsize=8,
        frameon=True,
        borderaxespad=0,
    )

    quiet_text = fig.text(
        0.12,
        0.105,
        "",
        fontsize=8,
        color="C4",
        ha="left",
        va="bottom",
        bbox={"boxstyle": "round,pad=0.3", "facecolor": "white", "alpha": 0.85},
    )

    live_text = fig.text(
        0.08,
        0.998,
        "sample: —   energy: —",
        fontsize=10,
        color="0.15",
        ha="left",
        va="top",
        bbox={"boxstyle": "round,pad=0.35", "facecolor": "white", "alpha": 0.9},
    )

    # --- dump subplot ---
    ax_dump.set_title("Latest buffer dump (sample_buffer[256])", fontsize=9)
    ax_dump.set_xlabel("sample index")
    ax_dump.set_ylabel("y (signed int32)", color="C0")
    ax_dump.axhline(0, color="0.6", lw=0.6)
    ax_dump.tick_params(axis="y", labelcolor="C0")
    (line_dump,) = ax_dump.plot([], [], lw=0.9, marker=".", ms=2, color="C0")
    dump_label = ax_dump.text(
        0.99, 0.97, "No dump yet",
        transform=ax_dump.transAxes,
        fontsize=8, color="0.5", ha="right", va="top",
    )

    fig.text(
        0.01,
        0.01,
        f"Source: {log_path.name}  ·  16-byte: sample, energy, debounce, timer  ·  orange dashed = energy {ENERGY_LOUD}",
        fontsize=8,
        color="0.4",
    )

    ax_slider = plt.axes([0.12, 0.06, 0.62, 0.03])
    slider = Slider(ax_slider, "View", 0, 1, valinit=1, valstep=1)
    ax_follow = plt.axes([0.78, 0.05, 0.09, 0.04])
    ax_pause = plt.axes([0.88, 0.05, 0.09, 0.04])
    ax_follow.set_zorder(20)
    ax_pause.set_zorder(20)
    btn_follow = Button(ax_follow, "Follow")
    btn_pause = Button(ax_pause, "Pause")

    def max_slider_start():
        n = len(samples)
        w = min(args.window, max(n, 1))
        return max(0, n - w)

    def set_slider_range(value=None):
        """Update slider span without treating it as a user scrub (no follow-off)."""
        max_start = max(1, max_slider_start())
        cur = 0 if value is None else int(value)
        cur = min(max(cur, 0), max_start)
        slider.eventson = False
        slider.valmax = max_start
        slider.ax.set_xlim(slider.valmin, max_start)
        slider.set_val(cur)
        slider.eventson = True

    def style_buttons():
        on = {"color": "#4caf50", "hovercolor": "#66bb6a"}
        off = {"color": "0.85", "hovercolor": "0.75"}
        paused = {"color": "#ef6c00", "hovercolor": "#ff9800"}
        idle = {"color": "0.85", "hovercolor": "0.75"}
        if follow:
            btn_follow.color = on["color"]
            btn_follow.hovercolor = on["hovercolor"]
            btn_pause.color = idle["color"]
            btn_pause.hovercolor = idle["hovercolor"]
            btn_follow.ax.set_facecolor(on["color"])
            btn_pause.ax.set_facecolor(idle["color"])
        else:
            btn_follow.color = off["color"]
            btn_follow.hovercolor = off["hovercolor"]
            btn_pause.color = paused["color"]
            btn_pause.hovercolor = paused["hovercolor"]
            btn_follow.ax.set_facecolor(off["color"])
            btn_pause.ax.set_facecolor(paused["color"])
        fig.canvas.draw_idle()

    def window_slice():
        n = len(samples)
        if n == 0:
            return [], [], [], []
        w = min(args.window, n)
        max_start = max(0, n - w)
        if follow:
            start = max_start
            set_slider_range(start)
        else:
            start = int(min(slider.val, max_start))
        end = start + w
        return (
            list(times)[start:end],
            list(samples)[start:end],
            list(energies)[start:end],
            list(flags)[start:end],
        )

    def redraw(_=None):
        x, y, e, fl = window_slice()
        session_max, streak_max = quiet_energy_stats(
            list(times), list(energies), list(flags)
        )

        line_pcm.set_data(x, y)
        line_e.set_data(x, e)
        line_f.set_data(x, fl)
        line_loud.set_data(x, [1 if v > ENERGY_LOUD else 0 for v in e])
        if x and session_max is not None:
            line_quiet.set_data([x[0], x[-1]], [session_max, session_max])
            line_quiet.set_visible(True)
        else:
            line_quiet.set_visible(False)

        if session_max is None:
            quiet_text.set_text(f"Quiet max (t>{QUIET_MAX_START_MS} ms): —")
        else:
            streak_txt = "—" if streak_max is None else str(streak_max)
            margin = max(ENERGY_LOUD - session_max, 0)
            quiet_text.set_text(
                f"Quiet max: {session_max}   streak: {streak_txt}   "
                f"threshold: {ENERGY_LOUD}   headroom: {margin}"
            )

        if y:
            live_text.set_text(f"sample: {y[-1]}   energy: {e[-1]}")
        else:
            live_text.set_text("sample: —   energy: —")

        if x:
            ax.set_xlim(x[0], x[-1] if x[-1] != x[0] else x[0] + 1)
        if y:
            peak = max(abs(min(y)), abs(max(y)), 4000)
            peak = min(peak * 1.3, FULL_SCALE)
            ax.set_ylim(-peak, peak)
        if e or session_max is not None:
            top = max(max(e) if e else 0, session_max or 0, ENERGY_LOUD, 1000)
            ax_e.set_ylim(0, top * 1.3)
        ax_f.set_ylim(-0.1, 1.1)
        fig.canvas.draw_idle()

    def on_slider(_val):
        nonlocal follow
        follow = False
        style_buttons()
        redraw()

    def on_follow(_event):
        nonlocal follow
        follow = True
        style_buttons()
        redraw()

    def on_pause(_event):
        nonlocal follow
        follow = False
        style_buttons()
        redraw()

    slider.on_changed(on_slider)
    btn_follow.on_clicked(on_follow)
    btn_pause.on_clicked(on_pause)

    def on_button_axes(event):
        # Timer redraws steal matplotlib Button clicks on macOS; hit the axes directly.
        if event.inaxes is ax_follow:
            on_follow(event)
        elif event.inaxes is ax_pause:
            on_pause(event)

    fig.canvas.mpl_connect("button_press_event", on_button_axes)

    def refresh_dump():
        """Check for a new buffer dump and update the bottom subplot."""
        dp = latest_dump(dump_dir)
        if dp is None or dp == last_dump_path[0]:
            return False
        try:
            if dp.stat().st_mtime < session_start:
                return False
        except OSError:
            return False
        last_dump_path[0] = dp
        ys = load_dump(dp)
        if not ys:
            return False
        xs = list(range(len(ys)))
        line_dump.set_data(xs, ys)
        peak = max(abs(min(ys)), abs(max(ys)), 1)
        ax_dump.set_xlim(0, len(ys) - 1)
        ax_dump.set_ylim(-peak * 1.3, peak * 1.3)
        dump_label.set_text(
            f"{dp.name}  ·  {len(ys)} samples  ·  peak={peak}  min={min(ys)}  max={max(ys)}"
        )
        dump_label.set_color("C0")
        return True

    def on_timer():
        nonlocal offset, log_inode
        try:
            st = log_path.stat()
        except OSError:
            return
        if log_inode is None or st.st_ino != log_inode or st.st_size < offset:
            reload_log()
            redraw()
            return
        try:
            with log_path.open("r", encoding="utf-8") as f:
                f.seek(offset)
                extra = f.read()
                offset = f.tell()
        except OSError:
            return
        if extra:
            for line in extra.splitlines():
                parsed = parse_line(line)
                if parsed is not None:
                    t, s, e, fl = parsed
                    times.append(t)
                    samples.append(s)
                    energies.append(e)
                    flags.append(fl)
        dumped = refresh_dump()
        if follow:
            if extra or dumped:
                redraw()
        else:
            set_slider_range(slider.val)
            if dumped:
                fig.canvas.draw_idle()

    timer = fig.canvas.new_timer(interval=50)
    timer.add_callback(on_timer)
    timer.start()
    style_buttons()
    redraw()
    plt.show()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Record Arduino CSV output and plot saved experiments with Matplotlib."""

import argparse
import csv
from datetime import datetime
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import webbrowser

PROJECT = Path(__file__).resolve().parents[1]
COLUMNS = ["time_ms", "theta_ref", "theta", "control_voltage", "motor_voltage"]
LABELS = {
    "time_s": "Time from first sample (s)", "time_ms": "Arduino time (ms)",
    "theta_ref": "Reference angle (rad)", "theta": "Measured angle (rad)",
    "error": "Tracking error (rad)", "control_voltage": "Control voltage (V)",
    "motor_voltage": "Requested motor voltage (V)",
}


def numeric_row(line, columns):
    try:
        values = [float(value) for value in line.strip().split(",")]
    except ValueError:
        return None
    if len(values) != len(columns) or not all(math.isfinite(v) for v in values):
        return None
    return values


def load_csv(path):
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        names = reader.fieldnames
        if not names or len(set(names)) != len(names):
            raise ValueError("CSV needs unique column headers.")
        data = {name: [] for name in names}
        for line_number, row in enumerate(reader, 2):
            if None in row or any(value is None for value in row.values()):
                raise ValueError(f"Incomplete CSV row at line {line_number}.")
            values = numeric_row(",".join(row[name] for name in names), names)
            if values is None:
                raise ValueError(f"Non-numeric CSV row at line {line_number}.")
            for name, value in zip(names, values):
                data[name].append(value)
    if not data[names[0]]:
        raise ValueError("No numeric samples recorded; no plot generated.")
    if "time_ms" in data:
        ticks = data["time_ms"]
        if any(b < a for a, b in zip(ticks, ticks[1:])):
            raise ValueError("Arduino time moved backwards. Split recordings at board resets before plotting.")
        data.setdefault("time_s", [(value - ticks[0]) / 1000 for value in ticks])
    if "theta_ref" in data and "theta" in data:
        data.setdefault("error", [ref - angle for ref, angle in zip(data["theta_ref"], data["theta"])])
    return data


def interactive_plot(data, x, columns, selected, title, ylabel):
    """Build a browser plot whose hover values are recorded samples."""
    import plotly.graph_objects as go

    figure = go.Figure()
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#9467bd"]
    xvalues = [data[x][i] for i in selected]
    for index, name in enumerate(columns):
        values = [data[name][i] for i in selected]
        label = LABELS.get(name, name)
        color = colors[index % len(colors)]
        tooltip = (LABELS.get(x, x) + ": %{x:.5f}<br>" + label
                   + ": %{y:.5f}<extra></extra>")
        figure.add_trace(go.Scatter(
            x=xvalues, y=values, name=label, mode="lines", legendgroup=name,
            line=dict(color=color, shape="hv" if name == "theta_ref" else "linear"),
            hovertemplate=tooltip,
        ))
        # Mark sampled extrema in the exported range, not inferred step-response peaks.
        if name != "theta_ref":
            for kind, point, symbol in (
                ("Maximum", max(range(len(values)), key=values.__getitem__), "triangle-up"),
                ("Minimum", min(range(len(values)), key=values.__getitem__), "triangle-down"),
            ):
                figure.add_trace(go.Scatter(
                    x=[xvalues[point]], y=[values[point]], mode="markers",
                    name=f"{label}: {kind.lower()}", legendgroup=name, showlegend=False,
                    marker=dict(size=12, symbol=symbol, color=color, line=dict(width=1, color="white")),
                    hovertemplate=kind + " in exported range<br>" + tooltip,
                ))
    if set(columns) <= {"control_voltage", "motor_voltage"}:
        for limit in (-6, 6):
            figure.add_hline(y=limit, line_dash="dot", line_color="red")
    figure.update_layout(
        title=dict(text=title, subtitle=dict(text="Hover for sample values · triangles mark sampled min/max · drag to zoom · double-click to reset")),
        xaxis_title=LABELS.get(x, x), yaxis_title=ylabel, template="plotly_white",
        hovermode="closest", dragmode="zoom", height=650,
        margin=dict(t=110, b=80), legend=dict(orientation="h", y=-0.2),
    )
    figure.update_xaxes(showspikes=True, spikemode="across", spikesnap="cursor")
    if x == "time_s":
        figure.update_xaxes(tickformat=".3f", title_text="Time from first sample (s)")
        if max(xvalues) - min(xvalues) <= 5:
            figure.update_xaxes(dtick=0.25)
    return figure


def plot_csv(path, args):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    data = load_csv(path)
    x = args.x or ("time_s" if "time_s" in data else next(iter(data)))
    if args.y:
        groups = [args.y]
    elif "theta_ref" in data and "theta" in data:
        groups = [["theta_ref", "theta"]]
    else:
        groups = [[name for name in data if name != x]]
    requested = [x] + [name for group in groups for name in group]
    unknown = set(requested) - data.keys()
    if unknown or not groups[0]:
        raise ValueError(f"Choose columns from: {', '.join(data)}. Unknown: {', '.join(sorted(unknown))}")
    selected = [i for i, value in enumerate(data[x])
                if (args.start is None or value >= args.start)
                and (args.end is None or value <= args.end)]
    if not selected:
        raise ValueError("No samples in the selected range.")
    fig, axes = plt.subplots(len(groups), 1, figsize=(14, 4.5 * len(groups)),
                             sharex=True, squeeze=False, layout="constrained")
    fig.suptitle(args.title or path.stem)
    for axis, group in zip(axes[:, 0], groups):
        for name in group:
            axis.plot([data[x][i] for i in selected], [data[name][i] for i in selected],
                      label=LABELS.get(name, name), linewidth=1.2,
                      drawstyle="steps-post" if name == "theta_ref" else "default")
        if set(group) <= {"theta", "theta_ref", "error"}:
            axis.set_ylabel("Angle (rad)")
        elif set(group) <= {"control_voltage", "motor_voltage"}:
            axis.set_ylabel("Voltage (V)")
            axis.axhline(6, color="red", linestyle=":", label="Driver limits (±6 V)")
            axis.axhline(-6, color="red", linestyle=":")
        else:
            axis.set_ylabel("Value (units in legend)")
        axis.grid(True, alpha=0.25)
        axis.legend(loc="best")
    axes[-1, 0].set_xlabel(LABELS.get(x, x))
    if x == "time_s":
        from matplotlib.ticker import FormatStrFormatter, MultipleLocator
        axes[-1, 0].xaxis.set_major_formatter(FormatStrFormatter("%.3f"))
        if max(data[x][i] for i in selected) - min(data[x][i] for i in selected) <= 5:
            axes[-1, 0].xaxis.set_major_locator(MultipleLocator(0.25))
            axes[-1, 0].xaxis.set_minor_locator(MultipleLocator(0.05))
        axes[-1, 0].margins(x=0.01)
    output = args.output or path.with_suffix(".png")
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=200)
    print(f"Plot: {output}")
    plt.close(fig)
    interactive = interactive_plot(data, x, groups[0], selected,
                                   args.title or path.stem, axes[0, 0].get_ylabel())
    html_output = output.with_suffix(".html")
    interactive.write_html(html_output, include_plotlyjs=True, full_html=True,
                           config={"scrollZoom": True, "displaylogo": False})
    print(f"Interactive plot: {html_output}")
    if "motor_voltage" in data:
        saturated = sum(abs(v) >= 6 for v in data["motor_voltage"])
        if saturated:
            print(f"Part E check: {saturated} samples request |motor voltage| >= 6 V.")
    if "time_s" in data and len(data["time_s"]) > 1:
        intervals = [b - a for a, b in zip(data["time_s"], data["time_s"][1:])]
        mean = sum(intervals) / len(intervals)
        if mean > 0:
            print(f"Recorded rate: {1 / mean:.1f} Hz; largest timestamp gap: {max(intervals)*1000:.1f} ms")
    if args.show:
        webbrowser.open(html_output.resolve().as_uri())


def platformio():
    executable = shutil.which("pio") or shutil.which("platformio")
    if executable:
        return executable
    for suffix in ("bin/platformio", "Scripts/platformio.exe"):
        candidate = Path.home() / ".platformio/penv" / suffix
        if candidate.is_file():
            return str(candidate)
    raise ValueError("PlatformIO not found. Run from the PlatformIO terminal or install PlatformIO Core.")


def capture(args):
    import serial
    from serial.tools import list_ports

    if args.seconds <= 0 or not math.isfinite(args.seconds):
        raise ValueError("--seconds must be positive and finite.")
    columns = args.columns
    if len(columns) != len(set(columns)) or any(not name.strip() for name in columns):
        raise ValueError("Column names must be unique and nonempty.")
    if args.upload:
        command = [platformio(), "run", "-d", str(PROJECT), "-e", args.env, "-t", "upload"]
        if args.port:
            command += ["--upload-port", args.port]
        subprocess.run(command, check=True)

    # USB serial devices can disappear briefly after uploading.
    connection = None
    deadline = time.monotonic() + 10
    reason = "No USB serial device found; connect the board or specify --port."
    while time.monotonic() < deadline:
        devices = [device.device for device in list_ports.comports() if device.vid is not None]
        if not args.port and len(devices) > 1:
            raise ValueError(f"Multiple devices: {', '.join(devices)}. Select one with --port.")
        port = args.port or (devices[0] if devices else None)
        if port:
            try:
                connection = serial.Serial(port, args.baud, timeout=0.2)
                break
            except serial.SerialException as error:
                reason = str(error)
        time.sleep(0.2)
    if connection is None:
        raise ValueError(f"Cannot open serial port: {reason}. Close other Serial Monitors.")

    label = re.sub(r"[^A-Za-z0-9_.-]+", "_", args.name).strip("._") or "run"
    path = PROJECT / "data" / label / f"{datetime.now():%Y%m%d_%H%M%S_%f}_{label}.csv"
    count, skipped, failure = 0, 0, None
    started = None
    previous_time = None
    pending = b""
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        with connection, path.open("x", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(columns)
            handle.flush()
            print(f"Recording {connection.port} at {args.baud} baud → {path}")
            print(f"Capturing {args.seconds:g} seconds from first sample. Ctrl+C saves early.")
            waiting_since = time.monotonic()
            try:
                while started is None or time.monotonic() - started < args.seconds:
                    if started is None and time.monotonic() - waiting_since > 15:
                        raise ValueError("No valid samples in 15 seconds. Check baud rate and --columns.")
                    pending += connection.read_until(b"\n")
                    if not pending.endswith(b"\n"):
                        if len(pending) > 16384:
                            raise ValueError("Serial data has no line endings.")
                        continue
                    values = numeric_row(pending.decode("utf-8", errors="replace"), columns)
                    pending = b""
                    if values is None:
                        skipped += 1
                        continue
                    if "time_ms" in columns:
                        timestamp = values[columns.index("time_ms")]
                        if previous_time is not None and timestamp < previous_time:
                            raise ValueError("Board timestamp restarted; saved samples before the reset.")
                        previous_time = timestamp
                    if started is None:
                        started = time.monotonic()
                    writer.writerow(values)
                    handle.flush()
                    count += 1
            except KeyboardInterrupt:
                print("\nRecording stopped.")
            except (serial.SerialException, ValueError) as error:
                failure = str(error)
    finally:
        connection.close()
    print(f"Saved {count} samples; skipped {skipped} non-data lines. CSV: {path}")
    if count and not args.no_plot:
        plot_csv(path, args)
    if failure or not count:
        raise ValueError(failure or "No samples recorded.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    record = commands.add_parser("record", help="Record a timestamped CSV, then plot it")
    record.add_argument("--upload", action="store_true", help="Upload through PlatformIO first")
    record.add_argument("--env", default="uno_r4_minima")
    record.add_argument("--port", help="Serial port; auto-detects a single USB serial device")
    record.add_argument("--baud", type=int, default=115200)
    record.add_argument("--seconds", type=float, default=5, help="Recording duration (default: 5 seconds)")
    record.add_argument("--name", default="lab1e", help="Experiment label, e.g. lab1e_Kp_minus25")
    record.add_argument("--columns", nargs="+", default=COLUMNS, help="Names in serial output order")
    record.add_argument("--no-plot", action="store_true", help="Save only CSV")
    plot = commands.add_parser("plot", help="Plot any numeric columns from an existing CSV")
    plot.add_argument("csv", type=Path)
    for command in (record, plot):
        command.add_argument("--x", help="X column (default: time_s if available)")
        command.add_argument("--y", nargs="+", help="Y columns to overlay (default: reference and measured angle)")
        command.add_argument("--start", type=float, help="Minimum X value")
        command.add_argument("--end", type=float, help="Maximum X value")
        command.add_argument("--title")
        command.add_argument("--output", type=Path, help="Plot filename, e.g. angle.png or angle.pdf")
        command.add_argument("--show", action="store_true", help="Open the saved interactive HTML plot in your browser")
    args = parser.parse_args()
    try:
        if args.command == "record":
            capture(args)
        else:
            plot_csv(args.csv, args)
    except ImportError as error:
        parser.exit(1, f"Missing dependency: {error}. Install requirements.txt using your Python environment.\n")
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()

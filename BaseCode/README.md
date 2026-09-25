# Lab data: CSV recording and Matplotlib plots

PlatformIO builds `Code/Code.ino` (`src_dir = Code`), not the top-level `Code.ino`.
The current Lab 1 Part E sketch sends five comma-separated values at 115200 baud:

```text
time_ms,theta_ref,theta,control_voltage,motor_voltage
```

Angles are in radians. Voltages are in volts. `control_voltage` is before stiction
compensation; `motor_voltage` is the requested voltage after compensation and
before the driver's ±6 V clipping. It is not a measured voltage.

## One-time setup

From the BaseCode terminal:

```sh
python3 -m pip install -r requirements.txt
```

This installs the packages for your regular `python3`; no activation is needed.

## Run an experiment

Close any other Serial Monitor. Set `Kp` in `Code/Code.ino`, save it, then run:

```sh
python3 scripts/lab_data.py record --upload --name lab1e_Kp_minus25 --show
```

This uploads, connects to the serial port, records for 5 seconds from the first
valid sample, and saves a CSV, PNG, and interactive HTML graph under `data/<name>/` with unique
timestamped filenames. For example, `--name lab1e_Kp_minus25` produces:

```text
data/lab1e_Kp_minus25/
  <timestamp>_lab1e_Kp_minus25.csv
  <timestamp>_lab1e_Kp_minus25.png
  <timestamp>_lab1e_Kp_minus25.html
```

Repeated recordings with the same name go into the same folder, with distinct timestamps.
Five seconds is the default; use `--seconds` to override it. Time axes show three
decimal places, with 0.25 s ticks for recordings up to 5 s. The static plot is
wider for easier reading; zoom the HTML graph to inspect individual samples.
Extra displayed decimal places do not increase the sensor or sampling resolution.
The label is your record of the experiment; it does not change or verify `Kp`.
The default graph shows reference and measured angle. Matplotlib generates the
PNG for reports; Plotly generates an interactive HTML file you can reopen in a
browser, even offline. Hover over a curve for its recorded X/Y values. Triangles
mark each measured series' sampled maximum and minimum within the exported range
(the first occurrence if tied). These markers are not automatic measurements of
the first peak after each step. Drag to zoom, scroll to zoom, double-click to reset,
or click a legend entry to hide/show that series.
Startup messages and malformed lines are skipped; valid CSV rows are flushed to
disk immediately. Ctrl+C ends recording early and generates the plot. A serial
disconnect preserves and plots existing data, then reports an error.

Omit `--upload` to record an already running sketch. Add `--no-plot` for CSV only.
Add `--show` to open the interactive HTML graph in your browser. If multiple boards
are connected, specify `--port /dev/cu.usbmodem...` (or `COM3` on Windows).
The upload may change the board's port; if an explicitly selected port changes,
run `record` again with the new port and without `--upload`.

Recording starts when the serial port opens, so the first moments after boot may
be missed. For Part E the repeated square wave supplies later complete steps.
Stopping the recording does not stop the controller on the Arduino.

## Plot whichever columns you need

Replace `data/YOUR_NAME/YOUR_RUN.csv` with the saved filename and `YOUR_NAME`
in the output paths with your experiment name:

```sh
# Reference and measured angle, zoomed into seconds 2 through 4
python3 scripts/lab_data.py plot data/YOUR_NAME/YOUR_RUN.csv --y theta_ref theta --start 2 --end 4 --show --output data/YOUR_NAME/angle_detail.png

# Tracking error (computed as theta_ref - theta)
python3 scripts/lab_data.py plot data/YOUR_NAME/YOUR_RUN.csv --y error --output data/YOUR_NAME/error.png

# Requested motor voltage
python3 scripts/lab_data.py plot data/YOUR_NAME/YOUR_RUN.csv --y motor_voltage --output data/YOUR_NAME/voltage.png

# Select any X and Y columns; export a PDF for a report
python3 scripts/lab_data.py plot data/YOUR_NAME/YOUR_RUN.csv --x theta --y control_voltage --output data/YOUR_NAME/voltage_vs_angle.pdf
```

`time_s` is computed from Arduino timestamps relative to the first saved sample.
`--start` and `--end` are in the selected X column's units. An existing plot at the
chosen output path is replaced; the CSV is never modified by plotting.
Each plot also saves an `.html` file alongside the static image, using the same
basename. The HTML remains interactive when reopened; the PNG/PDF is static.

For later labs, use `record --columns name1 name2 ...` to match the numeric fields
the sketch prints, and `--y` to choose what to plot. Any headered numeric CSV can
be plotted. Fields absent from the serial output cannot be recovered by the logger.

## Lab 1 Part E checks

The lab manual, pages 20–22, asks for three datasets with different proportional
gains and the same small step size. Keep separate labelled recordings for each.
The current reference alternates between -0.1 and +0.1 rad every second.

Use the angle plot to inspect overshoot, time to the first peak, steady-state
error, and the quality of stiction compensation. Voltage remains in the CSV, and
the script flags requests at or beyond ±6 V. If needed, explicitly select
`--y motor_voltage` to plot it with the ±6 V driver limits.

The sketch currently requests a 5 ms interval (200 Hz). The script reports the observed
rate and largest gap from saved timestamps; these do not prove there were no
missed samples. Check whether the first peak is resolved well enough and document
your choice of sampling rate as the manual requires. The script does not infer
overshoot, peak time, K1, or tau automatically.

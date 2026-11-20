# Lab 1 Part E: identification using Kp = -25

Source: [20260925_123048_771632_lab1e_Kp_minus25_c5ms.csv](20260925_123048_771632_lab1e_Kp_minus25_c5ms.csv). Manual: pages 20-22 of MTE_484_F26_Lab_Manual (1).pdf.

**Estimate: K1 = -2.114507 rad/(V s), tau = 0.02164084 s.**

These are arithmetic means of the five separately calculated parameter estimates. No averaging, smoothing, filtering, interpolation, or resampling was applied to the recorded response samples. The source CSV and graphs are unchanged.

## Method

The plant is G(s) = K1 / [s (tau s + 1)]. With proportional gain Kp and unity negative feedback, the closed-loop denominator is s^2 + (1/tau)s + Kp K1/tau.

Comparing with s^2 + 2 zeta omega_n s + omega_n^2 gives:

- tau = 1 / (2 zeta omega_n)
- K1 = tau omega_n^2 / Kp

Each reference transition is from -0.1 to +0.1 rad or vice versa: the step magnitude is **0.2 rad**, not 0.1 rad. The reference levels are used as the nominal settled values; the late portions of the five steps have mean angles within 0.00056 rad of those targets (checked only to assess this assumption).

For each interval between reference transitions, take the largest excursion in the direction of the step. Inspection of the graph and neighboring raw samples confirms these are the primary overshoot lobes, not later noise spikes. Tp is measured from the first CSV sample at the new reference to the selected raw peak sample. The final interval ends with the recording, after its peak and settling region.

- Mp = abs(theta_peak - theta_final) / 0.2 (fractional overshoot)
- zeta = -ln(Mp) / sqrt(pi^2 + ln(Mp)^2)
- omega_n = pi / [Tp sqrt(1 - zeta^2)]

## Per-step results

Absolute Arduino timestamps are shown below, in milliseconds since boot.

| Step | Direction | Reference change (ms) | Peak (ms) | Peak angle (rad) | Overshoot (%) | Tp (s) | zeta | omega_n (rad/s) | K1 (rad/(V s)) | tau (s) |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | Rising | 1000 | 1070 | 0.1324 | 16.20 | 0.070 | 0.50131 | 51.86832 | -2.069299 | 0.01922909 |
| 2 | Falling | 2000 | 2075 | -0.1404 | 20.20 | 0.075 | 0.45371 | 47.00443 | -2.071991 | 0.02344501 |
| 3 | Rising | 3000 | 3065 | 0.1371 | 18.55 | 0.065 | 0.47259 | 54.84312 | -2.320946 | 0.01929126 |
| 4 | Falling | 4000 | 4075 | -0.1397 | 19.85 | 0.075 | 0.45764 | 47.11062 | -2.058865 | 0.02319158 |
| 5 | Rising | 5000 | 5075 | 0.1393 | 19.65 | 0.075 | 0.45990 | 47.17256 | -2.051436 | 0.02304724 |

## Worked example: first rising step

The reference changes at 1000 ms and the sampled peak is 0.1324 rad at 1070 ms.

- Tp = (1070 - 1000)/1000 = 0.070 s
- Mp = (0.1324 - 0.1000)/0.2000 = 0.162 = 16.2% overshoot
- zeta = 0.501313
- omega_n = 51.868324 rad/s
- tau = 0.01922909 s
- K1 = -2.069299 rad/(V s)

## Range and data checks

- Manual requires abs(K1) = 1.50 to 3.00 rad/(V s). Mean abs(K1) = 2.114507: within range.
- Manual requires tau = 0.010 to 0.035 s. Mean tau = 0.02164084: within range.
- All five individual K1 and tau estimates are also within range.
- K1 is negative because Kp is negative and stable negative feedback requires Kp K1 > 0 for this model.
- 1001 samples over 5.000 seconds; every adjacent timestamp is 5 ms apart (200 Hz).
- Largest absolute requested motor voltage: 5.48 V, below the manual's 6 V limit.
- The recorded control_voltage values agree with -25*(theta_ref - theta) to within 0.005 V, consistent with serial rounding. The currently open sketch has a different gain, so these calculations use the recorded -25 run.

## Interpretation and limitations

A representative model from this dataset is G(s) = -2.114507 / [s (0.02164084 s + 1)].

This is the manual's approximate continuous-time second-order identification method. Five-millisecond sampling, angle noise, stiction compensation, and discrete control affect the estimate; the extra calculation digits do not imply matching experimental precision. Peaks are selected from raw samples and can be affected by noise. Each peak occurs 13-15 sample intervals after the reference transition.

The mean summarizes repeated steps at ONE gain. It does not replace the manual's requirement to compare datasets obtained with three different proportional gains.

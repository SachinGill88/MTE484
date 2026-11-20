# Lab 3 Part C: outer-loop controller with integral action

## Proper pole-placement redesign

The original `0.1/s` addition below was an exploratory hardware candidate, not a
pole-placement design. The new
[MATLAB pole-placement script](../scripts/lab3_part_c_pole_placement.m) designs a
replacement by characteristic-polynomial coefficient matching. It does not change
the Arduino sketch.

Using the fast-inner-loop approximation, let `b = K2*K3` and `Papprox = b/s^2`.
Choose an integral controller of the form:

```text
C2(s) = (n2*s^2 + n1*s + n0)/(s*(s+p))
```

The approximate closed-loop characteristic polynomial is:

```text
s^4 + p*s^3 + b*n2*s^2 + b*n1*s + b*n0
```

For a desired monic polynomial `[1 a3 a2 a1 a0]`, exact matching gives
`p=a3`, `n2=a2/b`, `n1=a1/b`, and `n0=a0/b`. The script states the chosen four
poles, explains the initial damping/speed guidelines and slow integral mode,
then checks the actual response including all modes and controller zeros.
The chosen pair is not assumed dominant because a slower real pole is included.

The script uses the provisional `K3=-4.28`. Confirm the final Lab 2 value before
adopting the design. It verifies the continuous full model, Tustin discretization,
and sampled full model with reference saturation and conditional integration.
It reports per-step settling and overshoot instead of assuming that pole locations
alone guarantee specifications. The MATLAB script has not been executed in MATLAB
here; the design was numerically checked separately with Python/SciPy.

The sections below document the earlier experimental integrator variant.

## Purpose and scope

The Part B experiment followed changes in ball-position reference but retained a
large position error. The original outer controller has finite DC gain and no
integrator:

```text
C2LD(s) = -7(s + 0.35)/(s + 2.5)
C2LD(0) = -0.98
```

A nonzero position error is therefore needed to generate a nonzero steady gear-angle
reference. On the real apparatus, beam-level offsets, rolling resistance, and
inner-loop errors can require such a reference. Integral action was added so that
the outer controller can retain a corrective gear-angle command even when position
error approaches zero. It does not correct an incorrect position calibration.

This is a candidate controller for the Part C redesign. It is not a claim that all
Part C simulation requirements or later experimental requirements have been met.

## Controller change

The original lead controller was retained and a parallel integral term was added:

```text
C2new(s) = -7(s + 0.35)/(s + 2.5) - 0.1/s
        = (-7s^2 - 2.55s - 0.25)/(s^2 + 2.5s)
```

The negative integral gain follows the apparatus sign convention: positive
ball-position error requires a negative corrective gear-angle command.
`OUTER_KI = 0.1` is an initial, slow integral-gain choice, not a value obtained from
a completed analytical pole-placement design.

The inner loop is much faster than the ball-position loop, so a unity-gain inner
closed loop is a useful first approximation for outer-loop design. Under that
approximation, the outer plant is `K2*K3/s^2`, with `K2 = 0.0609`. Final verification
must include the complete inner loop and motor model.

The sampled linear model was checked before implementation. With the identified
motor, existing discrete inner controller, and provisional `K3 = -4.28`, the
largest closed-loop pole magnitude was approximately 0.999274, inside the unit
circle. Stability was also checked at the preliminary K3 extremes of -1.63 and
-7.33. These checks establish nominal linear stability only; they do not establish
settling time, overshoot, robustness, or nonlinear hardware performance.

## Discrete implementation

Both loops execute every `Ts = 0.005 s` (200 Hz). For ball-position error
`e[k] = ball_ref[k] - ballPosition[k]`, the Tustin-discretized lead branch is:

```text
L[k] = 0.987577639751553 L[k-1]
     - 6.962608695652174 e[k]
     + 6.950434782608695 e[k-1]
```

The integral branch uses the trapezoidal rule, equivalent to Tustin integration:

```text
I[k] = I[k-1] - Ki*Ts/2 * (e[k] + e[k-1])
     = I[k-1] - 0.00025 * (e[k] + e[k-1])

theta_ref[k] = L[k] + I[k]
theta_ref_sat[k] = constrain(theta_ref[k], -0.7, 0.7)
```

`previousThetaRef` stores the previous lead-branch output `L`, not the combined or
saturated reference. `integralThetaRef` stores `I`. This prevents the integrator
and saturator from being inadvertently fed back into the lead recurrence.

Conditional integration prevents windup: the integral update is rejected if it
would drive the candidate reference farther beyond +/-0.7 rad. Updates that move
it back toward the allowed range remain permitted. Integration is also frozen
when measured gear angle is outside +/-0.8 rad.

The existing inner-loop equation remains:

```text
u[k] = 0.692327639311257 u[k-1]
     + 0.032635065920414 innerError[k]
     - 1.462208491947337 innerError[k-1]

innerError[k] = theta_ref_sat[k] - theta[k]
```

The current sketch adds no stiction offset for positive controller voltage and
subtracts 0.522 V for negative controller voltage. The requested motor voltage is
set to zero if measured gear angle exceeds +0.8 rad or is below -0.8 rad. That
shutdown is not latched; control resumes if the measured angle returns inside the
range. The driver library clips applied commands at +/-6 V; the logged motor
voltage is the request before that clipping.

## Files and preserved code

- [Integrator sketch](../code_lab3/Codelab3_integrator.ino): controller, sensor
  calibration, reference generation, safety checks, and serial output.
- [Original Part B sketch](../code_lab3/Codelab3.ino): retained separately.
- [main.cpp](main.cpp): includes the integrator sketch and exposes the local
  `geeWhiz` dependency to PlatformIO's library discovery.
- [platformio.ini](../platformio.ini): selects `code_lab3_integrator` as the source
  directory, avoiding compilation of both `.ino` sketches together.

Earlier experiment code remains commented out. Added or modified Part B/C lines
are marked with `// *` where applicable. The unused `TEST_ANGLE` constant does not
control this experiment; the outer controller generates the gear-angle reference.

## Reference, calibration, and recording

The ball-position reference alternates between 0.10 and 0.25 m every 15 seconds:
a 30-second period with 50% duty cycle, timed from Arduino startup.

```text
theta = -0.000362771 * motorRaw + 1.398480                 [rad]
ballPosition = 0.0000248138957816377 * ballRaw
             + 0.00148883374689826                       [m]
beamAngle = 0.0609 * theta                               [rad, estimate]
```

The ball-position calibration uses the measured points 3970 at 0.10 m and 8000 at
0.20 m. `beamAngle` is calculated from gear angle, not independently measured.

Close other Serial Monitors, then run from the project root:

```sh
python3 scripts/lab_data.py record --upload --name lab3_integrator --seconds 90 \
  --columns time_ms theta_ref theta_ref_sat theta control_voltage motor_voltage ball_raw ball_position beam_angle ball_ref \
  --y ball_ref ball_position --show
```

`--upload` builds and uploads the integrator variant before recording. Omit it to
record an already running sketch. The logger saves CSV, PNG, and interactive HTML
files under `data/<name>/`. Beginning a recording does not restart the reference
clock or clear controller state unless the board resets.

## Results reviewed so far

The reviewed recording is
[20261110_162928_538571_lab3_integrator.csv](../data/lab3_integrator/20261110_162928_538571_lab3_integrator.csv),
with its [position plot](../data/lab3_integrator/20261110_162928_538571_lab3_integrator.png).
These results describe that recording; controller and compensation settings should
be archived with future runs because the sketch can change afterward.

Startup included measured gear angles outside the safety range, keeping motor
voltage at zero for approximately 14.48 seconds. The later complete transitions
were assessed separately. Times below are Arduino timestamps, not time relative
to the start of the plotted recording.

| Step time | Target (m) | Mean final position (m) | Mean error (mm) | 2% settling time (s) |
| --- | --- | --- | --- | --- |
| 30 s | 0.10 | 0.099298 | +0.702 | 4.815 |
| 45 s | 0.25 | 0.245378 | +4.622 | Not settled before next step |
| 60 s | 0.10 | 0.101411 | -1.411 | 14.925 |
| 75 s | 0.25 | 0.248523 | +1.477 | 14.410 |

Final position is averaged over the last three seconds of each plateau. Error is
reference minus position. The 2% settling band is +/-0.003 m, based on the 0.15 m
step magnitude, and requires remaining inside until the next step. The largest
overshoot across these four transitions was 33.93% of the reference step.
Recorded sample intervals were uniformly 5 ms, and requested motor voltage ranged
from -4.436 to +2.512 V over the full recording.

Tracking improved substantially, but raw-data settling did not consistently meet
7 seconds and exact zero steady-state error was not demonstrated. With Part E's
wider bands (0.094-0.106 m and 0.240-0.260 m), these four later transitions settled
in 4.565, 2.500, 4.160, and 2.385 seconds respectively. Those are different
acceptance bands and do not establish compliance with the tighter criterion.

## Work remaining for the report

1. Document the outer-loop design rationale, including the integral-gain choice.
2. Simulate the complete sampled-data system and show whether Part C's stability,
   zero steady-state error, settling time <=7 s, and overshoot <45% requirements
   are met. Include reference and ball position on the same plot.
3. Compare the simulation with subsequent experimental results. Correct the
   startup condition before assessing normal step responses.
4. For the later final assessment, record ten consecutive periods after setup;
   the reviewed 90-second run does not provide that evidence.

Requirements source: [Lab manual](../MTE_484_F26_Lab_Manual%20%281%29.pdf),
Lab 3 Parts C-E, printed pages 32-34.

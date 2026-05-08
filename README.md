# Traffic Lights

A traffic light simulator built with the [Adapnex SDK](https://docs.adapnex.com/adapnex-sdk/latest/), targeting the WAGO Compact Controller 100.

## Demo

https://github.com/user-attachments/assets/978f203a-960a-42e4-9906-98b88d4694d0

## Hardware mapping

| Pin | Direction | Signal              |
|-----|-----------|---------------------|
| DI1 | Input     | `main_switch`       |
| DI2 | Input     | `pedestrian_button` |
| DO1 | Output    | `north_light`       |
| DO2 | Output    | `east_light`        |
| DO3 | Output    | `south_light`       |
| DO4 | Output    | `west_light`        |

## How it works

The control logic lives in `main_task.h` as a `MainTask` running on a 20ms cyclic task group. Every cycle it:

1. Reads `main_switch` and `pedestrian_button`.
2. Updates an internal state machine.
3. Drives the four light outputs.

### State machine

```
        ┌───────┐
        │  Off  │ ◀── main_switch == false (always)
        └───┬───┘
            │ main_switch goes true
            ▼
   ┌───────────────┐  10s  ┌───────────────┐
   │  NorthSouth   │ ────▶ │   EastWest    │
   │  N+S lit      │       │  E+W lit      │
   └───────┬───────┘ ◀──── └───────┬───────┘
           │           10s         │
           │                       │
           │ pedestrian pending?   │ pedestrian pending?
           │      yes ▼            │ yes ▼
           │  ┌────────────────────┴──┐
           └─▶│  PedestrianFlash      │
              │  all four blinking    │
              │  500ms on/off, 10s    │
              └───────┬───────────────┘
                      │ resume next normal phase
                      ▼
              (whichever was queued)
```

- **Off** — `main_switch` is low. All outputs forced low. Any queued pedestrian request is cleared.
- **NorthSouth / EastWest** — Normal alternation. Each phase lasts 10s.
- **PedestrianFlash** — Triggered on a pedestrian button press. The press is _latched_ at the rising edge and consumed at the end of the current normal phase, so the current cycle finishes before flashing begins. After 10s of flashing, the system resumes whichever normal phase was queued next (e.g. press during NS → NS finishes → flash → EW).

### Function blocks used

The control loop is built from a few stateful primitives provided by the SDK (declared as members so their state persists across cycles):

- `ClockGenerator phase_clock` — pulses once every 10s while enabled, driving phase transitions.
- `SquareWaveGenerator flash` — 500ms on, 500ms off; drives the pedestrian-flash blink pattern.
- `R_TRIG pedestrian_edge` — rising-edge detector so a held button only registers once.

A small `clock_armed` flag swallows the very first `phase_clock` pulse after enable, so the initial NorthSouth phase gets its full 10s instead of being cut short by the rising-edge tick.

## Project layout

```
.
├── CMakeLists.txt   # build + test registration
├── main.cpp         # setup(): scheduler, IO driver, wiring
├── main_task.h      # MainTask: state machine + control loop
├── main_test.cpp    # gtest unit tests using the Simulation fixture
└── README.md
```

## Getting started

### Prerequisites

- The [Adapnex CLI](https://docs.adapnex.com/) installed and on your `PATH` (`adapnex version` to verify).
- A registered device to deploy to. List what you have available with:
  ```sh
  adapnex device list
  ```
  This project targets the WAGO Compact Controller 100 simulator.

### Build

```sh
adapnex build wago_cc100_simulator traffic_lights
```

The compiled executable lands at `build_wago_cc100_simulator/traffic_lights`. Build directories (`build_generic/`, `build_wago_cc100_simulator/`) are gitignored.

### Run on the simulator

```sh
./build_wago_cc100_simulator/traffic_lights
```

This deploys the binary and attaches to its output. Once it's running:

- Toggle **DI1** to power the system on/off.
- Press **DI2** to queue a pedestrian crossing.
- Watch **DO1–DO4** for the four traffic lights.

### Tests

Tests run on the host (no device needed) using the SDK's `Simulation` fixture:

```sh
adapnex test
```

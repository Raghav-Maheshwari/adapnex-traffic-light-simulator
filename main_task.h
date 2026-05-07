#pragma once

#include "adapnex.h"

class MainTask final : public Task {
public:
    // Software IO — wired to physical DI/DO pins in main.cpp.
    bool main_switch = false;
    bool pedestrian_button = false;

    bool north_light = false;
    bool east_light = false;
    bool south_light = false;
    bool west_light = false;

private:
    enum class State { Off, NorthSouth, EastWest, PedestrianFlash };

    State state = State::Off;

    // Remembers which normal phase to return to after PedestrianFlash.
    State next_normal = State::NorthSouth;

    bool pedestrian_pending = false;

    // Swallows the first phase_clock tick after enable so the very
    // first NorthSouth phase gets a full 10s instead of being cut
    // short by the rising-edge pulse.
    bool clock_armed = false;

    // Pulses CLK true for one cycle every PT while enabled — drives
    // 10s phase changes.
    ClockGenerator phase_clock;

    // Toggles its output between PTON-high and PTOFF-low while enabled.
    SquareWaveGenerator flash;

    // One-cycle pulse on the rising edge of its input.
    R_TRIG pedestrian_edge;

    // Pick the next state when the phase clock ticks.
    State advance_state(State current) {
        switch (current) {
            case State::NorthSouth:
                next_normal = State::EastWest;
                if (pedestrian_pending) {
                    pedestrian_pending = false;
                    return State::PedestrianFlash;
                }
                return State::EastWest;
            case State::EastWest:
                next_normal = State::NorthSouth;
                if (pedestrian_pending) {
                    pedestrian_pending = false;
                    return State::PedestrianFlash;
                }
                return State::NorthSouth;
            case State::PedestrianFlash:
                return next_normal;
            case State::Off:
                return State::Off;
        }
        return State::Off;
    }

    // Called by the cyclic task group every 20ms (configured in main.cpp).
    void Update() override {
        bool ped_pressed = false;
        pedestrian_edge(pedestrian_button, ped_pressed);

        bool phase_tick = false;
        phase_clock(state != State::Off, 10s, phase_tick);

        bool flash_clk = false;
        flash(state == State::PedestrianFlash, 500ms, 500ms, flash_clk);

        // --- State updates ---
        if (!main_switch) {
            // Master switch off: hard-reset.
            state = State::Off;
            next_normal = State::NorthSouth;
            pedestrian_pending = false;
            clock_armed = false;
        } else {
            if (state == State::Off) {
                state = State::NorthSouth;
                next_normal = State::EastWest;
            }
            if (ped_pressed) {
                pedestrian_pending = true;
            }
            if (phase_tick) {
                if (clock_armed) {
                    state = advance_state(state);
                } else {
                    // First tick after enable — swallow it.
                    clock_armed = true;
                }
            }
        }

        // --- Outputs ---
        switch (state) {
            case State::NorthSouth:
                north_light = south_light = true;
                east_light = west_light = false;
                break;
            case State::EastWest:
                east_light = west_light = true;
                north_light = south_light = false;
                break;
            case State::PedestrianFlash:
                north_light = east_light = south_light = west_light = flash_clk;
                break;
            case State::Off:
                north_light = east_light = south_light = west_light = false;
                break;
        }
    }
};

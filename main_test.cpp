#include "adapnex.h"

#include <gtest/gtest.h>

#include "main_task.h"

// With main_switch off, every light stays low.
TEST_F(Simulation, SystemOffWhenMainSwitchLow) {
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);
    const auto main_task = main_group->CreateTask<MainTask>();

    main_task->main_switch = false;

    Simulate(1s);

    EXPECT_FALSE(main_task->north_light);
    EXPECT_FALSE(main_task->south_light);
    EXPECT_FALSE(main_task->east_light);
    EXPECT_FALSE(main_task->west_light);
}

// Flipping main_switch on brings up the NorthSouth phase first.
TEST_F(Simulation, MainSwitchTurnsOnSystem) {
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);
    const auto main_task = main_group->CreateTask<MainTask>();

    main_task->main_switch = true;

    Simulate(20ms);

    EXPECT_TRUE(main_task->north_light);
    EXPECT_TRUE(main_task->south_light);
    EXPECT_FALSE(main_task->east_light);
    EXPECT_FALSE(main_task->west_light);
}

// Flipping main_switch off mid-cycle turns every light off.
TEST_F(Simulation, MainSwitchTurnsOffSystem) {
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);
    const auto main_task = main_group->CreateTask<MainTask>();

    main_task->main_switch = true;
    Simulate(2s);
    ASSERT_TRUE(main_task->north_light);

    main_task->main_switch = false;
    Simulate(20ms);

    EXPECT_FALSE(main_task->north_light);
    EXPECT_FALSE(main_task->south_light);
    EXPECT_FALSE(main_task->east_light);
    EXPECT_FALSE(main_task->west_light);
}

// After ~10s in NorthSouth, the lights toggle to EastWest.
TEST_F(Simulation, LightsToggleFromNorthSouthToEastWest) {
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);
    const auto main_task = main_group->CreateTask<MainTask>();

    main_task->main_switch = true;

    // Still in NS just before the phase boundary.
    Simulate(9s);
    EXPECT_TRUE(main_task->north_light);
    EXPECT_TRUE(main_task->south_light);
    EXPECT_FALSE(main_task->east_light);
    EXPECT_FALSE(main_task->west_light);

    // Cross the 10s boundary — now in EW.
    Simulate(2s);
    EXPECT_FALSE(main_task->north_light);
    EXPECT_FALSE(main_task->south_light);
    EXPECT_TRUE(main_task->east_light);
    EXPECT_TRUE(main_task->west_light);
}

// Pressing the pedestrian button mid-phase queues the flash sequence,
// which runs for 10s with all four lights blinking together.
TEST_F(Simulation, PedestrianButtonQueuesFlashing) {
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);
    const auto main_task = main_group->CreateTask<MainTask>();

    main_task->main_switch = true;
    Simulate(20ms);

    main_task->pedestrian_button = true;
    Simulate(20ms);

    // Wait for the NS phase to finish and flash to begin.
    Simulate(11s);

    // In the flash phase all four outputs are driven by the same
    // square wave, so they're always equal to each other.
    EXPECT_EQ(main_task->north_light, main_task->south_light);
    EXPECT_EQ(main_task->north_light, main_task->east_light);
    EXPECT_EQ(main_task->north_light, main_task->west_light);

    // Half a flash period later they should have toggled to the
    // opposite value (proving they're actually blinking, not just off).
    bool before = main_task->north_light;
    Simulate(500ms);
    EXPECT_NE(before, main_task->north_light);
    EXPECT_EQ(main_task->north_light, main_task->south_light);
    EXPECT_EQ(main_task->north_light, main_task->east_light);
    EXPECT_EQ(main_task->north_light, main_task->west_light);
}

// After the pedestrian flash completes, normal alternation resumes
// with EastWest (the phase that would have followed NorthSouth).
TEST_F(Simulation, CycleReturnsToNextStateAfterPedestrianFlash) {
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);
    const auto main_task = main_group->CreateTask<MainTask>();

    main_task->main_switch = true;
    Simulate(20ms);

    main_task->pedestrian_button = true;
    Simulate(20ms);

    // 10s NS + 10s flash + a small cushion to cross into EW.
    Simulate(21s);

    EXPECT_FALSE(main_task->north_light);
    EXPECT_FALSE(main_task->south_light);
    EXPECT_TRUE(main_task->east_light);
    EXPECT_TRUE(main_task->west_light);
}

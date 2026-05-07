#include "adapnex.h"

#include "main_task.h"

// The 'setup' function is the entry point for Adapnex applications.
// It runs once at startup and is responsible for declaring tasks and
// wiring software IO to physical hardware pins.
void setup() {
    // --- 1. Configure the Scheduler ---
    // A cyclic task group runs all its tasks at a fixed period on the
    // same thread. 20ms = 50Hz, plenty fast for traffic-light timing
    // (10s phases, 500ms blink). Priority 0 is the default.
    const auto main_group = Application::CreateCyclicTaskGroup(20ms, 0);

    // Instantiate our control logic task inside that group.
    // CreateTask<T>() forwards to T's constructor and registers the
    // task to be ticked each cycle.
    const auto main_task = main_group->CreateTask<MainTask>();

    // --- 2. Configure Hardware Drivers ---
    // The CC100 IO driver exposes the WAGO Compact Controller 100's
    // digital inputs (DI1..) and outputs (DO1..) as software IO that
    // can be wired to task members.
    const auto io_driver = main_group->CreateTask<CC100IODriver>();

    // --- 3. Wiring ---
    // '>>' flows a value from a producer (driver input) into a consumer
    // (task input field). '<<' flows from task output back to driver
    // output. The direction of the arrow follows the data.

    // Inputs: hardware → task.
    io_driver->DI1 >> main_task->main_switch;        // Master on/off
    io_driver->DI2 >> main_task->pedestrian_button;  // Crossing request

    // Outputs: task → hardware. One physical output per traffic light.
    io_driver->DO1 << main_task->north_light;
    io_driver->DO2 << main_task->east_light;
    io_driver->DO3 << main_task->south_light;
    io_driver->DO4 << main_task->west_light;
}

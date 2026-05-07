cmake_minimum_required(VERSION 3.25)
project(traffic_lights)

# Create the executable application
# 'adapnex_executable' handles toolchains and dependencies automatically.
adapnex_executable(traffic_lights main.cpp)

# Unit tests
enable_testing()
adapnex_tests(traffic_lights_tests main_test.cpp)
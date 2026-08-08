# TabManager prototype

This small prototype simulates opening browser tabs where each tab allocates a fixed amount of memory and background tabs are periodically "refreshed" (touched) to simulate activity and keep pages resident.

Build (requires a C++17 compiler and pthreads on POSIX; on Windows use an appropriate toolchain):

g++ -std=c++17 -O2 -pthread utilities/tab_manager/TabManager.cpp utilities/tab_manager/test_main.cpp -o tab_test

Run:

./tab_test 10 10.24 60

Arguments: <tab_count> <per_tab_mib> <runtime_seconds>

Notes:
- Default per-tab memory is ~10.24 MiB which yields ~102 MiB for 10 tabs and ~1 GiB for 100 tabs.
- This is a simulation: a real browser requires much more sophisticated memory management and integration with the renderer and OS.

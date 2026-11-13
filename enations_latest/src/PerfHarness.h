#pragma once

// All game access is serviced on the main loop. The listener is opt-in.
void PerfHarnessStart();
void PerfHarnessStop();
void PerfHarnessService();
const char* PerfHarnessPendingLoadPath();

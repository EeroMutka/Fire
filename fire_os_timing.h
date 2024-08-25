// fire_os_timing.h - Timing library (currently only implemented on windows)
// Written by Eero Mutka.
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than OS_TIMING_, simply do a find and replace in this file.
//

#ifndef FIRE_OS_TIMING_INCLUDED
#define FIRE_OS_TIMING_INCLUDED

#ifndef OS_TIMING_API
#define OS_TIMING_API static
#endif

#include <stdint.h>

// You must call OS_TIMING_Init to initialize this library!
OS_TIMING_API void OS_TIMING_Init();

OS_TIMING_API uint64_t OS_TIMING_GetTick();

// Returns the duration in seconds between two ticks.
OS_TIMING_API double OS_TIMING_GetDuration(uint64_t start, uint64_t end);

#ifdef /**********/ FIRE_OS_TIMING_IMPLEMENTATION /**********/

#ifdef FIRE_OS_TIMING_NO_WINDOWS_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint64_t x; } LARGE_INTEGER;
__declspec(dllimport) bool __stdcall QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);
__declspec(dllimport) bool __stdcall QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
#ifdef __cplusplus
} // extern "C"
#endif
#else
#include <Windows.h>
#endif

#ifndef _WINDOWS_
#endif

OS_TIMING_API uint64_t OS_TIMING_ticks_per_second;

OS_TIMING_API void OS_TIMING_Init() {
	QueryPerformanceFrequency((LARGE_INTEGER*)&OS_TIMING_ticks_per_second);
}

OS_TIMING_API uint64_t OS_TIMING_GetTick() {
	uint64_t tick;
	QueryPerformanceCounter((LARGE_INTEGER*)&tick);
	return tick;
}

OS_TIMING_API double OS_TIMING_GetDuration(uint64_t start, uint64_t end) {
	// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
	uint64_t elapsed = end - start;
	return (double)elapsed / (double)OS_TIMING_ticks_per_second;
}

#endif // FIRE_OS_TIMING_IMPLEMENTATION
#endif // FIRE_OS_TIMING_INCLUDED
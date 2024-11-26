// fire_os_timing.h - by Eero Mutka (https://eeromutka.github.io/)
// 
// High-performance time measurements. Only Windows is supported for time being.
// 
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than OS_TIMING_, simply do a find and replace in this file.
//

#ifndef FIRE_OS_TIMING_INCLUDED
#define FIRE_OS_TIMING_INCLUDED

#ifndef OS_TIMING_API
#define OS_TIMING_API
#endif

#include <stdint.h>

OS_TIMING_API uint64_t OS_GetCPUFrequency();

OS_TIMING_API uint64_t OS_GetCPUTick();

// Returns the duration in seconds between two CPU ticks.
OS_TIMING_API double OS_GetDuration(uint64_t cpu_frequency, uint64_t start, uint64_t end);

#ifdef /**********/ FIRE_OS_TIMING_IMPLEMENTATION /**********/

// -- from Windows.h -----------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
union _LARGE_INTEGER;
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(union _LARGE_INTEGER* lpFrequency);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(union _LARGE_INTEGER* lpPerformanceCount);
#ifdef __cplusplus
} // extern "C"
#endif
// -----------------------------------------------------------

OS_TIMING_API uint64_t OS_GetCPUFrequency() {
	uint64_t result;
	QueryPerformanceFrequency((union _LARGE_INTEGER*)&result);
	return result;
}

OS_TIMING_API uint64_t OS_GetCPUTick() {
	uint64_t tick;
	QueryPerformanceCounter((union _LARGE_INTEGER*)&tick);
	return tick;
}

OS_TIMING_API double OS_GetDuration(uint64_t cpu_frequency, uint64_t start, uint64_t end) {
	// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
	uint64_t elapsed = end - start;
	return (double)elapsed / (double)cpu_frequency;
}

#endif // FIRE_OS_TIMING_IMPLEMENTATION
#endif // FIRE_OS_TIMING_INCLUDED
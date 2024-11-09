// fire_os_sync.h - by Eero Mutka (https://eeromutka.github.io/)
// 
// Threading and synchronization primitives. Only Windows is supported for time being.
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than OS_SYNC_, simply do a find and replace in this file.
//

#ifndef FIRE_OS_SYNC_INCLUDED
#define FIRE_OS_SYNC_INCLUDED

#ifndef OS_SYNC_API
#define OS_SYNC_API
#endif

#include <assert.h>

typedef void (*OS_ThreadFn)(void* user_data);
typedef struct OS_Thread {
	void* os_specific;
	OS_ThreadFn fn;
	void* user_data;
} OS_Thread;

typedef struct OS_Mutex {
	uint64_t os_specific[5];
} OS_Mutex;

typedef struct OS_ConditionVar {
	uint64_t os_specific;
} OS_ConditionVar;

// NOTE: The `thread` pointer may not be moved or copied while in use.
// * if `debug_name` is an empty string, no debug name will be specified
OS_SYNC_API void OS_ThreadStart(OS_Thread* thread, OS_ThreadFn fn, void* user_data, const char* debug_name);
OS_SYNC_API void OS_ThreadJoin(OS_Thread* thread);

OS_SYNC_API void OS_MutexInit(OS_Mutex* mutex);
OS_SYNC_API void OS_MutexDestroy(OS_Mutex* mutex);
OS_SYNC_API void OS_MutexLock(OS_Mutex* mutex);
OS_SYNC_API void OS_MutexUnlock(OS_Mutex* mutex);

// NOTE: The condition var pointer cannot be moved or copied while in use.
OS_SYNC_API void OS_ConditionVarInit(OS_ConditionVar* condition_var);
OS_SYNC_API void OS_ConditionVarDestroy(OS_ConditionVar* condition_var);

// Unblock execution on one thread that's waiting on a condition variable (if there is one)
OS_SYNC_API void OS_ConditionVarSignal(OS_ConditionVar* condition_var);

// Unblock execution on all threads that are waiting on a condition variable
OS_SYNC_API void OS_ConditionVarBroadcast(OS_ConditionVar* condition_var);

// Add this thread to a condition variable's waiting list, unlock a mutex and block execution.
// When a signal is given for this thread to continue execution, the mutex is locked and the function returns.
// * The mutex must be locked/entered exactly once prior to calling this function!
OS_SYNC_API void OS_ConditionVarWait(OS_ConditionVar* condition_var, OS_Mutex* mutex);

#ifdef /**********/ FIRE_OS_SYNC_IMPLEMENTATION /**********/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h> // for _endthreadex

static uint32_t OS_ThreadEntryFn(void* args) {
	OS_Thread* thread = (OS_Thread*)args;
	thread->fn(thread->user_data);

	_endthreadex(0);
	return 0;
}

OS_SYNC_API void OS_ThreadStart(OS_Thread* thread, OS_ThreadFn fn, void* user_data, const char* debug_name) {
	assert(thread->os_specific == NULL);

	// OS_ThreadEntryFn might be called at any time after calling `_beginthreadex`, and we must pass `fn` and `user_data` to it somehow. We can't pass them
	// on the stack, because this function might have exited at the point the ThreadEntryFn is entered. So, let's pass them in the OS_Thread structure
	// and let the user manage the OS_Thread pointer. Alternatively, we could make the user manually call OS_SYNC_ThreadEnter(); and OS_SYNC_ThreadExit(); inside
	// their thread function and directly pass the user function into _beginthreadex.
	thread->fn = fn;
	thread->user_data = user_data;

	// We're using _beginthreadex instead of CreateThread, because we're using the C runtime library.
	// https://stackoverflow.com/questions/331536/windows-threading-beginthread-vs-beginthreadex-vs-createthread-c
	// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex

	unsigned int thread_id;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, OS_ThreadEntryFn, thread, 0/* CREATE_SUSPENDED */, &thread_id);
	thread->os_specific = handle;

	if (debug_name && *debug_name) {
		wchar_t debug_name_wide[256];
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, debug_name, -1, debug_name_wide, 256);
		SetThreadDescription(handle, debug_name_wide);
	}
}

OS_SYNC_API void OS_ThreadJoin(OS_Thread* thread) {
	WaitForSingleObject((HANDLE)thread->os_specific, INFINITE);
	CloseHandle((HANDLE)thread->os_specific);
	memset(thread, 0, sizeof(*thread)); // Do this so that we can safely start a new thread again using this same struct
}

OS_SYNC_API void OS_MutexInit(OS_Mutex* mutex) {
	assert(sizeof(OS_Mutex) >= sizeof(CRITICAL_SECTION));
	InitializeCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_SYNC_API void OS_MutexDestroy(OS_Mutex* mutex) {
	DeleteCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_SYNC_API void OS_MutexLock(OS_Mutex* mutex) {
	EnterCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_SYNC_API void OS_MutexUnlock(OS_Mutex* mutex) {
	LeaveCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_SYNC_API void OS_ConditionVarInit(OS_ConditionVar* condition_var) {
	assert(sizeof(OS_ConditionVar) >= sizeof(CONDITION_VARIABLE));
	InitializeConditionVariable((CONDITION_VARIABLE*)condition_var);
}

OS_SYNC_API void OS_ConditionVarDestroy(OS_ConditionVar* condition_var) {
	// No need to explicitly destroy
	// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializeconditionvariable
}

OS_SYNC_API void OS_ConditionVarWait(OS_ConditionVar* condition_var, OS_Mutex* mutex) {
	SleepConditionVariableCS((CONDITION_VARIABLE*)condition_var, (CRITICAL_SECTION*)mutex, INFINITE);
}

OS_SYNC_API void OS_ConditionVarSignal(OS_ConditionVar* condition_var) {
	WakeConditionVariable((CONDITION_VARIABLE*)condition_var);
}

OS_SYNC_API void OS_ConditionVarBroadcast(OS_ConditionVar* condition_var) {
	WakeAllConditionVariable((CONDITION_VARIABLE*)condition_var);
}

#endif // FIRE_OS_SYNC_IMPLEMENTATION
#endif // FIRE_OS_SYNC_INCLUDED
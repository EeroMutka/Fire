// fire_os.h - basic operating system abstraction layer
//
// Author: Eero Mutka
// Version: 0
// Date: 25 March, 2024
// 
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
// 
// About file paths in this library:
// - When a function takes a path as an argument, / and \ are valid separator characters.
// - When a function returns a path, all separators will be /
// I think that currently not all functions follow the above rules, TODO!
//

#ifndef OS_INCLUDED
#define OS_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#ifndef OS_CHECK_OVERRIDE
#include <assert.h>
#define OS_CHECK(x) assert(x)
#endif

#ifndef OS_API
#define OS_API static
#define OS_IMPLEMENTATION
#endif

#ifndef OS_STRING_OVERRIDE
typedef struct OS_String {
	const char* data;
	int size;
} OS_String;
#endif

#ifdef __cplusplus
#define OS_STR(X) OS_String{X, sizeof(X)-1}
#else
#define OS_STR(X) (OS_String){X, sizeof(X)-1}
#endif

/*
These can be combined as a mask.
@cleanup: remove this thing
*/
typedef enum {
	OS_ConsoleAttribute_Blue = 0x0001,
	OS_ConsoleAttribute_Green = 0x0002,
	OS_ConsoleAttribute_Red = 0x0004,
	OS_ConsoleAttribute_Intensify = 0x0008,
} OS_ConsoleAttribute;

typedef int OS_ConsoleAttributeFlags;

typedef struct OS_Mutex { uint64_t os_specific[5]; } OS_Mutex;
typedef struct OS_ConditionVar {
	uint64_t os_specific;
} OS_ConditionVar;

typedef struct { void* handle; } OS_DirectoryWatch;

typedef void (*OS_ThreadFn)(void* user_data);
typedef struct OS_Thread {
	void* os_specific;
	OS_ThreadFn fn;
	void* user_data;
} OS_Thread;

typedef struct {
	OS_String function;
	OS_String file;
	uint32_t line;
} OS_DebugStackFrame;
typedef struct { OS_DebugStackFrame* data; int length; } OS_DebugStackFrameArray;

// OS-specific window handle. On windows, it's represented as HWND.
typedef void* OS_WindowHandle;

typedef struct { uint64_t tick; } OS_Tick;

typedef struct { void* handle; } OS_DynamicLibrary;

typedef struct { uint64_t os_specific; } OS_FileTime;

typedef enum OS_FileOpenMode {
	OS_FileOpenMode_Read,
	OS_FileOpenMode_Write,
	OS_FileOpenMode_Append,
} OS_FileOpenMode;

//#ifndef OS_FILE_WRITER_BUFFER_SIZE
//#define OS_FILE_WRITER_BUFFER_SIZE 4096
//#endif

typedef struct OS_File {
	OS_FileOpenMode mode;
	void* os_handle;
	// ...Maybe I should provide a buffered write function.
	// uint8_t buffer[OS_FILE_WRITER_BUFFER_SIZE];
} OS_File;

typedef struct OS_FileInfo {
	bool is_directory;
	OS_String name; // includes the file extension if there is one
	OS_FileTime last_write_time;
} OS_FileInfo;
typedef struct { OS_FileInfo* data; int length; } OS_FileInfoArray;

typedef enum OS_Input {
	OS_Input_Invalid = 0,

	OS_Input_Space = 32,
	OS_Input_Apostrophe = 39,   /* ' */
	OS_Input_Comma = 44,        /* , */
	OS_Input_Minus = 45,        /* - */
	OS_Input_Period = 46,       /* . */
	OS_Input_Slash = 47,        /* / */

	OS_Input_0 = 48,
	OS_Input_1 = 49,
	OS_Input_2 = 50,
	OS_Input_3 = 51,
	OS_Input_4 = 52,
	OS_Input_5 = 53,
	OS_Input_6 = 54,
	OS_Input_7 = 55,
	OS_Input_8 = 56,
	OS_Input_9 = 57,

	OS_Input_Semicolon = 59,    /* ; */
	OS_Input_Equal = 61,        /* = */
	OS_Input_LeftBracket = 91,  /* [ */
	OS_Input_Backslash = 92,    /* \ */
	OS_Input_RightBracket = 93, /* ] */
	OS_Input_GraveAccent = 96,  /* ` */

	OS_Input_A = 65,
	OS_Input_B = 66,
	OS_Input_C = 67,
	OS_Input_D = 68,
	OS_Input_E = 69,
	OS_Input_F = 70,
	OS_Input_G = 71,
	OS_Input_H = 72,
	OS_Input_I = 73,
	OS_Input_J = 74,
	OS_Input_K = 75,
	OS_Input_L = 76,
	OS_Input_M = 77,
	OS_Input_N = 78,
	OS_Input_O = 79,
	OS_Input_P = 80,
	OS_Input_Q = 81,
	OS_Input_R = 82,
	OS_Input_S = 83,
	OS_Input_T = 84,
	OS_Input_U = 85,
	OS_Input_V = 86,
	OS_Input_W = 87,
	OS_Input_X = 88,
	OS_Input_Y = 89,
	OS_Input_Z = 90,

	OS_Input_Escape = 256,
	OS_Input_Enter = 257,
	OS_Input_Tab = 258,
	OS_Input_Backspace = 259,
	OS_Input_Insert = 260,
	OS_Input_Delete = 261,
	OS_Input_Right = 262,
	OS_Input_Left = 263,
	OS_Input_Down = 264,
	OS_Input_Up = 265,
	OS_Input_PageUp = 266,
	OS_Input_PageDown = 267,
	OS_Input_Home = 268,
	OS_Input_End = 269,
	OS_Input_CapsLock = 280,
	OS_Input_ScrollLock = 281,
	OS_Input_NumLock = 282,
	OS_Input_PrintScreen = 283,
	OS_Input_Pause = 284,

	OS_Input_F1 = 290,
	OS_Input_F2 = 291,
	OS_Input_F3 = 292,
	OS_Input_F4 = 293,
	OS_Input_F5 = 294,
	OS_Input_F6 = 295,
	OS_Input_F7 = 296,
	OS_Input_F8 = 297,
	OS_Input_F9 = 298,
	OS_Input_F10 = 299,
	OS_Input_F11 = 300,
	OS_Input_F12 = 301,
	OS_Input_F13 = 302,
	OS_Input_F14 = 303,
	OS_Input_F15 = 304,
	OS_Input_F16 = 305,
	OS_Input_F17 = 306,
	OS_Input_F18 = 307,
	OS_Input_F19 = 308,
	OS_Input_F20 = 309,
	OS_Input_F21 = 310,
	OS_Input_F22 = 311,
	OS_Input_F23 = 312,
	OS_Input_F24 = 313,
	OS_Input_F25 = 314,

	//OS_Input_KP_0 = 320,
	//OS_Input_KP_1 = 321,
	//OS_Input_KP_2 = 322,
	//OS_Input_KP_3 = 323,
	//OS_Input_KP_4 = 324,
	//OS_Input_KP_5 = 325,
	//OS_Input_KP_6 = 326,
	//OS_Input_KP_7 = 327,
	//OS_Input_KP_8 = 328,
	//OS_Input_KP_9 = 329,

	//OS_Input_KP_Decimal = 330,
	//OS_Input_KP_Divide = 331,
	//OS_Input_KP_Multiply = 332,
	//OS_Input_KP_Subtract = 333,
	//OS_Input_KP_Add = 334,
	//OS_Input_KP_Enter = 335,
	//OS_Input_KP_Equal = 336,

	OS_Input_LeftShift = 340,
	OS_Input_LeftControl = 341,
	OS_Input_LeftAlt = 342,
	OS_Input_LeftSuper = 343,
	OS_Input_RightShift = 344,
	OS_Input_RightControl = 345,
	OS_Input_RightAlt = 346,
	OS_Input_RightSuper = 347,
	//OS_Input_Menu = 348,

	OS_Input_Shift = 349,
	OS_Input_Control = 350,
	OS_Input_Alt = 351,
	OS_Input_Super = 352,

	OS_Input_MouseLeft = 353,
	OS_Input_MouseRight = 354,
	OS_Input_MouseMiddle = 355,
	//OS_Input_Mouse_4 = 356,
	//OS_Input_Mouse_5 = 357,
	//OS_Input_Mouse_6 = 358,
	//OS_Input_Mouse_7 = 359,
	//OS_Input_Mouse_8 = 360,

	OS_Input_COUNT,
} OS_Input;

typedef enum OS_MouseCursor {
	OS_MouseCursor_Arrow,
	OS_MouseCursor_Hand,
	OS_MouseCursor_I_Beam,
	OS_MouseCursor_Crosshair,
	OS_MouseCursor_ResizeH,
	OS_MouseCursor_ResizeV,
	OS_MouseCursor_ResizeNESW,
	OS_MouseCursor_ResizeNWSE,
	OS_MouseCursor_ResizeAll,
	OS_MouseCursor_COUNT
} OS_MouseCursor;

typedef void(*OS_WindowOnResize)(uint32_t width, uint32_t height, void* user_data);

typedef uint8_t OS_InputStateFlags;
typedef enum OS_InputStateFlag {
	OS_InputStateFlag_IsDown = 1 << 0,
	OS_InputStateFlag_WasPressed = 1 << 1,
	OS_InputStateFlag_WasPressedOrRepeat = 1 << 2,
	OS_InputStateFlag_WasReleased = 1 << 3,
} OS_InputStateFlag;

typedef struct OS_Inputs {
	// if we do keep a public list of `OS_Input` states, and in general keep the ABI public,
	// it means that whatever library uses `foundation_os` for getting input doesn't actually their user
	// to use `foundation_os`. The user COULD pull the rug and provide their own initialized `fWindowEvents`!

	uint32_t mouse_x, mouse_y; // mouse position in window pixel coordinates (+x right, +y down)
	int32_t mouse_raw_dx, mouse_raw_dy; // raw mouse motion, +x right, +y down
	float mouse_wheel_delta; // +1.0 means the wheel was rotated forward by one detent (scroll step)

	OS_InputStateFlags input_states[OS_Input_COUNT];

	uint32_t* text_input_utf32;
	int text_input_utf32_length;
} OS_Inputs;

typedef struct OS_Log OS_Log;
struct OS_Log {
	void(*print)(OS_Log* self, OS_String data);
};

typedef struct OS_Window {
	OS_WindowHandle handle;
	struct {
		uint32_t left, top, right, bottom;
	} pre_fullscreen_state; // When calling OS_SetFullscreen, some information about the previous window state needs to be saved in these fields
} OS_Window;

typedef struct OS_ArenaBlockHeader {
	int size_including_header;
	struct OS_ArenaBlockHeader* next; // may be NULL
} OS_ArenaBlockHeader;

typedef struct OS_ArenaMark {
	OS_ArenaBlockHeader* block; // If the arena has no blocks allocated yet, then we mark the beginning of the arena by setting this member to NULL.
	char* ptr;
} OS_ArenaMark;

typedef struct OS_DefaultArena {
	int block_size;
	OS_ArenaBlockHeader* first_block; // may be NULL
	OS_ArenaMark mark;
} OS_DefaultArena;

#ifdef OS_USE_FIRE_DS_ARENA
#define OS_CUSTOM_ARENA
#define OS_Arena DS_Arena
#define OS_ArenaInit_Custom(arena, block_size)      DS_ArenaInit(arena, block_size, DS_HEAP)
#define OS_ArenaDeinit_Custom(arena)                DS_ArenaDeinit(arena)
#define OS_ArenaPush_Custom(arena, size, alignment) DS_ArenaPushEx(arena, size, alignment)
#define OS_ArenaReset_Custom(arena)                 DS_ArenaReset(arena)
#endif

#ifdef OS_CUSTOM_ARENA
// If you want to implement your own custom arena, you must provide definitions for the following:
// - OS_Arena
// - void OS_ArenaInit_Custom(OS_Arena *arena, int block_size)
// - void OS_ArenaDeinit_Custom(OS_Arena *arena)
// - char *OS_ArenaPush_Custom(OS_Arena *arena, int size, int alignment)
// - void OS_ArenaReset_Custom(OS_Arena *arena)
#else
typedef OS_DefaultArena OS_Arena;
#endif

OS_API void OS_Init(void);
OS_API void OS_Deinit(void);

OS_API void OS_ArenaInit(OS_Arena* arena, int block_size);
OS_API void OS_ArenaDeinit(OS_Arena* arena);
OS_API char* OS_ArenaPush(OS_Arena* arena, int size);
OS_API void OS_ArenaReset(OS_Arena* arena);

// OS_API OS_Log *OS_Console();  // Redirects writes to OS console (stdout)
// OS_API OS_Log *OS_DebugLog(); // Redirects writes to OS debug output

// `log` may be NULL, in which case nothing is printed
// OS_API void OS_LogPrint(OS_Log *log, const char *fmt, ...);

OS_API void OS_PrintString(OS_String str);
OS_API void OS_PrintStringColor(OS_String str, OS_ConsoleAttributeFlags attributes_mask);
OS_API void OS_DebugPrintString(OS_String str);

OS_API uint64_t OS_ReadCycleCounter(void);
OS_API void OS_Sleep(uint64_t ms);

// * Uses the current working directory
// * if you want to read both stdout and stderr together in the right order, then pass identical pointers to `stdout_log` and `stderr_log`.
// * `args[0]` should be the path of the executable, where `\' and `/` are accepted path separators.
// * `out_exit_code`, `stdout_log`, `stderr_log` may be NULL
OS_API bool OS_RunCommand(OS_String* args, uint32_t args_count, uint32_t* out_exit_code, OS_Log* stdout_log, OS_Log* stderr_log);

OS_API void OS_OpenFileInDefaultProgram(OS_String file);

// Find the path of an executable given its name, or return an empty string if it's not found.
// NOTE: `name` must include the file extension!
OS_API OS_String OS_FindExecutable(OS_Arena* arena, OS_String name);

OS_API bool OS_SetWorkingDir(OS_String dir);
OS_API OS_String OS_GetWorkingDir(OS_Arena* arena);

OS_API OS_String OS_GetExecutablePath(OS_Arena* arena);

OS_API void OS_MessageBox(OS_String title, OS_String message);

OS_API OS_DebugStackFrameArray OS_DebugGetStackTrace(OS_Arena* arena);

// -- Clipboard ----------------------------------------------------------------

OS_API OS_String OS_ClipboardGetText(OS_Arena* arena);
OS_API void OS_ClipboardSetText(OS_String str);

// -- DynamicLibrary -----------------------------------------------------------

OS_API bool OS_LoadDLL(OS_String dll_path, OS_DynamicLibrary* out_dll);
OS_API void OS_UnloadDLL(OS_DynamicLibrary dll);

OS_API void* OS_GetSymbolAddress(OS_DynamicLibrary dll, OS_String symbol); // NULL is returned if not found

// -- Files --------------------------------------------------------------------

// The path separator in the returned string will depend on the OS. On windows, it will be a backslash.
// If the provided path is invalid, an empty string will be returned.
// TODO: maybe it'd be better if these weren't os-specific functions, and instead could take an argument for specifying
//       windows-style paths / unix-style paths
OS_API bool OS_PathIsAbsolute(OS_String path);

// OLD COMMENT: If `working_dir` is an empty string, the current working directory will be used.
// `working_dir` must be an absolute path.
OS_API bool OS_PathToCanonical(OS_Arena* arena, OS_String path, OS_String* out_canonical);

OS_API bool OS_GetAllFilesInDirectory(OS_Arena* arena, OS_String directory, OS_FileInfoArray* out_files);
// OS_API bool OS_VisitDirectory(OS_WorkingDir working_dir, OS_String path, OS_VisitDirectoryVisitor visitor, void *visitor_userptr);

OS_API bool OS_DirectoryExists(OS_String directory_path);
OS_API bool OS_DeleteDirectory(OS_String directory_path); // If the directory doesn't already exist, it's treated as a success.
OS_API bool OS_MakeDirectory(OS_String directory_path);   // If the directory already exists, it's treated as a success.

OS_API bool OS_ReadEntireFile(OS_Arena* arena, OS_String file_path, OS_String* out_str);
OS_API bool OS_WriteEntireFile(OS_String file_path, OS_String data);

OS_API bool OS_FileOpen(OS_String file_path, OS_FileOpenMode mode, OS_File* out_file);
OS_API bool OS_FileClose(OS_File* file);
OS_API uint64_t OS_FileSize(OS_File* file);
OS_API size_t OS_FileRead(OS_File* file, void* dst, size_t size);
OS_API bool OS_FileWrite(OS_File* file, OS_String data);
OS_API uint64_t OS_FileGetCursor(OS_File* file);
OS_API bool OS_FileSetCursor(OS_File* file, uint64_t position);

OS_API bool OS_FileModtime(OS_String file_path, OS_FileTime* out_modtime); // Returns false if the file does not exist
OS_API int OS_FileCmpModtime(OS_FileTime a, OS_FileTime b); // Returns 1 when a > b, -1 when a < b, 0 when a == b
OS_API bool OS_CloneFile(OS_String dst_file_path, OS_String src_file_path); // NOTE: the parent directory of the destination filepath must already exist.
OS_API bool OS_DeleteFile(OS_String file_path);

// * The parent directory of the destination path must already exist.
OS_API bool OS_CloneDirectory(OS_String dst_directory_path, OS_String src_directory_path);

OS_API bool OS_FilePicker(OS_Arena* arena, OS_String* out_filepath);
OS_API bool OS_FolderPicker(OS_Arena* arena, OS_String* out_path);

// -- Directory Watch ---------------------------------------------------------

OS_API bool OS_InitDirectoryWatch(OS_DirectoryWatch* watch, OS_String directory_path);
OS_API void OS_DeinitDirectoryWatch(OS_DirectoryWatch* watch); // you may call this on a zero/deinitialized OS_DirectoryWatch

OS_API bool OS_DirectoryWatchHasChanges(OS_DirectoryWatch* watch);

// -- Threads -----------------------------------------------------------------

// NOTE: The `thread` pointer may not be moved or copied while in use.
// * if `debug_name` is NULL, no debug name will be specified
OS_API void OS_ThreadStart(OS_Thread* thread, OS_ThreadFn fn, void* user_data, OS_String* debug_name);
OS_API void OS_ThreadJoin(OS_Thread* thread);

OS_API void OS_MutexInit(OS_Mutex* mutex);
OS_API void OS_MutexDestroy(OS_Mutex* mutex);
OS_API void OS_MutexLock(OS_Mutex* mutex);
OS_API void OS_MutexUnlock(OS_Mutex* mutex);

// NOTE: A condition variable cannot be moved or copied while in use.
OS_API void OS_ConditionVarInit(OS_ConditionVar* condition_var);
OS_API void OS_ConditionVarDestroy(OS_ConditionVar* condition_var);
OS_API void OS_ConditionVarSignal(OS_ConditionVar* condition_var);
OS_API void OS_ConditionVarBroadcast(OS_ConditionVar* condition_var);
OS_API void OS_ConditionVarWait(OS_ConditionVar* condition_var, OS_Mutex* lock);
// OS_API void OS_ConditionVarWaitEx(OS_ConditionVar *condition_var, OS_Mutex *lock, Int wait_milliseconds);

//OS_API Semaphore *OS_SemaphoreCreate(); // Starts as 0
//OS_API void OS_SemaphoreDestroy(Semaphore *semaphore);
//OS_API void OS_SemaphoreWait(Semaphore *semaphore);
//OS_API void OS_SemaphoreWaitEx(Semaphore *semaphore, Int milliseconds);
//OS_API void OS_SemaphoreSignal(Semaphore *semaphore);

// -- Time --------------------------------------------------------------------

OS_API OS_Tick OS_Now();
OS_API double OS_DurationInSeconds(OS_Tick start, OS_Tick end);

// -- Window creation ---------------------------------------------------------

OS_API bool OS_InputIsDown(const OS_Inputs* inputs, OS_Input input);
OS_API bool OS_InputWasPressed(const OS_Inputs* inputs, OS_Input input);
OS_API bool OS_InputWasPressedOrRepeat(const OS_Inputs* inputs, OS_Input input);
OS_API bool OS_InputWasReleased(const OS_Inputs* inputs, OS_Input input);

/*
 NOTE: After calling `OS_WindowCreateNoShow`, the window will remain hidden.
 To show the window, you must explicitly call `OS_WindowShow`. This separation is there so that you
 can get rid of the initial flicker that normally happens when you create a window, then do some work
 such as initialize a graphics OS_API, and finally present a frame. If you instead first call `create`,
 then initialize the graphics OS_API, and only then call `show`, there won't be any flicker as the
 window doesn't have to wait for the initialization.
*/
OS_API OS_Window OS_WindowCreateHidden(uint32_t width, uint32_t height, OS_String name);
OS_API void OS_WindowShow_(OS_Window* window);

OS_API OS_Window OS_WindowCreate(uint32_t width, uint32_t height, OS_String name);

OS_API bool OS_SetFullscreen(OS_Window* window, bool fullscreen);

// * Returns `false` when the window is closed.
// * The reason we provide a callback for window resizes instead of returning the size is that, when resizing, Windows doesn't
//   let us exit the event loop until the user lets go of the mouse. This means that if you want to draw into the window
//   while resizing to make it feel smooth, you have to do it from a callback internally. So, if you want to draw while resizing,
//   you should do it inside `on_resize`.
// * on_resize may be NULL
// * inputs is a stateful data structure and should be initialized and kept around across multiple frames. TODO: refactor this and return an array of events instead.
OS_API bool OS_WindowPoll(OS_Arena* arena, OS_Inputs* inputs, OS_Window* window, OS_WindowOnResize on_resize, void* user_data);

OS_API void OS_SetMouseCursor(OS_MouseCursor cursor);
OS_API void OS_LockAndHideMouseCursor();

#ifdef /* ---------------- */ OS_IMPLEMENTATION /* ---------------- */

// -- Linker inputs --
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shell32.lib") // for SHFileOperationW
#pragma comment(lib, "Advapi32.lib") // for RegCloseKey
#pragma comment(lib, "Ole32.lib") // for CoCreateInstance
#pragma comment(lib, "OleAut32.lib") // for SysFreeString
#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Comdlg32.lib") // for GetOpenFileNameW
// --

#include <string.h>
#include <stdlib.h> // malloc, free

#define OS_SANE_MAX_PATH 1024

#define OS_Vec(T) struct { OS_Arena *arena; T *data; int length; int capacity; }
typedef OS_Vec(char) OS_VecRaw;

#define OS_VecPush(VEC, ...) do { \
	OS_VecReserveRaw((OS_VecRaw*)(VEC), (VEC)->length + 1, sizeof(*(VEC)->data)); \
	(VEC)->data[(VEC)->length++] = __VA_ARGS__; } while (0)

static void OS_VecReserveRaw(OS_VecRaw* array, int capacity, int elem_size) {
	int new_capacity = array->capacity;
	while (capacity > new_capacity) {
		new_capacity = new_capacity == 0 ? 8 : new_capacity * 2;
	}
	if (new_capacity != array->capacity) {
		char* new_data = OS_ArenaPush(array->arena, new_capacity * elem_size);
		memcpy(new_data, array->data, array->length * elem_size);
		array->data = new_data;
		array->capacity = new_capacity;
	}
}

#define OS_AlignUpPow2(x, p) (((x) + (p) - 1) & ~((p) - 1)) // e.g. (x=30, p=16) -> 32

OS_API void OS_ArenaInit(OS_Arena* arena, int block_size) {
#ifdef OS_CUSTOM_ARENA
	OS_ArenaInit_Custom(arena, block_size);
#else
	OS_Arena _arena = {0};
	_arena.block_size = block_size;
	*arena = _arena;
#endif
}

OS_API void OS_ArenaDeinit(OS_Arena* arena) {
#ifdef OS_CUSTOM_ARENA
	OS_ArenaDeinit_Custom(arena);
#else
	for (OS_ArenaBlockHeader* block = arena->first_block; block;) {
		OS_ArenaBlockHeader* next = block->next;
		free(block);
		block = next;
	}
#endif
}

OS_API char* OS_ArenaPush(OS_Arena* arena, int size/*, int alignment*/) {
#ifdef OS_CUSTOM_ARENA
	char* result = OS_ArenaPush_Custom(arena, size, sizeof(void*));
	return result;
#else
	int alignment = sizeof(void*);
	bool alignment_is_power_of_2 = ((alignment) & ((alignment)-1)) == 0;
	OS_CHECK(alignment != 0 && alignment_is_power_of_2);

	OS_ArenaBlockHeader* curr_block = arena->mark.block; // may be NULL
	void* curr_ptr = arena->mark.ptr;

	char* result_address = (char*)OS_AlignUpPow2((uintptr_t)curr_ptr, alignment);
	int remaining_space = curr_block ? curr_block->size_including_header - (int)((uintptr_t)result_address - (uintptr_t)curr_block) : 0;

	if (size > remaining_space) { // We need a new block!
		int result_offset = OS_AlignUpPow2(sizeof(OS_ArenaBlockHeader), alignment);
		int new_block_size = result_offset + size;
		if (arena->block_size > new_block_size) new_block_size = arena->block_size;

		OS_ArenaBlockHeader* new_block = NULL;
		OS_ArenaBlockHeader* next_block = NULL;

		// If there is a block at the end of the list that we have used previously, but aren't using anymore, then try to start using that one.
		if (curr_block && curr_block->next) {
			next_block = curr_block->next;

			int next_block_remaining_space = next_block->size_including_header - result_offset;
			if (size <= next_block_remaining_space) {
				new_block = next_block; // Next block has enough space, let's use it!
			}
		}

		// Otherwise, insert a new block.
		if (new_block == NULL) {
			new_block = (OS_ArenaBlockHeader*)malloc(new_block_size);
			new_block->size_including_header = new_block_size;
			new_block->next = next_block;

			if (curr_block) curr_block->next = new_block;
			else arena->first_block = new_block;
		}

		arena->mark.block = new_block;
		result_address = (char*)new_block + result_offset;
	}

	arena->mark.ptr = result_address + size;
	return result_address;
#endif
}

OS_API void OS_ArenaReset(OS_Arena* arena) {
#ifdef OS_CUSTOM_ARENA
	OS_ArenaReset_Custom(arena);
#else
	arena->mark.block = arena->first_block;
	arena->mark.ptr = (char*)arena->first_block + sizeof(OS_ArenaBlockHeader);
#endif
}

OS_API bool OS_WriteEntireFile(OS_String file_path, OS_String data) {
	OS_File file;
	if (!OS_FileOpen(file_path, OS_FileOpenMode_Write, &file)) return false;
	if (!OS_FileWrite(&file, data)) return false;
	OS_FileClose(&file);
	return true;
}

OS_API bool OS_ReadEntireFile(OS_Arena* arena, OS_String file_path, OS_String* out_str) {
	OS_File file;
	if (!OS_FileOpen(file_path, OS_FileOpenMode_Read, &file)) return false;

	uint64_t size = OS_FileSize(&file);
	if (size >= 2147483647) return false;

	char* data = OS_ArenaPush(arena, (int)size);

	bool ok = OS_FileRead(&file, data, (int)size) == (int)size;

	OS_FileClose(&file);
	out_str->data = data;
	out_str->size = (int)size;
	return ok;
}

OS_API bool OS_InputIsDown(const OS_Inputs* inputs, OS_Input input) {
	return inputs->input_states[input] & OS_InputStateFlag_IsDown;
}

OS_API bool OS_InputWasPressed(const OS_Inputs* inputs, OS_Input input) {
	return inputs->input_states[input] & OS_InputStateFlag_WasPressed;
}

OS_API bool OS_InputWasPressedOrRepeat(const OS_Inputs* inputs, OS_Input input) {
	return inputs->input_states[input] & OS_InputStateFlag_WasPressedOrRepeat;
}

OS_API bool OS_InputWasReleased(const OS_Inputs* inputs, OS_Input input) {
	return inputs->input_states[input] & OS_InputStateFlag_WasReleased;
}

static char* OS_StrToCStr(OS_Arena* arena, OS_String s) {
	char* data = OS_ArenaPush(arena, s.size + 1);
	memcpy(data, s.data, s.size);
	data[s.size] = 0;
	return data;
}

//#ifdef OS_WINDOWS

// - Windows implementation -------------------------------------------------------------------------

// from <process.h>:
uintptr_t _beginthreadex(void* security, unsigned stack_size, unsigned (*start_address)(void*), void* arglist, unsigned initflag, unsigned* thrdaddr);
void _endthreadex(unsigned retval);

#define UNICODE
#define CINTERFACE // for com macros
#include <Windows.h>
#include <shobjidl_core.h> // required for OS_FolderPicker
#include <DbgHelp.h>

// ----------- Global state -----------

static HANDLE OS_current_process;
static uint64_t OS_ticks_per_second;
static OS_Arena OS_temp_arena; // TODO: clean this up
static int OS_temp_arena_counter = 0;

// @cleanup
static OS_MouseCursor OS_current_cursor;
static HCURSOR OS_current_cursor_handle;
static bool OS_mouse_cursor_is_hidden;
static POINT OS_mouse_cursor_hidden_pos;
static bool OS_mouse_cursor_should_hide;

// ------------------------------------

static OS_Arena* OS_TempArenaBegin(void) {
	OS_temp_arena_counter++;
	return &OS_temp_arena;
}

static void OS_TempArenaEnd(void) {
	OS_temp_arena_counter--;
	OS_CHECK(OS_temp_arena_counter >= 0);
	if (OS_temp_arena_counter == 0) {
		OS_ArenaReset(&OS_temp_arena);
	}
}

OS_API void OS_Init(void) {
	QueryPerformanceFrequency((LARGE_INTEGER*)&OS_ticks_per_second);

	OS_current_process = GetCurrentProcess();
	SymInitialize(OS_current_process, NULL, true);

	OS_current_cursor = OS_MouseCursor_Arrow;
	OS_current_cursor_handle = LoadCursorW(NULL, (wchar_t*)IDC_ARROW);

	OS_ArenaInit(&OS_temp_arena, 1024);
}

OS_API void OS_Deinit(void) {
	OS_CHECK(OS_temp_arena_counter == 0);
	OS_ArenaDeinit(&OS_temp_arena);
}

OS_API OS_Tick OS_Now(void) {
	OS_Tick tick;
	QueryPerformanceCounter((LARGE_INTEGER*)&tick);
	return tick;
}

static void OS_ConvertPathSlashes(char* path, int path_length, char from, char to) {
	for (int i = 0; i < path_length; i++) {
		char* c = &path[i];
		if (*c == from) *c = to;
	}
}

static void OS_ConvertPathSlashesW(wchar_t* path, int path_length, wchar_t from, wchar_t to) {
	for (int i = 0; i < path_length; i++) {
		wchar_t* c = &path[i];
		if (*c == from) *c = to;
	}
}

static wchar_t* OS_StrToWide(OS_Arena* arena, OS_String str, int num_null_terminations, int* out_len) {
	if (str.size == 0) {
		if (out_len) *out_len = 0;
		return NULL;
	}
	OS_CHECK(str.size < INT_MAX);

	wchar_t* w_text = NULL;

	int w_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)str.data, (int)str.size, NULL, 0);
	w_text = (wchar_t*)OS_ArenaPush(arena, sizeof(wchar_t) * (w_len + num_null_terminations));

	int w_len1 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)str.data, (int)str.size, w_text, w_len);
	OS_CHECK(w_len != 0 && w_len1 == w_len);

	memset(&w_text[w_len], 0, num_null_terminations * sizeof(wchar_t));

	if (out_len) *out_len = w_len;
	return w_text;
}

static wchar_t* OS_StrToWideWin32Path(OS_Arena* arena, OS_String str, int num_null_terminations, int* out_len) {
	wchar_t* result = OS_StrToWide(arena, str, num_null_terminations, out_len);

	for (intptr_t i = 0; i < str.size; i++) {
		if (result[i] == L'/') result[i] = L'\\';
	}

	return result;
}

static OS_String OS_StrFromWide(OS_Arena* arena, wchar_t* str_wide) {
	if (*str_wide == 0) return OS_STR("");

	int length = WideCharToMultiByte(CP_UTF8, 0, str_wide, -1, NULL, 0, NULL, NULL);
	if (length <= 0) return OS_STR("");

	char* result_data = OS_ArenaPush(arena, length);

	int length2 = WideCharToMultiByte(CP_UTF8, 0, str_wide, -1, result_data, length, NULL, NULL);
	if (length2 <= 0) return OS_STR("");

	OS_String result = { result_data, length - 1 };
	return result;
}

OS_API void OS_MessageBox(OS_String title, OS_String message) {
	OS_Arena* temp = OS_TempArenaBegin();

	wchar_t* title_utf16 = OS_StrToWide(temp, title, 1, NULL);
	wchar_t* message_utf16 = OS_StrToWide(temp, message, 1, NULL);
	MessageBoxW(0, message_utf16, title_utf16, MB_OK);

	OS_TempArenaEnd();
}

OS_API double OS_DurationInSeconds(OS_Tick start, OS_Tick end) {
	OS_CHECK(end.tick >= start.tick);

	// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
	uint64_t elapsed = end.tick - start.tick;
	return (double)elapsed / (double)OS_ticks_per_second;
}

static void OS_ConsolePrint(OS_Log* self, OS_String str) { OS_PrintString(str); };

OS_API OS_Log* OS_Console() {
	static const OS_Log log = { OS_ConsolePrint };
	return (OS_Log*)&log;
};

static void OS_DebugLogPrint(OS_Log* self, OS_String str) { OS_DebugPrintString(str); };

OS_API OS_Log* OS_DebugLog() {
	static const OS_Log log = { OS_DebugLogPrint };
	return (OS_Log*)&log;
}

OS_API void OS_PrintString(OS_String str) {
	//Scope temp = ScopePush();

	//fuint str_wide_len;
	//wchar_t *str_wide = StrToWide(str, 1, temp.arena, &str_wide_len);
	//f_assert((uint32_t)str_wide_len == str_wide_len);

	// NOTE: it seems like when we're capturing the output from an outside program,
	//       using WriteConsoleW doesn't get captured, but WriteFile does.
	unsigned long num_chars_written;
	//WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str_wide, (uint32_t)str_wide_len * 2, &num_chars_written, NULL);

	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.data, (uint32_t)str.size, &num_chars_written, NULL);
	//OutputDebugStringW(
	//ScopePop(temp);
}

OS_API void OS_DebugPrintString(OS_String str) {
	OS_Arena* temp = OS_TempArenaBegin();

	wchar_t* str_wide = OS_StrToWide(temp, str, 1, NULL);
	OutputDebugStringW(str_wide);

	OS_TempArenaEnd();
}

// colored write to console
// WriteConsoleOutputAttribute
OS_API void OS_PrintStringColor(OS_String str, OS_ConsoleAttributeFlags attributes_mask) {
	if (str.size == 0) return;

	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	GetConsoleScreenBufferInfo(stdout_handle, &console_info);

	//SetConsoleTextAttribute(stdout_handle, attributes_mask);

	// @robustness: strings > 4GB
	unsigned long num_chars_written;
	WriteFile(stdout_handle, str.data, (uint32_t)str.size, &num_chars_written, NULL);

	//SetConsoleTextAttribute(stdout_handle, console_info.wAttributes); // Restore original state
}

OS_API uint64_t OS_ReadCycleCounter(void) {
	uint64_t counter = 0;
	BOOL res = QueryPerformanceCounter((LARGE_INTEGER*)&counter);
	OS_CHECK(res);
	return counter;
}

OS_API bool OS_FileModtime(OS_String file_path, OS_FileTime* out_modtime) {
	OS_Arena* temp = OS_TempArenaBegin();

	// TODO: use GetFileAttributesExW like https://github.com/mmozeiko/TwitchNotify/blob/master/TwitchNotify.c#L568-L583
	HANDLE h = CreateFileW(OS_StrToWide(temp, file_path, 1, NULL), 0, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (h != INVALID_HANDLE_VALUE) {
		GetFileTime(h, NULL, NULL, (FILETIME*)out_modtime);
		CloseHandle(h);
	}

	OS_TempArenaEnd();
	return h != INVALID_HANDLE_VALUE;
}

OS_API int OS_FileCmpModtime(OS_FileTime a, OS_FileTime b) {
	return CompareFileTime((FILETIME*)&a, (FILETIME*)&b);
}

OS_API bool OS_CloneFile(OS_String dst_file_path, OS_String src_file_path) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* src_filepath_w = OS_StrToWideWin32Path(temp, src_file_path, 1, NULL);
	wchar_t* dst_filepath_w = OS_StrToWideWin32Path(temp, dst_file_path, 1, NULL);
	BOOL ok = CopyFileW(src_filepath_w, dst_filepath_w, 0) == 1;
	OS_TempArenaEnd();
	return ok;
}

OS_API bool OS_CloneDirectory(OS_String dst_directory_path, OS_String src_directory_path) {
	OS_Arena* temp = OS_TempArenaBegin();

	SHFILEOPSTRUCTW s = {0};
	s.wFunc = FO_COPY;
	s.pTo = OS_StrToWide(temp, dst_directory_path, 2, NULL);
	s.pFrom = OS_StrToWide(temp, src_directory_path, 2, NULL);
	s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
	int result = SHFileOperationW(&s);

	OS_TempArenaEnd();
	return result == 0;
}

OS_API bool OS_DeleteFile(OS_String file_path) {
	OS_Arena* temp = OS_TempArenaBegin();
	bool ok = DeleteFileW(OS_StrToWide(temp, file_path, 1, NULL)) == 1;
	OS_TempArenaEnd();
	return ok;
}

OS_API void OS_Sleep(uint64_t ms) {
	OS_CHECK(ms < UINT_MAX); // TODO
	Sleep((uint32_t)ms);
}

OS_API bool OS_LoadDLL(OS_String dll_path, OS_DynamicLibrary* out_dll) {
	OS_Arena* temp = OS_TempArenaBegin();

	HANDLE handle = LoadLibraryW(OS_StrToWide(temp, dll_path, 1, NULL));
	out_dll->handle = handle;

	OS_TempArenaEnd();
	return handle != NULL;
}

OS_API bool OS_InitDirectoryWatch(OS_DirectoryWatch* watch, OS_String directory_path) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* directory_wide = OS_StrToWide(temp, directory_path, 1, NULL);

	HANDLE handle = FindFirstChangeNotificationW(directory_wide, TRUE,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY);
	watch->handle = handle;

	OS_TempArenaEnd();
	return handle != INVALID_HANDLE_VALUE;
}

OS_API void OS_DeinitDirectoryWatch(OS_DirectoryWatch* watch) {
	if (watch->handle) {
		OS_CHECK(FindCloseChangeNotification(watch->handle));
		watch->handle = 0;
	}
}

OS_API bool OS_DirectoryWatchHasChanges(OS_DirectoryWatch* watch) {
	DWORD wait = WaitForSingleObject((HANDLE)watch->handle, 0);
	if (wait == WAIT_OBJECT_0) {
		OS_CHECK(FindNextChangeNotification((HANDLE)watch->handle));
	}
	return wait == WAIT_OBJECT_0;
}

OS_API void OS_UnloadDLL(OS_DynamicLibrary dll) {
	OS_CHECK(FreeLibrary((HINSTANCE)dll.handle));
}

OS_API void* OS_GetSymbolAddress(OS_DynamicLibrary dll, OS_String symbol) {
	OS_Arena* temp = OS_TempArenaBegin();
	void* addr = GetProcAddress((HINSTANCE)dll.handle, OS_StrToCStr(temp, symbol));
	OS_TempArenaEnd();
	return addr;
}

OS_API bool OS_FolderPicker(OS_Arena* arena, OS_String* out_path) {
	bool ok = false;
	if (SUCCEEDED(CoInitialize(NULL))) {
		IFileDialog* dialog;

#ifdef __cplusplus
		HRESULT instance_result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void**)&dialog);
#else
		HRESULT instance_result = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, &dialog);
#endif

		if (SUCCEEDED(instance_result)) {
			// NOTE: for the following to compile, the CINTERFACE macro must be defined before the Windows COM headers are included!

			dialog->lpVtbl->SetOptions(dialog, FOS_PICKFOLDERS);
			dialog->lpVtbl->Show(dialog, NULL);

			IShellItem* dialog_result;
			if (SUCCEEDED(dialog->lpVtbl->GetResult(dialog, &dialog_result))) {
				wchar_t* path_wide = NULL;
				if (SUCCEEDED(dialog_result->lpVtbl->GetDisplayName(dialog_result, SIGDN_FILESYSPATH, &path_wide))) {
					ok = true;

					OS_String result = OS_StrFromWide(arena, path_wide);
					OS_ConvertPathSlashes((char*)result.data, result.size, '\\', '/');
					*out_path = result;

					CoTaskMemFree(path_wide);
				}
				dialog_result->lpVtbl->Release(dialog_result);
			}
			dialog->lpVtbl->Release(dialog);
		}
		CoUninitialize();
	}
	return ok;
}

OS_API bool OS_FilePicker(OS_Arena* arena, OS_String* out_filepath) {
	wchar_t buffer[OS_SANE_MAX_PATH];
	buffer[0] = 0;

	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = OS_SANE_MAX_PATH - 1;
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	GetOpenFileNameW(&ofn);

	*out_filepath = OS_StrFromWide(arena, buffer);
	return out_filepath->size > 0;
}

//Slice(OS_String) os_file_picker_multi() {
//	ZoneScoped;
//	ASSERT(false); // cancelling does not work!!!!
//	return {};

//Slice(uint8_t) buffer = mem_alloc(16*KB, TEMP_ALLOCATOR);
//buffer[0] = '\0';
//
//OPENFILENAMEA ofn = {};
//ofn.lStructSize = sizeof(ofn);
//ofn.hwndOwner = NULL;
//ofn.lpstrFile = (char*)buffer.data;
//ofn.nMaxFile = (uint32_t)buffer.length - 1;
//ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
//ofn.nFilterIndex = 1;
//ofn.lpstrFileTitle = NULL;
//ofn.nMaxFileTitle = 0;
//ofn.lpstrInitialDir = NULL;
//ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_NOCHANGEDIR;
//GetOpenFileNameA(&ofn);
//
//Vec(OS_String) items = {};
//items.arena = TEMP_ALLOCATOR;
//
//OS_String directory = OS_String{ buffer.data, (isize)strlen((char*)buffer.data) };
//uint8_t *ptr = buffer.data + directory.length + 1;
//
//if (*ptr == NULL) {
//	if (directory.length == 0) return {};
//	
//	array_append(items, directory);
//	return items.slice;
//}
//
//int64_t i = 0;
//while (*ptr) {
//	OS_String filename = OS_String{ ptr, (isize)strlen((char*)ptr) };
//
//	OS_String fullpath = str_join(slice({ directory, LIT("\\"), filename }), TEMP_ALLOCATOR);
//	assert(fullpath.length < 270);
//	array_append(items, fullpath);
//
//	ptr += filename.length + 1;
//	i++;
//	assert(i < 1000);
//}
//
//return items.slice;
//}

OS_API bool OS_SetWorkingDir(OS_String dir) {
	OS_Arena* temp = OS_TempArenaBegin();
	BOOL ok = SetCurrentDirectoryW(OS_StrToWide(temp, dir, 1, NULL));
	OS_TempArenaEnd();
	return ok;
}

OS_API OS_String OS_GetWorkingDir(OS_Arena* arena) {
	wchar_t buf[OS_SANE_MAX_PATH];
	buf[0] = 0;
	GetCurrentDirectoryW(OS_SANE_MAX_PATH, buf);
	return OS_StrFromWide(arena, buf);
}

OS_API OS_String OS_GetExecutablePath(OS_Arena* arena) {
	wchar_t buf[OS_SANE_MAX_PATH];
	uint32_t n = GetModuleFileNameW(NULL, buf, OS_SANE_MAX_PATH);
	OS_CHECK(n > 0 && n < OS_SANE_MAX_PATH);
	return OS_StrFromWide(arena, buf);
}

OS_API OS_String OS_ClipboardGetText(OS_Arena* arena) {
	char* text = NULL;
	int length = 0;

	if (OpenClipboard(NULL)) {
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);

		GlobalLock(hData);

		length = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)hData, -1, NULL, 0, NULL, NULL);
		if (length > 0) {
			text = OS_ArenaPush(arena, length);
			WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)hData, -1, text, length, NULL, NULL);
			length -= 1;
		}

		GlobalUnlock(hData);
		CloseClipboard();
	}

	OS_String result = { text, length };
	return result;
}

OS_API void OS_ClipboardSetText(OS_String text) {
	if (!OpenClipboard(NULL)) return;
	OS_Arena* temp = OS_TempArenaBegin();
	if (text.size > 0) {
		EmptyClipboard();

		int utf16_len;
		wchar_t* utf16 = OS_StrToWide(temp, text, 1, &utf16_len);

		HANDLE clipbuffer = GlobalAlloc(0, utf16_len * 2 + 2);
		char* buffer = (char*)GlobalLock(clipbuffer);
		memcpy(buffer, utf16, utf16_len * 2 + 2);

		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_UNICODETEXT, clipbuffer);

		CloseClipboard();
	}
	OS_TempArenaEnd();
}

OS_API bool OS_DirectoryExists(OS_String directory_path) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* path_utf16 = OS_StrToWide(temp, directory_path, 1, NULL);
	uint32_t dwAttrib = GetFileAttributesW(path_utf16);
	OS_TempArenaEnd();
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

OS_API bool OS_PathIsAbsolute(OS_String path) {
	return path.size > 2 && path.data[1] == ':';
}

OS_API bool OS_PathToCanonical(OS_Arena* arena, OS_String path, OS_String* out_canonical) {
	// https://pdh11.blogspot.com/2009/05/pathcanonicalize-versus-what-it-says-on.html
	// https://stackoverflow.com/questions/10198420/open-directory-using-createfile

	OS_Arena* temp = OS_TempArenaBegin();

	wchar_t* path_utf16 = OS_StrToWide(temp, path, 1, NULL);

	HANDLE file_handle = CreateFileW(path_utf16, 0, 0, NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	bool dummy_file_was_created = GetLastError() == 0;
	bool ok = file_handle != INVALID_HANDLE_VALUE;

	wchar_t result_utf16[OS_SANE_MAX_PATH];
	if (ok) ok = GetFinalPathNameByHandleW(file_handle, result_utf16, OS_SANE_MAX_PATH, VOLUME_NAME_DOS) < OS_SANE_MAX_PATH;

	if (file_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(file_handle);
	}

	if (dummy_file_was_created) {
		DeleteFileW(result_utf16);
	}

	OS_String result = {0};
	if (ok) {
		result = OS_StrFromWide(arena, result_utf16);
		result.data += 4; result.size -= 4; // strings returned have `\\?\` - prefix that we should get rid of
		*out_canonical = result;
	}

	if (arena != temp) OS_TempArenaEnd();
	return ok;
}

OS_API bool OS_GetAllFilesInDirectory(OS_Arena* arena, OS_String directory, OS_FileInfoArray* out_files) {
	OS_Arena* temp = OS_TempArenaBegin();

	char* match_cstr = OS_ArenaPush(temp, directory.size + 2);
	memcpy(match_cstr, directory.data, directory.size);
	match_cstr[directory.size] = '\\';
	match_cstr[directory.size + 1] = '*';
	OS_String match_str = { match_cstr, directory.size + 2 };
	wchar_t* match_wstr = OS_StrToWide(temp, match_str, 1, NULL);

	OS_Vec(OS_FileInfo) file_infos = { arena };

	WIN32_FIND_DATAW find_info;
	HANDLE handle = FindFirstFileW(match_wstr, &find_info);

	bool ok = handle != INVALID_HANDLE_VALUE;
	if (ok) {
		for (; FindNextFileW(handle, &find_info);) {
			OS_FileInfo info = {0};
			info.name = OS_StrFromWide(arena, find_info.cFileName);
			info.is_directory = find_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			info.last_write_time.os_specific = *(uint64_t*)&find_info.ftLastWriteTime;
			if (info.name.size == 2 && info.name.data[0] == '.' && info.name.data[1] == '.') continue;

			OS_VecPush(&file_infos, info);
		}
		ok = GetLastError() == ERROR_NO_MORE_FILES;
		FindClose(handle);
	}

	out_files->data = file_infos.data;
	out_files->length = file_infos.length;
	OS_TempArenaEnd();
	return ok;
}

OS_API bool OS_DeleteDirectory(OS_String directory_path) {
	if (!OS_DirectoryExists(directory_path)) return true;
	OS_Arena* temp = OS_TempArenaBegin();

	SHFILEOPSTRUCTW file_op = {0};
	file_op.hwnd = NULL;
	file_op.wFunc = FO_DELETE;
	file_op.pFrom = OS_StrToWide(temp, directory_path, 2, NULL); // NOTE: pFrom must be double null-terminated!
	file_op.pTo = NULL;
	file_op.fFlags = FOF_NO_UI;
	file_op.fAnyOperationsAborted = false;
	file_op.hNameMappings = 0;
	file_op.lpszProgressTitle = NULL;
	int res = SHFileOperationW(&file_op);

	OS_TempArenaEnd();
	return res == 0;
}

OS_API bool OS_MakeDirectory(OS_String directory_path) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* path_utf16 = OS_StrToWide(temp, directory_path, 1, NULL);
	bool created = CreateDirectoryW(path_utf16, NULL);
	OS_TempArenaEnd();

	if (created) return true;
	return GetLastError() == ERROR_ALREADY_EXISTS;
}

OS_API bool OS_FileOpen(OS_String file_path, OS_FileOpenMode mode, OS_File* out_file) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* filepath_utf16 = OS_StrToWide(temp, file_path, 1, NULL);

	out_file->mode = mode;

	if (mode == OS_FileOpenMode_Read) {
		out_file->os_handle = CreateFileW(filepath_utf16, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	}
	else {
		uint32_t creation = mode == OS_FileOpenMode_Append ? OPEN_ALWAYS : CREATE_ALWAYS;
		out_file->os_handle = CreateFileW(filepath_utf16, FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, creation, 0, NULL);
	}

	bool ok = out_file->os_handle != INVALID_HANDLE_VALUE;
	if (ok) {
		//f_leak_tracker_begin_entry(out_file->os_handle, 1);

		//if (mode != OS_FileOpenMode_Read) {
		//	TODO(); // out_file->backing_writer = (Writer){OS_FileUnbufferedWriterFn, out_file};
		//	// OpenBufferedWriter(&out_file->backing_writer, &out_file->buffer[0], FILE_WRITER_BUFFER_SIZE, &out_file->buffered_writer);
		//}
	}

	OS_TempArenaEnd();
	return ok;
}

OS_API size_t OS_FileRead(OS_File* file, void* dst, size_t size) {
	if (dst == NULL) return 0;
	if (size == 0) return 0;

	for (size_t read_so_far = 0; read_so_far < size;) {
		size_t remaining = size - read_so_far;
		uint32_t to_read = remaining >= UINT_MAX ? UINT_MAX : (uint32_t)remaining;

		unsigned long bytes_read;
		BOOL ok = ReadFile(file->os_handle, (char*)dst + read_so_far, to_read, &bytes_read, NULL);
		read_so_far += bytes_read;

		if (ok != 1 || bytes_read < to_read) {
			return read_so_far;
		}
	}
	return size;
}

OS_API uint64_t OS_FileSize(OS_File* file) {
	uint64_t size;
	if (GetFileSizeEx(file->os_handle, (LARGE_INTEGER*)&size) != 1) return -1;
	return size;
}

OS_API bool OS_FileWrite(OS_File* file, OS_String data) {
	if (data.size >= UINT_MAX) return false; // TODO: writing files greater than 4 GB

	unsigned long bytes_written;
	return WriteFile(file->os_handle, data.data, (uint32_t)data.size, &bytes_written, NULL) == 1 && bytes_written == data.size;
}

OS_API uint64_t OS_FileGetCursor(OS_File* file) {
	uint64_t offset;
	LARGE_INTEGER move = {0};
	if (SetFilePointerEx(file->os_handle, move, (LARGE_INTEGER*)&offset, FILE_CURRENT) != 1) return -1;
	return offset;
}

OS_API bool OS_FileSetCursor(OS_File* file, uint64_t position) {
	return SetFilePointerEx(file->os_handle, *(LARGE_INTEGER*)&position, NULL, FILE_BEGIN) == 1;
}

//OS_API void OS_FileFlush(OS_File *file) {
//	OS_CHECK(file->mode != OS_FileOpenMode_Read);
//	TODO(); //FlushBufferedWriter(&file->buffered_writer);
//}

OS_API bool OS_FileClose(OS_File* file) {
	//if (file->mode != OS_FileOpenMode_Read) {
	//	OS_FileFlush(file);
	//}

	bool ok = CloseHandle(file->os_handle) == 1;
	//f_leak_tracker_end_entry(file->os_handle);
	return ok;
}

OS_API OS_DebugStackFrameArray OS_DebugGetStackTrace(OS_Arena* arena) {
	// NOTE: for any of this to work, `SymInitialize` must have been called! We do this in OS_Init().

	CONTEXT ctx;
	RtlCaptureContext(&ctx);

	STACKFRAME64 stack_frame = {0};
	stack_frame.AddrPC.Offset = ctx.Rip;
	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Offset = ctx.Rsp;
	stack_frame.AddrFrame.Mode = AddrModeFlat;
	stack_frame.AddrStack.Offset = ctx.Rsp;
	stack_frame.AddrStack.Mode = AddrModeFlat;

	OS_DebugStackFrame frames[64];
	int frames_count = 0;

	for (size_t i = 0; i < 64; i++) {
		if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, OS_current_process, GetCurrentThread(), &stack_frame, &ctx,
			NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
			// Maybe it failed, maybe we have finished walking the stack.
			break;
		}

		if (stack_frame.AddrPC.Offset == 0) break;
		if (i == 0) continue; // ignore this function

		struct {
			IMAGEHLP_SYMBOL64 s;
			char name_buf[64];
		} sym;

		sym.s.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		sym.s.MaxNameLength = 64;

		if (SymGetSymFromAddr64(OS_current_process, stack_frame.AddrPC.Offset, NULL, &sym.s) != TRUE) break;

		IMAGEHLP_LINE64 Line64;
		Line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		unsigned long dwDisplacement;
		if (SymGetLineFromAddr64(OS_current_process, ctx.Rip, &dwDisplacement, &Line64) != TRUE) break;

		// TODO: figure out what to do with arenas and arena customization...
		// Using stuff like StrClone is obviously something I do want to do, but that adds a dependency on fire_ds.h arenas...

		int function_len = (int)strlen(sym.s.Name);
		char* function = OS_ArenaPush(arena, function_len);
		memcpy(function, sym.s.Name, function_len);

		int filename_len = (int)strlen(Line64.FileName);
		char* filename = OS_ArenaPush(arena, filename_len);
		memcpy(filename, Line64.FileName, filename_len);

		OS_DebugStackFrame frame = {0};
		frame.function.data = function;
		frame.function.size = function_len;
		frame.file.data = filename;
		frame.file.size = filename_len;
		frame.line = Line64.LineNumber;
		frames[frames_count++] = frame;

		if (strcmp(sym.s.Name, "main") == 0) break; // Ignore everything past main
	}

	OS_DebugStackFrameArray result = {0};
	result.data = (OS_DebugStackFrame*)OS_ArenaPush(arena, sizeof(OS_DebugStackFrame) * frames_count);
	result.length = frames_count;
	memcpy(result.data, frames, sizeof(OS_DebugStackFrame) * frames_count);
	return result;
}

OS_API void OS_OpenFileInDefaultProgram(OS_String file) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* name_wide = OS_StrToWide(temp, file, 1, NULL);
	ShellExecuteW(NULL, NULL, name_wide, NULL, NULL, SW_SHOW);
	//ShellExecuteW(NULL, NULL, name_wide, NULL, NULL, SW_NORMAL);
	//ShellExecuteW(NULL, L"Open", name_wide, NULL, NULL, SW_SHOWNORMAL);
	OS_TempArenaEnd();
}

OS_API OS_String OS_FindExecutable(OS_Arena* arena, OS_String name) {
	OS_Arena* temp = OS_TempArenaBegin();
	wchar_t* name_wide = OS_StrToWide(temp, name, 1, NULL);

	wchar_t path[OS_SANE_MAX_PATH] = {0};
	uint32_t dwRet = SearchPathW(NULL, name_wide, NULL, OS_SANE_MAX_PATH, path, NULL);
	if (arena != temp) OS_TempArenaEnd(); // TODO: this leaks memory.

	if (dwRet == 0 || dwRet >= OS_SANE_MAX_PATH) {
		return OS_STR("");
	}
	return OS_StrFromWide(arena, path);
}

OS_API bool OS_RunCommand(OS_String* args, uint32_t args_count, uint32_t* out_exit_code, OS_Log* stdout_log, OS_Log* stderr_log) {
	OS_Arena* temp = OS_TempArenaBegin();

	// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

	// Windows expects a single space-separated string that encodes a list of the passed command-line arguments.
	// In order to support spaces within an argument, we must enclose it with quotation marks (").
	// This escaping method is the stupidest things.
	// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?redirectedfrom=MSDN&view=msvc-170
	// https://stackoverflow.com/questions/1291291/how-to-accept-command-line-args-ending-in-backslash
	// https://daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC

	OS_Vec(char) cmd_string_buf = { temp };

	for (uint32_t i = 0; i < args_count; i++) {
		OS_String arg = args[i];

		bool contains_space = false;

		OS_Vec(char) arg_string = { temp };

		for (intptr_t j = 0; j < arg.size; j++) {
			char c = arg.data[j];
			if (c == '\"') {
				OS_VecPush(&arg_string, '\\'); // escape quotation marks with a backslash
			}
			else if (c == '\\') {
				if (j + 1 == arg.size) {
					// if we have a backslash and it's the last character in the string,
					// we must push \\"
					OS_VecPush(&arg_string, '\\');
					OS_VecPush(&arg_string, '\\');
					break;
				}
				else if (arg.data[j + 1] == '\"') {
					// if we have a backslash and the next character is a quotation mark,
					// we must push \\\"
					OS_VecPush(&arg_string, '\\');
					OS_VecPush(&arg_string, '\\');
					OS_VecPush(&arg_string, '\\');
					OS_VecPush(&arg_string, '\"');
					j++; // also skip the next "
					continue;
				}
			}
			else if (c == ' ') {
				contains_space = true;
			}

			OS_VecPush(&arg_string, c);
		}

		int test = 0;
		(void)test;

		// Only add quotations if the argument contains spaces. You might ask why this matters: couldn't we always just add the quotation?
		// And yes, if the program is only using `argc` and `argv` style parameters, it does not matter. However, if it uses the GetCommandLine OS_API or
		// the `lpCmdLine` parameter in WinMain, then the argument string will be used as a single string and parsed manually by the program.
		// And so, some programs, for example Visual Studio (devenv.exe), will fail to parse arguments correctly if they're quoted.
		if (contains_space) OS_VecPush(&cmd_string_buf, '\"');
		for (int j = 0; j < arg_string.length; j++) {
			OS_VecPush(&cmd_string_buf, arg_string.data[j]);
		}
		if (contains_space) OS_VecPush(&cmd_string_buf, '\"');

		if (i < args_count - 1) OS_VecPush(&cmd_string_buf, ' '); // Separate each argument with a space
	}

	OS_String cmd_string = { cmd_string_buf.data, cmd_string_buf.length };
	wchar_t* cmd_string_utf16 = OS_StrToWide(temp, cmd_string, 1, NULL);

	PROCESS_INFORMATION process_info = {0};

	// Initialize pipes
	SECURITY_ATTRIBUTES security_attrs = {0};
	security_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attrs.lpSecurityDescriptor = NULL;
	security_attrs.bInheritHandle = 1;

	//HANDLE IN_Rd = NULL,  IN_Wr = NULL;
	HANDLE OUT_Rd = NULL, OUT_Wr = NULL;
	HANDLE ERR_Rd = NULL, ERR_Wr = NULL;

	bool ok = true;
	if (ok) ok = CreatePipe(&OUT_Rd, &OUT_Wr, &security_attrs, 0);
	if (ok) ok = SetHandleInformation(OUT_Rd, HANDLE_FLAG_INHERIT, 0);

	if (ok) ok = CreatePipe(&ERR_Rd, &ERR_Wr, &security_attrs, 0);
	if (ok) ok = SetHandleInformation(ERR_Rd, HANDLE_FLAG_INHERIT, 0);

	// let's not support stdin, at least for now.
	// if (ok) ok = CreatePipe(&IN_Rd, &IN_Wr, &security_attrs, 0);
	// if (ok) ok = SetHandleInformation(IN_Rd, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOW startup_info = {0};
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.dwFlags = STARTF_USESTDHANDLES;
	startup_info.hStdOutput = OUT_Wr;
	startup_info.hStdError = stdout_log == stderr_log ? OUT_Wr : ERR_Wr; // When redirecting both stdout AND stderr into the same writer, we want to use the same pipe for both of them to get proper order in the output.
	startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	if (ok) ok = CreateProcessW(NULL, cmd_string_utf16, NULL, NULL, true, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &startup_info, &process_info);

	// We don't need these handles for ourselves and we must close them to say that we won't be using them, to let `ReadFile` exit
	// when the process finishes instead of locking. At least that's how I think it works.
	if (OUT_Wr) CloseHandle(OUT_Wr);
	if (ERR_Wr) CloseHandle(ERR_Wr);

	if (ok) {
		char buf[512];
		unsigned long num_read_bytes;

		for (;;) {
			if (!ReadFile(OUT_Rd, buf, sizeof(buf), &num_read_bytes, NULL)) break;
			if (stdout_log) {
				OS_String str = { buf, (int)num_read_bytes };
				stdout_log->print(stdout_log, str);
			}
		}

		for (;;) {
			if (!ReadFile(ERR_Rd, buf, sizeof(buf), &num_read_bytes, NULL)) break;
			if (stderr_log && stdout_log != stderr_log) {
				OS_String str = { buf, (int)num_read_bytes };
				stderr_log->print(stderr_log, str);
			}
		}

		WaitForSingleObject(process_info.hProcess, INFINITE);

		unsigned long exit_code;
		ok = ok && GetExitCodeProcess(process_info.hProcess, &exit_code);

		if (out_exit_code) *out_exit_code = exit_code;

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);
	}

	if (OUT_Rd) CloseHandle(OUT_Rd);
	if (ERR_Rd) CloseHandle(ERR_Rd);

	OS_TempArenaEnd();
	return ok;
}

// Window creation & input ------------------

typedef struct WindowProcUserData {
	OS_Window* window;
	OS_Inputs* inputs;

	OS_Vec(uint32_t) text_input_utf32;

	OS_WindowOnResize on_resize; // may be NULL
	void* user_data;

	bool should_exit;
} WindowProcUserData;

static void UpdateKeyInputState(OS_Inputs* inputs, uint64_t vk, OS_InputStateFlags flags) {
	OS_Input input = OS_Input_Invalid;

	if ((vk >= OS_Input_0 && vk <= OS_Input_9) || (vk >= OS_Input_A && vk <= OS_Input_Z)) {
		input = (OS_Input)vk;
	}
	else if (vk >= VK_F1 && vk <= VK_F24) {
		input = (OS_Input)(OS_Input_F1 + (vk - VK_F1));
	}
	else switch (vk) {
	case VK_SPACE: input = OS_Input_Space; break;
	case VK_OEM_7: input = OS_Input_Apostrophe; break;
	case VK_OEM_COMMA: input = OS_Input_Comma; break;
	case VK_OEM_MINUS: input = OS_Input_Minus; break;
	case VK_OEM_PERIOD: input = OS_Input_Period; break;
	case VK_OEM_2: input = OS_Input_Slash; break;
	case VK_OEM_1: input = OS_Input_Semicolon; break;
	case VK_OEM_PLUS: input = OS_Input_Equal; break;
	case VK_OEM_4: input = OS_Input_LeftBracket; break;
	case VK_OEM_5: input = OS_Input_Backslash; break;
	case VK_OEM_6: input = OS_Input_RightBracket; break;
	case VK_OEM_3: input = OS_Input_GraveAccent; break;
	case VK_ESCAPE: input = OS_Input_Escape; break;
	case VK_RETURN: input = OS_Input_Enter; break;
	case VK_TAB: input = OS_Input_Tab; break;
	case VK_BACK: input = OS_Input_Backspace; break;
	case VK_INSERT: input = OS_Input_Insert; break;
	case VK_DELETE: input = OS_Input_Delete; break;
	case VK_RIGHT: input = OS_Input_Right; break;
	case VK_LEFT: input = OS_Input_Left; break;
	case VK_DOWN: input = OS_Input_Down; break;
	case VK_UP: input = OS_Input_Up; break;
	case VK_PRIOR: input = OS_Input_PageUp; break;
	case VK_NEXT: input = OS_Input_PageDown; break;
	case VK_HOME: input = OS_Input_Home; break;
	case VK_END: input = OS_Input_End; break;
	case VK_CAPITAL: input = OS_Input_CapsLock; break;
	case VK_NUMLOCK: input = OS_Input_NumLock; break;
	case VK_SNAPSHOT: input = OS_Input_PrintScreen; break;
	case VK_PAUSE: input = OS_Input_Pause; break;
	case VK_SHIFT: {
		// TODO: I think we can also detect the right-vs-left from lParam bit 24
		input = GetKeyState(VK_RSHIFT) & 0x8000 ? OS_Input_RightShift : OS_Input_LeftShift;
		inputs->input_states[OS_Input_Shift] = flags;
	} break;
	case VK_MENU: {
		input = GetKeyState(VK_RMENU) & 0x8000 ? OS_Input_RightAlt : OS_Input_LeftAlt;
		inputs->input_states[OS_Input_Alt] = flags;
	} break;
	case VK_CONTROL: {
		input = GetKeyState(VK_RCONTROL) & 0x8000 ? OS_Input_RightControl : OS_Input_LeftControl;
		inputs->input_states[OS_Input_Control] = flags;
	} break;
	case VK_LWIN: {
		input = OS_Input_LeftSuper;
		inputs->input_states[OS_Input_Super] = flags;
	} break;
	case VK_RWIN: {
		input = OS_Input_RightSuper;
		inputs->input_states[OS_Input_Super] = flags;
	} break;

	default: break;
	}
	inputs->input_states[input] = flags;
}

static int64_t OS_WindowProc(HWND hWnd, uint32_t uMsg, uint64_t wParam, int64_t lParam) {
	// `passed` will be NULL during window creation
	WindowProcUserData* passed = (WindowProcUserData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

	int64_t result = 0;
	if (passed && (hWnd == (HWND)passed->window->handle)) {
		OS_Inputs* inputs = passed->inputs;
		switch (uMsg) {
		default: {
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;
		case WM_INPUT: {
			RAWINPUT raw_input = {0};
			uint32_t dwSize = sizeof(RAWINPUT);
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw_input, &dwSize, sizeof(RAWINPUTHEADER));

			// Avoid using arena allocations here to get more deterministic counters for debugging.
			//uint32_t dwSize;
			//GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
			//
			//Scope T = ScopePush(NULL);
			//RAWINPUT *raw_input = MemAlloc(dwSize, temp);
			//if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw_input, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
			//	OS_CHECK(0);
			//}

			if (raw_input.header.dwType == RIM_TYPEMOUSE &&
				!(raw_input.data.mouse.usFlags & MOUSE_MOVE_RELATIVE) &&
				!(raw_input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) // For instance, drawing tables can give absolute mouse inputs which we want to ignore
				) {
				inputs->mouse_raw_dx = raw_input.data.mouse.lLastX;
				inputs->mouse_raw_dy = raw_input.data.mouse.lLastY;
			}
			
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;

		case WM_CHAR: {
			// wParam contains the character as a 32-bit unicode codepoint.
			if (wParam >= 32 && wParam != 127 /* ignore the DEL character on ctrl-backspace */)
			{
				OS_VecPush(&passed->text_input_utf32, (uint32_t)wParam);
			}
		} break;

		case WM_SETCURSOR: {
			if (LOWORD(lParam) == HTCLIENT) {
				SetCursor(OS_current_cursor_handle);
			}
			else {
				result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}
		} break;

		case WM_SIZE: {
			// "The low-order word of lParam specifies the new width of the client area."
			// "The high-order word of lParam specifies the new height of the client area."

			uint32_t width = (uint32_t)(lParam) & 0xFFFF;
			uint32_t height = (uint32_t)(lParam >> 16);
			if (passed->on_resize) {
				passed->on_resize(width, height, passed->user_data);
			}
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;

		case WM_CLOSE: // fallthrough
		case WM_QUIT: {
			passed->should_exit = true;
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;

		case WM_SYSKEYDOWN: // fallthrough
		case WM_KEYDOWN: {
			bool is_repeat = (lParam & (1 << 30)) != 0;

			OS_InputStateFlags flags = OS_InputStateFlag_IsDown | OS_InputStateFlag_WasPressedOrRepeat;
			if (!is_repeat) flags |= OS_InputStateFlag_WasPressed;

			//OS_InputStateFlags flags = is_repeat ?
			//	OS_InputStateFlag_IsPressed | OS_InputStateFlag_DidBeginPressOrRepeat :
			//	OS_InputStateFlag_IsPressed | OS_InputStateFlag_DidBeginPress;

			UpdateKeyInputState(inputs, wParam, flags);
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;
		case WM_SYSKEYUP: // fallthrough
		case WM_KEYUP: {
			// Right now, if you press and release a key within a single frame, our input system will miss that.
			// TODO: either make it an event list, or fix this (but still disallow lose information about multiple keypresses per frame).
			UpdateKeyInputState(inputs, wParam, OS_InputStateFlag_WasReleased);
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;

			// NOTE: releasing mouse buttons is handled manually :ManualMouseReleaseEvents
		case WM_LBUTTONDOWN: { inputs->input_states[OS_Input_MouseLeft] = OS_InputStateFlag_WasPressed | OS_InputStateFlag_IsDown; } break;
		case WM_RBUTTONDOWN: { inputs->input_states[OS_Input_MouseRight] = OS_InputStateFlag_WasPressed | OS_InputStateFlag_IsDown; } break;
		case WM_MBUTTONDOWN: { inputs->input_states[OS_Input_MouseMiddle] = OS_InputStateFlag_WasPressed | OS_InputStateFlag_IsDown; } break;

		case WM_MOUSEWHEEL: {
			int16_t wheel = (int16_t)(wParam >> 16);
			inputs->mouse_wheel_delta = (float)wheel / (float)WHEEL_DELTA;
		} break;

		case WM_COPYDATA: {
			__debugbreak();
		} break;
		}
	}
	else {
		result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	return result;
}

#define OS_WINDOW_CLASS_NAME L"OS_WindowModuleClassName"

static void OS_RegisterWindowClass() {
	// Windows requires us to first register a "window class" with a given name,
	// which will be used in subsequent calls to CreateWindowExW()

	HINSTANCE hInst = GetModuleHandleW(NULL);

	//#COLOR_BACKGROUND: 1
	//bg_brush: HBRUSH(COLOR_BACKGROUND)

	// leave the background brush to NULL
	// https://stackoverflow.com/questions/6593014/how-to-draw-opengl-content-while-resizing-win32-window
	//wnd_class.hbrBackground

	WNDCLASSEXW wnd_class = {0};
	wnd_class.cbSize = sizeof(WNDCLASSEXW);
	wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // CS_OWNDC is required for OpenGL
	wnd_class.lpfnWndProc = OS_WindowProc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = hInst;
	wnd_class.hIcon = NULL;
	wnd_class.hCursor = LoadCursorW(0, (const wchar_t*)IDC_ARROW);
	wnd_class.hbrBackground = NULL;
	wnd_class.lpszMenuName = NULL;
	wnd_class.lpszClassName = OS_WINDOW_CLASS_NAME;
	wnd_class.hIconSm = NULL;

	uint64_t atom = RegisterClassExW(&wnd_class);
	OS_CHECK(atom != 0);
}

OS_API void OS_WindowShow_(OS_Window* window) {
	OS_CHECK(UpdateWindow((HWND)window->handle) != 0);
	ShowWindow((HWND)window->handle, SW_SHOW);
}

OS_API OS_Window OS_WindowCreate(uint32_t width, uint32_t height, OS_String name) {
	OS_Window window = OS_WindowCreateHidden(width, height, name);
	OS_WindowShow_(&window);
	return window;
}

OS_API bool OS_SetFullscreen(OS_Window* window, bool fullscreen) {
	// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

	bool ok = true;
	int32_t style = GetWindowLongW((HWND)window->handle, GWL_STYLE);

	if (fullscreen) {
		HMONITOR monitor = MonitorFromWindow((HWND)window->handle, MONITOR_DEFAULTTONEAREST);

		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		ok = GetMonitorInfoW(monitor, &info) == 1;
		if (ok) {
			SetWindowLongW((HWND)window->handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

			RECT old_rect;
			GetWindowRect((HWND)window->handle, &old_rect);

			window->pre_fullscreen_state.left = old_rect.left;
			window->pre_fullscreen_state.top = old_rect.top;
			window->pre_fullscreen_state.right = old_rect.right;
			window->pre_fullscreen_state.bottom = old_rect.bottom;

			int32_t x = info.rcMonitor.left;
			int32_t y = info.rcMonitor.top;
			int32_t w = info.rcMonitor.right - x;
			int32_t h = info.rcMonitor.bottom - y;
			SetWindowPos((HWND)window->handle, HWND_TOPMOST, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
	}
	else {
		SetWindowLongW((HWND)window->handle, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);

		int32_t x = window->pre_fullscreen_state.left;
		int32_t y = window->pre_fullscreen_state.top;
		int32_t w = window->pre_fullscreen_state.left - x;
		int32_t h = window->pre_fullscreen_state.bottom - y;
		SetWindowPos((HWND)window->handle, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED);
	}
	return ok;
}

OS_API OS_Window OS_WindowCreateHidden(uint32_t width, uint32_t height, OS_String name) {
	// register raw input
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/using-raw-input

	RAWINPUTDEVICE input_devices[1];
	input_devices[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
	input_devices[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
	input_devices[0].dwFlags = 0/*RIDEV_NOLEGACY*/;    // adds mouse and also ignores legacy mouse messages
	input_devices[0].hwndTarget = 0;
	OS_CHECK(RegisterRawInputDevices(input_devices, sizeof(input_devices) / sizeof(input_devices[0]), sizeof(RAWINPUTDEVICE)) == 1);

	// NOTE: When you use a DPI scale on windows that's not 1, the window that we get back from `CreateWindowExW`
	// has an incorrect size that's not what we ask for.
	// Calling `SetProcessDPIAware` seems to resolve this issue, at least for the single monitor case.
	// 
	// TODO: fix for multiple monitors
	// https://stackoverflow.com/questions/71300163/how-to-create-a-window-with-createwindowex-but-ignoring-the-scale-settings-on-wi
	OS_CHECK(SetProcessDPIAware() == 1);

	// TODO: multiple windows?
	OS_RegisterWindowClass();

	int32_t x = 200;
	int32_t y = 200;
	RECT rect = { x, y, x + (int32_t)width, y + (int32_t)height };

	// AdjustWindowRect modifies the window rectangle: we give it the client area rectangle, it gives us back the entire window rectangle.
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 0, 0);

	OS_Arena* temp = OS_TempArenaBegin();

	HWND hwnd = CreateWindowExW(0,
		OS_WINDOW_CLASS_NAME,
		OS_StrToWide(temp, name, 1, NULL),
		WS_OVERLAPPEDWINDOW,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, 0, 0);

	OS_CHECK(hwnd != 0);

	OS_TempArenaEnd();

	OS_Window result = {0};
	result.handle = (OS_WindowHandle)hwnd;
	return result;
}

OS_API void OS_SetMouseCursor(OS_MouseCursor cursor) {
	if (cursor != OS_current_cursor) {
		wchar_t* cursor_name = NULL;
		switch (cursor) {
		case OS_MouseCursor_Arrow: cursor_name = (wchar_t*)IDC_ARROW; break;
		case OS_MouseCursor_Hand: cursor_name = (wchar_t*)IDC_HAND; break;
		case OS_MouseCursor_I_Beam: cursor_name = (wchar_t*)IDC_IBEAM; break;
		case OS_MouseCursor_Crosshair: cursor_name = (wchar_t*)IDC_CROSS; break;
		case OS_MouseCursor_ResizeH: cursor_name = (wchar_t*)IDC_SIZEWE; break;
		case OS_MouseCursor_ResizeV: cursor_name = (wchar_t*)IDC_SIZENS; break;
		case OS_MouseCursor_ResizeNESW: cursor_name = (wchar_t*)IDC_SIZENESW; break;
		case OS_MouseCursor_ResizeNWSE: cursor_name = (wchar_t*)IDC_SIZENWSE; break;
		case OS_MouseCursor_ResizeAll: cursor_name = (wchar_t*)IDC_SIZEALL; break;
		case OS_MouseCursor_COUNT: break;
		}
		OS_current_cursor_handle = LoadCursorW(NULL, cursor_name);
		OS_current_cursor = cursor;
	}
}

OS_API void OS_LockAndHideMouseCursor() { OS_mouse_cursor_should_hide = true; }

OS_API bool OS_WindowPoll(OS_Arena* arena, OS_Inputs* inputs, OS_Window* window, OS_WindowOnResize on_resize, void* user_data) {
	WindowProcUserData passed = {0};
	passed.window = window;
	passed.inputs = inputs;
	passed.on_resize = on_resize;
	passed.user_data = user_data;
	passed.text_input_utf32.arena = arena;
	passed.should_exit = false;

	// inputs->characters_count = 0;
	inputs->mouse_raw_dx = 0;
	inputs->mouse_raw_dy = 0;
	inputs->mouse_wheel_delta = 0;

	OS_InputStateFlags remove_flags = OS_InputStateFlag_WasReleased | OS_InputStateFlag_WasPressed | OS_InputStateFlag_WasPressedOrRepeat;
	for (size_t i = 0; i < OS_Input_COUNT; i++) {
		OS_InputStateFlags* flags = &inputs->input_states[i];
		*flags &= ~remove_flags;
	}

	// https://stackoverflow.com/questions/117792/best-method-for-storing-this-pointer-for-use-in-wndproc
	SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, (int64_t)&passed);

	for (;;) {
		MSG msg = {0};
		BOOL result = PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
		if (result != 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else break;
		// redraw window
	}

	SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, 0); // if we get more window messages after this point, then OS_WindowProc should ignore them.

	// :ManualMouseReleaseEvents
	// Manually check for mouse release, because we won't receive a release event if you release the mouse outside of the window.
	// Microsoft advices to use `SetCapture` / `ReleaseCapture`, but that seems like it'd be error prone when there are multiple
	// Set/ReleaseCaptures live at the same time when holding multiple mouse buttons.
	{
		if (OS_InputIsDown(inputs, OS_Input_MouseLeft) && !(GetKeyState(VK_LBUTTON) & 0x8000)) {
			inputs->input_states[OS_Input_MouseLeft] = OS_InputStateFlag_WasReleased;
		}
		if (OS_InputIsDown(inputs, OS_Input_MouseMiddle) && !(GetKeyState(VK_MBUTTON) & 0x8000)) {
			inputs->input_states[OS_Input_MouseMiddle] = OS_InputStateFlag_WasReleased;
		}
		if (OS_InputIsDown(inputs, OS_Input_MouseRight) && !(GetKeyState(VK_RBUTTON) & 0x8000)) {
			inputs->input_states[OS_Input_MouseRight] = OS_InputStateFlag_WasReleased;
		}
	}

	inputs->text_input_utf32 = passed.text_input_utf32.data;
	inputs->text_input_utf32_length = passed.text_input_utf32.length;

	/*
	For the most consistent way of setting the mouse cursor, we're doing it here. Setting it directly in `OS_SetMouseCursor` would work as well,
	but that could introduce flickering in some cases.
	*/
	/*if (OS_current_cursor != OS_MouseCursor_Arrow) {
		wchar_t* cursor_name = NULL;
		switch (OS_current_cursor) {
		case OS_MouseCursor_Arrow: cursor_name = (wchar_t*)IDC_ARROW; break;
		case OS_MouseCursor_Hand: cursor_name = (wchar_t*)IDC_HAND; break;
		case OS_MouseCursor_I_Beam: cursor_name = (wchar_t*)IDC_IBEAM; break;
		case OS_MouseCursor_Crosshair: cursor_name = (wchar_t*)IDC_CROSS; break;
		case OS_MouseCursor_ResizeH: cursor_name = (wchar_t*)IDC_SIZEWE; break;
		case OS_MouseCursor_ResizeV: cursor_name = (wchar_t*)IDC_SIZENS; break;
		case OS_MouseCursor_ResizeNESW: cursor_name = (wchar_t*)IDC_SIZENESW; break;
		case OS_MouseCursor_ResizeNWSE: cursor_name = (wchar_t*)IDC_SIZENWSE; break;
		case OS_MouseCursor_ResizeAll: cursor_name = (wchar_t*)IDC_SIZEALL; break;
		case OS_MouseCursor_COUNT: break;
		}
		SetCursor(LoadCursorW(NULL, cursor_name));
		OS_current_cursor = OS_MouseCursor_Arrow;
	}*/

	// Lock & hide mouse
	{
		if (OS_mouse_cursor_is_hidden) {
			SetCursorPos(OS_mouse_cursor_hidden_pos.x, OS_mouse_cursor_hidden_pos.y);
		}

		if (OS_mouse_cursor_should_hide != OS_mouse_cursor_is_hidden) {
			if (OS_mouse_cursor_should_hide) {
				ShowCursor(0);
				GetCursorPos(&OS_mouse_cursor_hidden_pos);
			}
			else {
				ShowCursor(1);
			}
		}
		OS_mouse_cursor_is_hidden = OS_mouse_cursor_should_hide;
		OS_mouse_cursor_should_hide = false;
	}

	{
		POINT cursor_pos;
		GetCursorPos(&cursor_pos);
		ScreenToClient((HWND)window->handle, &cursor_pos);
		inputs->mouse_x = cursor_pos.x;
		inputs->mouse_y = cursor_pos.y;
	}

	return !passed.should_exit;
}

// ----------------------------------------------------------------------------------------------------------------

static uint32_t OS_ThreadEntryFn(void* args) {
	OS_Thread* thread = (OS_Thread*)args;
	thread->fn(thread->user_data);

	_endthreadex(0);
	return 0;
}

OS_API void OS_ThreadStart(OS_Thread* thread, OS_ThreadFn fn, void* user_data, OS_String* debug_name) {
	OS_CHECK(thread->os_specific == NULL);

	OS_Arena* temp = OS_TempArenaBegin();

	// OS_ThreadEntryFn might be called at any time after calling `_beginthreadex`, and we must pass `fn` and `user_data` to it somehow. We can't pass them
	// on the stack, because this function might have exited at the point the ThreadEntryFn is entered. So, let's pass them in the OS_Thread structure
	// and let the user manage the OS_Thread pointer. Alternatively, we could make the user manually call OS_ThreadEnter(); and OS_ThreadExit(); inside
	// their thread function and directly pass the user function into _beginthreadex.
	thread->fn = fn;
	thread->user_data = user_data;

	// We're using _beginthreadex instead of CreateThread, because we're using the C runtime library.
	// https://stackoverflow.com/questions/331536/windows-threading-beginthread-vs-beginthreadex-vs-createthread-c
	// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex

	unsigned int thread_id;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, OS_ThreadEntryFn, thread, 0/* CREATE_SUSPENDED */, &thread_id);
	thread->os_specific = handle;

	if (debug_name) {
		wchar_t* debug_name_utf16 = OS_StrToWide(temp, *debug_name, 1, NULL);
		SetThreadDescription(handle, debug_name_utf16);
	}

	OS_TempArenaEnd();
}

OS_API void OS_ThreadJoin(OS_Thread* thread) {
	WaitForSingleObject((HANDLE)thread->os_specific, INFINITE);
	CloseHandle((HANDLE)thread->os_specific);
	memset(thread, 0, sizeof(*thread)); // Do this so that we can safely start a new thread again using this same struct
}

OS_API void OS_MutexInit(OS_Mutex* mutex) {
	OS_CHECK(sizeof(OS_Mutex) >= sizeof(CRITICAL_SECTION));
	InitializeCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_API void OS_MutexDestroy(OS_Mutex* mutex) {
	DeleteCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_API void OS_MutexLock(OS_Mutex* mutex) {
	EnterCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_API void OS_MutexUnlock(OS_Mutex* mutex) {
	LeaveCriticalSection((CRITICAL_SECTION*)mutex);
}

OS_API void OS_ConditionVarInit(OS_ConditionVar* condition_var) {
	OS_CHECK(sizeof(OS_ConditionVar) >= sizeof(CONDITION_VARIABLE));
	InitializeConditionVariable((CONDITION_VARIABLE*)condition_var);
}

OS_API void OS_ConditionVarDestroy(OS_ConditionVar* condition_var) {
	// No need to explicitly destroy
	// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializeconditionvariable
}

OS_API void OS_ConditionVarWait(OS_ConditionVar* condition_var, OS_Mutex* lock) {
	SleepConditionVariableCS((CONDITION_VARIABLE*)condition_var, (CRITICAL_SECTION*)lock, INFINITE);
}

OS_API void OS_ConditionVarSignal(OS_ConditionVar* condition_var) {
	WakeConditionVariable((CONDITION_VARIABLE*)condition_var);
}

OS_API void OS_ConditionVarBroadcast(OS_ConditionVar* condition_var) {
	WakeAllConditionVariable((CONDITION_VARIABLE*)condition_var);
}
#endif // OS_IMPLEMENTATION
#endif // OS_INCLUDED
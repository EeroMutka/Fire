/* fire_os.h, created by Eero Mutka. Version date: Jan 7. 2024

fire_os is my operating system abstraction library. It lets you create windows, get keyboard/mouse input, open
file dialogs, launch threads, etc.
NOTE: This library is very work-in-progress and currently only works on Windows.

LICENSE: This code is released under the MIT-0 license, which you can find at https://opensource.org/license/mit-0/.
         Also, currently this code is including the `offsetAlloctor` library (by Sebastian Aaltonen, MIT license), but I'll soon it.
*/

#ifndef FOS_INCLUDED
#define FOS_INCLUDED

// It seems like a good idea to make the working directory visible and explicit, as opposed to the traditional way of having it
// be hidden state. As a fallback, you can still use CWD which will use the current working directory.

#include <stdbool.h>
#include <stdint.h>

#ifndef FOS_API
#define FOS_API
#endif

// Used to mark optional pointers that may be NULL
// #define FOS_Opt(ptr) ptr

#ifndef FOS_CUSTOM_STRING
typedef struct FOS_String {
	char* data;
	size_t len;
} FOS_String;
#endif

typedef FOS_String FOS_WorkingDir; // Must be an absolute path or CWD

#ifdef __cplusplus
#define FOS_CWD FOS_String{}    // Use current working directory
#else
#define FOS_CWD (FOS_String){0} // Use current working directory
#endif

typedef void FOS_Arena;
void FOS_Impl_Realloc(void* old_ptr, size_t size, size_t alignment);
void* FOS_Impl_ArenaPush(FOS_Arena* arena, size_t size, size_t alignment);

/*
These can be combined as a mask.
@cleanup: remove this thing
*/
typedef enum {
	FOS_ConsoleAttribute_Blue = 0x0001,
	FOS_ConsoleAttribute_Green = 0x0002,
	FOS_ConsoleAttribute_Red = 0x0004,
	FOS_ConsoleAttribute_Intensify = 0x0008,
} FOS_ConsoleAttribute;

typedef int FOS_ConsoleAttributeFlags;

//typedef struct Semaphore Semaphore;
typedef struct FOS_Mutex { uint64_t os_specific[5]; } FOS_Mutex;
typedef struct FOS_ConditionVar {
	uint64_t os_specific;
	//FOS_Mutex mutex;
} FOS_ConditionVar;

typedef void (*FOS_ThreadFn)(void* user_ptr);
typedef struct FOS_Thread {
	void* os_specific;
	FOS_ThreadFn fn;
	void* user_ptr;
} FOS_Thread;

// typedef struct {
// 	FOS_String function;
// 	FOS_String file;
// 	uint32_t line;
// } FOS_DebugStackFrame;
// typedef Slice(FOS_DebugStackFrame) DebugStackFrameSlice;

// OS-specific window handle. On windows, it's represented as HWND.
typedef void* FOS_WindowHandle;

typedef struct { uint64_t tick; } FOS_Tick;

typedef struct { void* handle; } FOS_DynamicLibrary;

typedef struct { uint64_t os_specific; } FOS_FileTime;

typedef enum FOS_FileOpenMode {
	FOS_FileOpenMode_Read,
	FOS_FileOpenMode_Write,
	FOS_FileOpenMode_Append,
} FOS_FileOpenMode;

#ifndef FOS_FILE_WRITER_BUFFER_SIZE
#define FOS_FILE_WRITER_BUFFER_SIZE 4096
#endif

typedef struct FOS_File {
	FOS_FileOpenMode mode;
	void* os_handle;

	//int64_t internal_data[8];
	// Writer backing_writer;
	// BufferedWriter buffered_writer;
	uint8_t buffer[FOS_FILE_WRITER_BUFFER_SIZE];
} FOS_File;

typedef struct FOS_VisitDirectoryInfo {
	FOS_String name;
	bool is_directory;
} FOS_VisitDirectoryInfo;

typedef enum FOS_VisitDirectoryResult {
	// TODO: OS_VisitDirectoryResult_Recurse,
	fVisitDirectoryResult_Continue,
} FOS_VisitDirectoryResult;

typedef FOS_VisitDirectoryResult(*FOS_VisitDirectoryVisitor)(const FOS_VisitDirectoryInfo* info, void* userptr);

typedef enum FOS_Input {
	FOS_Input_Invalid = 0,

	FOS_Input_Space = 32,
	FOS_Input_Apostrophe = 39,   /* ' */
	FOS_Input_Comma = 44,        /* , */
	FOS_Input_Minus = 45,        /* - */
	FOS_Input_Period = 46,       /* . */
	FOS_Input_Slash = 47,        /* / */

	FOS_Input_0 = 48,
	FOS_Input_1 = 49,
	FOS_Input_2 = 50,
	FOS_Input_3 = 51,
	FOS_Input_4 = 52,
	FOS_Input_5 = 53,
	FOS_Input_6 = 54,
	FOS_Input_7 = 55,
	FOS_Input_8 = 56,
	FOS_Input_9 = 57,

	FOS_Input_Semicolon = 59,    /* ; */
	FOS_Input_Equal = 61,        /* = */
	FOS_Input_LeftBracket = 91,  /* [ */
	FOS_Input_Backslash = 92,    /* \ */
	FOS_Input_RightBracket = 93, /* ] */
	FOS_Input_GraveAccent = 96,  /* ` */

	FOS_Input_A = 65,
	FOS_Input_B = 66,
	FOS_Input_C = 67,
	FOS_Input_D = 68,
	FOS_Input_E = 69,
	FOS_Input_F = 70,
	FOS_Input_G = 71,
	FOS_Input_H = 72,
	FOS_Input_I = 73,
	FOS_Input_J = 74,
	FOS_Input_K = 75,
	FOS_Input_L = 76,
	FOS_Input_M = 77,
	FOS_Input_N = 78,
	FOS_Input_O = 79,
	FOS_Input_P = 80,
	FOS_Input_Q = 81,
	FOS_Input_R = 82,
	FOS_Input_S = 83,
	FOS_Input_T = 84,
	FOS_Input_U = 85,
	FOS_Input_V = 86,
	FOS_Input_W = 87,
	FOS_Input_X = 88,
	FOS_Input_Y = 89,
	FOS_Input_Z = 90,

	FOS_Input_Escape = 256,
	FOS_Input_Enter = 257,
	FOS_Input_Tab = 258,
	FOS_Input_Backspace = 259,
	FOS_Input_Insert = 260,
	FOS_Input_Delete = 261,
	FOS_Input_Right = 262,
	FOS_Input_Left = 263,
	FOS_Input_Down = 264,
	FOS_Input_Up = 265,
	FOS_Input_PageUp = 266,
	FOS_Input_PageDown = 267,
	FOS_Input_Home = 268,
	FOS_Input_End = 269,
	FOS_Input_CapsLock = 280,
	FOS_Input_ScrollLock = 281,
	FOS_Input_NumLock = 282,
	FOS_Input_PrintScreen = 283,
	FOS_Input_Pause = 284,

	//FOS_Input_F1 = 290,
	//FOS_Input_F2 = 291,
	//FOS_Input_F3 = 292,
	//FOS_Input_F4 = 293,
	//FOS_Input_F5 = 294,
	//FOS_Input_F6 = 295,
	//FOS_Input_F7 = 296,
	//FOS_Input_F8 = 297,
	//FOS_Input_F9 = 298,
	//FOS_Input_F10 = 299,
	//FOS_Input_F11 = 300,
	//FOS_Input_F12 = 301,
	//FOS_Input_F13 = 302,
	//FOS_Input_F14 = 303,
	//FOS_Input_F15 = 304,
	//FOS_Input_F16 = 305,
	//FOS_Input_F17 = 306,
	//FOS_Input_F18 = 307,
	//FOS_Input_F19 = 308,
	//FOS_Input_F20 = 309,
	//FOS_Input_F21 = 310,
	//FOS_Input_F22 = 311,
	//FOS_Input_F23 = 312,
	//FOS_Input_F24 = 313,
	//FOS_Input_F25 = 314,

	//FOS_Input_KP_0 = 320,
	//FOS_Input_KP_1 = 321,
	//FOS_Input_KP_2 = 322,
	//FOS_Input_KP_3 = 323,
	//FOS_Input_KP_4 = 324,
	//FOS_Input_KP_5 = 325,
	//FOS_Input_KP_6 = 326,
	//FOS_Input_KP_7 = 327,
	//FOS_Input_KP_8 = 328,
	//FOS_Input_KP_9 = 329,

	//FOS_Input_KP_Decimal = 330,
	//FOS_Input_KP_Divide = 331,
	//FOS_Input_KP_Multiply = 332,
	//FOS_Input_KP_Subtract = 333,
	//FOS_Input_KP_Add = 334,
	//FOS_Input_KP_Enter = 335,
	//FOS_Input_KP_Equal = 336,

	FOS_Input_LeftShift = 340,
	FOS_Input_LeftControl = 341,
	FOS_Input_LeftAlt = 342,
	FOS_Input_LeftSuper = 343,
	FOS_Input_RightShift = 344,
	FOS_Input_RightControl = 345,
	FOS_Input_RightAlt = 346,
	FOS_Input_RightSuper = 347,
	//FOS_Input_Menu = 348,

	FOS_Input_Shift = 349,
	FOS_Input_Control = 350,
	FOS_Input_Alt = 351,
	FOS_Input_Super = 352,

	FOS_Input_MouseLeft = 353,
	FOS_Input_MouseRight = 354,
	FOS_Input_MouseMiddle = 355,
	//FOS_Input_Mouse_4 = 356,
	//FOS_Input_Mouse_5 = 357,
	//FOS_Input_Mouse_6 = 358,
	//FOS_Input_Mouse_7 = 359,
	//FOS_Input_Mouse_8 = 360,

	FOS_Input_COUNT,
} FOS_Input;

typedef enum FOS_MouseCursor {
	FOS_MouseCursor_Arrow,
	FOS_MouseCursor_Hand,
	FOS_MouseCursor_I_beam,
	FOS_MouseCursor_Crosshair,
	FOS_MouseCursor_ResizeH,
	FOS_MouseCursor_ResizeV,
	FOS_MouseCursor_ResizeNESW,
	FOS_MouseCursor_ResizeNWSE,
	FOS_MouseCursor_ResizeAll,
	FOS_MouseCursor_COUNT
} FOS_MouseCursor;

typedef void(*FOS_WindowOnResize)(uint32_t width, uint32_t height, void* user_ptr);

typedef uint8_t FOS_InputStateFlags;
typedef enum FOS_InputStateFlag {
	FOS_InputStateFlag_DidBeginPress = 1 << 0,
	FOS_InputStateFlag_DidEndPress = 1 << 1,
	FOS_InputStateFlag_DidRepeat = 1 << 2,
	FOS_InputStateFlag_IsPressed = 1 << 3,

	//FOS_InputStateFlag_WasReleased = 1 << 1,
	//FOS_InputStateFlag_WasPressed = 1 << 2,
} FOS_InputStateFlag;

typedef struct FOS_Inputs {
	// if we do keep a public list of `FOS_Input` states, and in general keep the ABI public,
	// it means that whatever library uses `foundation_os` for getting input doesn't actually their user
	// to use `foundation_os`. The user COULD pull the rug and provide their own initialized `fWindowEvents`!

	uint32_t mouse_x, mouse_y; // mouse position in window pixel coordinates (+x right, +y down)
	int32_t mouse_raw_dx, mouse_raw_dy; // raw mouse motion, +x right, +y down
	float mouse_wheel; // +1.0 means the wheel was rotated forward by one detent / scroll step

	FOS_InputStateFlags input_states[FOS_Input_COUNT];

	uint32_t characters[8]; // 32-bit unicode codepoints
	uint32_t characters_count;
} FOS_Inputs;

typedef struct FOS_Log FOS_Log;
struct FOS_Log {
	void(*print)(FOS_Log* self, FOS_String data);
};

typedef struct FOS_Window {
	FOS_WindowHandle handle;
	FOS_Inputs inputs;
} FOS_Window;

// The following calls Init() internally, so you only need to call FOS_Init().
FOS_API void FOS_Init();

FOS_API FOS_Log* FOS_Console();  // Redirects writes to OS console (stdout)
FOS_API FOS_Log* FOS_DebugLog(); // Redirects writes to OS debug output

// `log` may be NULL, in which case nothing is printed
FOS_API void FOS_LogPrint(FOS_Log* log, const char* fmt, ...);

FOS_API void FOS_Print(const char* fmt, ...); // Prints to OS console
FOS_API void FOS_PrintString(FOS_String str);
FOS_API void FOS_PrintStringColor(FOS_String str, FOS_ConsoleAttributeFlags attributes_mask);

FOS_API void FOS_DebugPrint(const char* fmt, ...);
FOS_API void FOS_DebugPrintString(FOS_String str);

FOS_API uint64_t FOS_ReadCycleCounter();
FOS_API void FOS_Sleep(uint64_t ms);

// * if you want to read both stdout and stderr together in the right order, then pass identical pointers to `stdout_log` and `stderr_log`.
// * `args[0]` should be the path of the executable, where `\' and `/` are accepted path separators.
// * `out_exit_code`, `stdout_log`, `stderr_log` may be NULL
FOS_API bool FOS_RunCommand(FOS_WorkingDir working_dir, FOS_String* args, uint32_t args_count, uint32_t* out_exit_code, FOS_Log* stdout_log, FOS_Log* stderr_log);

// Find the path of an executable given its name, or return an empty string if it's not found.
// NOTE: `name` must include the file extension!
FOS_API FOS_String FOS_FindExecutable(FOS_Arena* arena, FOS_String name);

FOS_API bool FOS_SetWorkingDir(FOS_WorkingDir dir);
FOS_API FOS_WorkingDir FOS_GetWorkingDir(FOS_Arena* arena);

FOS_API FOS_String FOS_GetExecutablePath(FOS_Arena* arena);

FOS_API void FOS_MessageBox(FOS_String title, FOS_String message);

// FOS_API DebugStackFrameSlice FOS_DebugGetStackTrace(FOS_Arena* arena);

// -- Clipboard ----------------------------------------------------------------

FOS_API FOS_String FOS_ClipboardGetText(FOS_Arena* arena);
FOS_API void FOS_ClipboardSetText(FOS_String str);

// -- DynamicLibrary -----------------------------------------------------------

FOS_API bool FOS_LoadDLL(FOS_WorkingDir working_dir, FOS_String filepath, FOS_DynamicLibrary* out_dll);
FOS_API void FOS_UnloadDLL(FOS_DynamicLibrary dll);
FOS_API void* FOS_GetSymbolAddress(FOS_DynamicLibrary dll, FOS_String symbol);

// -- Files --------------------------------------------------------------------

// The path separator in the returned string will depend on the OS. On windows, it will be a backslash.
// If the provided path is invalid, an empty string will be returned.
// TODO: maybe it'd be better if these weren't os-specific functions, and instead could take an argument for specifying
//       windows-style paths / unix-style paths
FOS_API bool FOS_PathIsAbsolute(FOS_String path);

// OLD COMMENT: If `working_dir` is an empty string, the current working directory will be used.
// `working_dir` must be an absolute path.
FOS_API bool FOS_PathToCanonical(FOS_Arena* arena, FOS_WorkingDir working_dir, FOS_String path, FOS_String* out_canonical);

FOS_API bool FOS_VisitDirectory(FOS_WorkingDir working_dir, FOS_String path, FOS_VisitDirectoryVisitor visitor, void* visitor_userptr);
FOS_API bool FOS_DirectoryExists(FOS_WorkingDir working_dir, FOS_String path);
FOS_API bool FOS_DeleteDirectory(FOS_WorkingDir working_dir, FOS_String path); // If the directory doesn't already exist, it's treated as a success.
FOS_API bool FOS_MakeDirectory(FOS_WorkingDir working_dir, FOS_String path); // If the directory already exists, it's treated as a success.

FOS_API bool FOS_ReadEntireFile(FOS_WorkingDir working_dir, FOS_String filepath, FOS_Arena* arena, FOS_String* out_str);
FOS_API bool FOS_WriteEntireFile(FOS_WorkingDir working_dir, FOS_String filepath, FOS_String data);

FOS_API bool FOS_FileOpen(FOS_WorkingDir working_dir, FOS_String filepath, FOS_FileOpenMode mode, FOS_File* out_file);
FOS_API bool FOS_FileClose(FOS_File* file);
FOS_API uint64_t FOS_FileSize(FOS_File* file);
FOS_API size_t FOS_FileRead(FOS_File* file, void* dst, size_t size);
FOS_API bool FOS_FileWriteUnbuffered(FOS_File* file, FOS_String data);
FOS_API uint64_t FOS_FileGetCursor(FOS_File* file);
FOS_API bool FOS_FileSetCursor(FOS_File* file, uint64_t position);

FOS_API void FOS_FileFlush(FOS_File* file);
// inline Writer* FOS_BufferedWriterFromFile(FOS_File* file) { return (Writer*)&file->buffered_writer; }

FOS_API bool FOS_FileModtime(FOS_WorkingDir working_dir, FOS_String filepath, FOS_FileTime* out_modtime); // returns false if the file does not exist
FOS_API int FOS_FileCmpModtime(FOS_FileTime a, FOS_FileTime b); // returns 1 when a > b, -1 when a < b, 0 when a == b
FOS_API bool FOS_CloneFile(FOS_WorkingDir working_dir, FOS_String dst_filepath, FOS_String src_filepath); // NOTE: the parent directory of the destination filepath must already exist.
FOS_API bool FOS_DeleteFile(FOS_WorkingDir working_dir, FOS_String filepath);

FOS_API bool FOS_CloneDirectory(FOS_WorkingDir working_dir, FOS_String dst_path, FOS_String src_path); // NOTE: the parent directory of the destination filepath must already exist.

FOS_API bool FOS_FilePicker(FOS_Arena* arena, FOS_String* out_filepath);

// TODO: writer. Should be buffered by default
//inline BufferedWriter f_files_writer(FOS_File* file) {
//
//}

// -- Threads -----------------------------------------------------------------

// NOTE: The `thread` pointer cannot be moved or copied while in use.
FOS_API void FOS_ThreadStart(FOS_Thread* thread, FOS_ThreadFn fn, void* user_ptr, FOS_String debug_name);
FOS_API void FOS_ThreadJoin(FOS_Thread* thread);

FOS_API void FOS_MutexInit(FOS_Mutex* mutex);
FOS_API void FOS_MutexDestroy(FOS_Mutex* mutex);
FOS_API void FOS_MutexLock(FOS_Mutex* mutex);
FOS_API void FOS_MutexUnlock(FOS_Mutex* mutex);

// NOTE: A condition variable cannot be moved or copied while in use.
FOS_API void FOS_ConditionVarInit(FOS_ConditionVar* condition_var);
FOS_API void FOS_ConditionVarDestroy(FOS_ConditionVar* condition_var);
FOS_API void FOS_ConditionVarSignal(FOS_ConditionVar* condition_var);
FOS_API void FOS_ConditionVarBroadcast(FOS_ConditionVar* condition_var);
FOS_API void FOS_ConditionVarWait(FOS_ConditionVar* condition_var, FOS_Mutex* lock);
// FOS_API void FOS_ConditionVarWaitEx(FOS_ConditionVar* condition_var, FOS_Mutex* lock, Int wait_milliseconds);

//FOS_API Semaphore* FOS_SemaphoreCreate(); // Starts as 0
//FOS_API void FOS_SemaphoreDestroy(Semaphore* semaphore);
//FOS_API void FOS_SemaphoreWait(Semaphore* semaphore);
//FOS_API void FOS_SemaphoreWaitEx(Semaphore* semaphore, Int milliseconds);
//FOS_API void FOS_SemaphoreSignal(Semaphore* semaphore);

// -- Time --------------------------------------------------------------------

FOS_API FOS_Tick FOS_Now();
FOS_API double FOS_DurationInSeconds(FOS_Tick start, FOS_Tick end);

// -- Window creation ---------------------------------------------------------

FOS_API bool FOS_IsPressed(const FOS_Inputs* inputs, FOS_Input input);
FOS_API bool FOS_DidBeginPress(const FOS_Inputs* inputs, FOS_Input input);
FOS_API bool FOS_DidBeginPressOrRepeat(const FOS_Inputs* inputs, FOS_Input input);
FOS_API bool FOS_DidEndPress(const FOS_Inputs* inputs, FOS_Input input);

/*
 NOTE: After calling `FOS_WindowCreateNoShow`, the window will remain hidden.
 To show the window, you must explicitly call `FOS_WindowShow`. This separation is there so that you
 can get rid of the initial flicker that normally happens when you create a window, then do some work
 such as initialize a graphics FOS_API, and finally present a frame. If you instead first call `create`,
 then initialize the graphics FOS_API, and only then call `show`, there won't be any flicker as the
 window doesn't have to wait for the initialization.
*/
FOS_API FOS_Window FOS_WindowCreateHidden(uint32_t width, uint32_t height, FOS_String name);
FOS_API void FOS_WindowShow_(FOS_Window* window);

FOS_API FOS_Window FOS_WindowCreate(uint32_t width, uint32_t height, FOS_String name);

// * Returns `false` when the window is closed.
// * The reason we provide a callback for window resizes instead of returning the size is that, when resizing, Windows doesn't
//   let us exit the event loop until the user lets go of the mouse. This means that if you want to draw into the window
//   while resizing to make it feel smooth, you have to do it from a callback internally. So, if you want to draw while resizing,
//   you should do it inside `on_resize`.
// * on_resize may be NULL
FOS_API bool FOS_WindowPoll(FOS_Window* window, FOS_WindowOnResize on_resize, void* on_resize_user_ptr);

FOS_API void FOS_SetMouseCursor(FOS_MouseCursor cursor);
FOS_API void FOS_LockAndHideMouseCursor();

#endif // FOS_INCLUDED
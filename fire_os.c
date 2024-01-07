
// -- Linker inputs --
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shell32.lib") // for SHFileOperationW
#pragma comment(lib, "Advapi32.lib") // for RegCloseKey
#pragma comment(lib, "Ole32.lib") // for CoCreateInstance
#pragma comment(lib, "OleAut32.lib") // for SysFreeString
#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Comdlg32.lib") // for GetOpenFileNameW
// --

#include <stdarg.h>

#include "fire.h"

#define FOS_CUSTOM_STRING
typedef String FOS_String;
#include "fire_os.h"

// #ifndef FOS_CUSTOM_ARENA
// // OK. so we want fire.h to use the implemented FOS_Arena. Let's assume we haven't included fire.h before.
// #define FIRE_CUSTOM_ARENA
// typedef FOS_Arena Arena;
// typedef FOS_ArenaMark ArenaMark;
// #define ArenaFn(arena, mark, op, size, alignment) DefaultArenaFn((DefaultArena*)arena, (DefaultArenaMark*)mark, op, size, alignment)
// #endif

#include "offsetAllocator.c"

#ifdef FOS_MODE_DEBUG
// TODO: use malloc for allocations bigger than 30KB and use TLSF for anything less than that but reserve 1MB for small allocations (falling back to malloc if full).
// That would make having many individual arenas more viable with O(1) allocation even if it's not what you want most of the time.
static oaNodeIndex OA_FREE_NODES[1024*32];
static oaNode OA_NODES[1024*32];
static U8 MEMORY[GIB(1)];
static oaAllocator OA_ALLOCATOR;
static FOS_Mutex OA_MUTEX;
#endif

FOS_API bool FOS_WriteEntireFile(FOS_WorkingDir working_dir, String filepath, String data) {
	FOS_File file;
	if (!FOS_FileOpen(working_dir, filepath, FOS_FileOpenMode_Write, &file)) return false;
	if (!FOS_FileWriteUnbuffered(&file, data)) return false;
	FOS_FileClose(&file);
	return true;
}

FOS_API bool FOS_ReadEntireFile(FOS_WorkingDir working_dir, String filepath, FOS_Arena* arena, String* out_str) {
	FOS_File file;
	if (!FOS_FileOpen(working_dir, filepath, FOS_FileOpenMode_Read, &file)) return false;

	U64 size = FOS_FileSize(&file);

	Array(U8) result = {0};
	ArrayResizeUndef(arena, &result, size);

	bool ok = FOS_FileRead(&file, result.data, size) == size;

	FOS_FileClose(&file);
	*out_str = ArrTo(String, result);
	return ok;
}

FOS_API bool FOS_IsPressed(const FOS_Inputs* inputs, FOS_Input input) {
	return inputs->input_states[input] & FOS_InputStateFlag_IsPressed;
}

FOS_API bool FOS_DidBeginPress(const FOS_Inputs* inputs, FOS_Input input) {
	return inputs->input_states[input] & FOS_InputStateFlag_DidBeginPress;
}

FOS_API bool FOS_DidBeginPressOrRepeat(const FOS_Inputs* inputs, FOS_Input input) {
	return inputs->input_states[input] & (FOS_InputStateFlag_DidBeginPress | FOS_InputStateFlag_DidRepeat);
}

FOS_API bool FOS_DidEndPress(const FOS_Inputs* inputs, FOS_Input input) {
	return inputs->input_states[input] & FOS_InputStateFlag_DidEndPress;
}

#ifdef FOS_WINDOWS

// - Windows implementation -------------------------------------------------------------------------

// <process.h>
uintptr_t _beginthreadex(void *security, unsigned stack_size, unsigned ( __stdcall *start_address )( void * ), void *arglist, unsigned initflag, unsigned *thrdaddr);
void _endthreadex(unsigned retval);

// we need Windows.h for the microsoft_craziness.h stuff
#if 1
#define UNICODE // required for Windows.h
#include <Windows.h>
#include <DbgHelp.h>
#else
// Why include these here instead of doing #include <Windows.h>? The main reason is that Windows.h is huge, so we can speed up compile times by not including it.

#define CP_UTF8 65001
#define STD_OUTPUT_HANDLE ((u32)-11)
#define OPEN_EXISTING 3
#define FILE_SHARE_READ                 0x00000001  
#define FILE_SHARE_DELETE               0x00000004  
#define FILE_ATTRIBUTE_READONLY         0x00000001
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define INVALID_HANDLE_VALUE ((HANDLE)(void*)-1)
#define MB_ERR_INVALID_CHARS 0x00000008
#define MB_OK 0x00000000L
#define MEM_RESERVE 0x00002000
#define PAGE_READWRITE 0x04
#define MEM_COMMIT 0x00001000
#define MEM_DECOMMIT 0x00004000
#define MEM_RELEASE 0x00008000
#define CF_UNICODETEXT 13
#define INVALID_FILE_ATTRIBUTES ((u32)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define VOLUME_NAME_DOS 0x0
#define ERROR_NO_MORE_FILES 18L
#define FO_DELETE 0x0003
#define FOF_NO_UI (0x0004 | 0x0010 | 0x0400 | 0x0200)
#define ERROR_ALREADY_EXISTS 183L
#define FILE_GENERIC_READ (((0x00020000L)) | (0x0001) | (0x0080) | (0x0008) | (0x00100000L))
#define FILE_GENERIC_WRITE (((0x00020000L)) | (0x0002) | (0x0100) | (0x0010) | (0x0004) | (0x00100000L))
#define OPEN_ALWAYS 4
#define CREATE_ALWAYS 2
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define HANDLE_FLAG_INHERIT 0x00000001
#define INFINITE 0xFFFFFFFF
#define GWLP_USERDATA (-21)
#define VK_SPACE          0x20
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_3          0xC0   // '`~' for US
#define VK_ESCAPE         0x1B
#define VK_RETURN         0x0D
#define VK_TAB            0x09
#define VK_BACK           0x08
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_RIGHT          0x27
#define VK_LEFT           0x25
#define VK_DOWN           0x28
#define VK_UP             0x26
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_HOME           0x24
#define VK_END            0x23
#define VK_CAPITAL        0x14
#define VK_NUMLOCK        0x90
#define VK_SNAPSHOT       0x2C
#define VK_PAUSE          0x13
#define VK_SHIFT          0x10
#define VK_RSHIFT         0xA1
#define VK_MENU           0x12
#define VK_RMENU          0xA5
#define VK_CONTROL        0x11
#define VK_RCONTROL       0xA3
#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_LBUTTON        0x01
#define VK_MBUTTON        0x04
#define VK_RBUTTON        0x02
#define WM_INPUT        0x00FF
#define WM_UNICHAR      0x0109
#define WM_CHAR         0x0102
#define WM_QUIT         0x0012
#define WM_SIZE         0x0005
#define WM_SETCURSOR    0x0020
#define WM_CLOSE        0x0010
#define WM_SYSKEYDOWN   0x0104
#define WM_KEYDOWN      0x0100
#define WM_SYSKEYUP     0x0105
#define WM_KEYUP        0x0101
#define WM_LBUTTONDOWN  0x0201
#define WM_RBUTTONDOWN  0x0204
#define WM_MBUTTONDOWN  0x0207
#define WM_MOUSEWHEEL   0x020A
#define RID_INPUT 0x10000003
#define RIM_TYPEMOUSE 0
#define MOUSE_MOVE_RELATIVE 0
#define MOUSE_MOVE_ABSOLUTE 1
#define WHEEL_DELTA 120
#define CS_HREDRAW 0x0002
#define CS_VREDRAW 0x0001
#define CS_OWNDC 0x0020
#define IDC_ARROW           ((u16*)((u64)(32512)))
#define IDC_SIZEWE          ((u16*)((u64)(32644)))
#define IDC_HAND            ((u16*)((u64)(32649)))
#define IDC_IBEAM           ((u16*)((u64)(32513)))
#define IDC_CROSS           ((u16*)((u64)(32515)))
#define IDC_SIZENS          ((u16*)((u64)(32645)))
#define IDC_SIZENESW        ((u16*)((u64)(32643)))
#define IDC_SIZENWSE        ((u16*)((u64)(32642)))
#define IDC_SIZEALL         ((u16*)((u64)(32646)))
#define SW_SHOW 5
#define WS_OVERLAPPEDWINDOW (0x00000000L | 0x00C00000L | 0x00080000L | 0x00040000L | 0x00020000L | 0x00010000L)
#define PM_REMOVE 0x0001

typedef struct { i32 x; i32 y; } POINT;
typedef struct { i16 X; i16 Y; } COORD;
typedef struct { i16 Left; i16 Top; i16 Right; i16 Bottom; } SMALL_RECT;
typedef struct { COORD dwSize; COORD dwCursorPosition; u16 wAttributes; SMALL_RECT srWindow; COORD dwMaximumWindowSize; } CONSOLE_SCREEN_BUFFER_INFO;
typedef union { struct { u32 LowPart; i32 HighPart; } u; i64 QuadPart; } LARGE_INTEGER;
typedef struct { u32 dwLowDateTime; u32 dwHighDateTime; } FILETIME;
typedef int BOOL;
typedef void* HANDLE;
typedef struct { int _; }* HWND;
typedef struct { int _; }* HRAWINPUT;
typedef struct { int _; }* HICON;
typedef struct { int _; }* HBRUSH;
typedef struct { int _; }* HINSTANCE;
typedef struct { int _; }* HMENU;
typedef struct { u32 dwFileAttributes; u64 ftCreationTime; u64 ftLastAccessTime; u64 ftLastWriteTime; u32 nFileSizeHigh; u32 nFileSizeLow; u32 dwReserved0; u32 dwReserved1; u16 cFileName[260]; u16 cAlternateFileName[14]; } WIN32_FIND_DATAW;
typedef struct { HWND hwnd; u32 wFunc; u16* pFrom; u16* pTo; u16 fFlags; BOOL fAnyOperationsAborted; void* hNameMappings; u16* lpszProgressTitle;} SHFILEOPSTRUCTW;
typedef struct { HANDLE hProcess; HANDLE hThread; u32 dwProcessId; u32 dwThreadId; } PROCESS_INFORMATION;
typedef struct { u32 cb; u16* lpReserved; u16* lpDesktop; u16* lpTitle; u32 dwX; u32 dwY; u32 dwXSize; u32 dwYSize; u32 dwXCountChars; u32 dwYCountChars; u32 dwFillAttribute; u32 dwFlags; u16 wShowWindow; u16 cbReserved2; u8* lpReserved2; HANDLE hStdInput; HANDLE hStdOutput; HANDLE hStdError; } STARTUPINFOW;
typedef struct { u32 nLength; void* lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES;
typedef struct { u32 dwType; u32 dwSize; HANDLE hDevice; u64 wParam; } RAWINPUTHEADER;
typedef struct { u16 MakeCode; u16 Flags; u16 Reserved; u16 VKey; u32 Message; u32 ExtraInformation; } RAWKEYBOARD;
typedef struct { u16 usFlags; union { u32 ulButtons; struct { u16 usButtonFlags; u16 usButtonData; } DUMMYSTRUCTNAME; } DUMMYUNIONNAME; u32 ulRawButtons; i32 lLastX; i32 lLastY; u32 ulExtraInformation; } RAWMOUSE;
typedef struct { u32 dwSizeHid; u32 dwCount; u8 bRawData[1]; } RAWHID;
typedef struct { RAWINPUTHEADER header; union { RAWMOUSE mouse; RAWKEYBOARD keyboard; RAWHID hid; } data; } RAWINPUT;
typedef struct tagRAWINPUTDEVICE { u16 usUsagePage; u16 usUsage; u32 dwFlags; HWND hwndTarget; } RAWINPUTDEVICE;
typedef i64 (__stdcall* WNDPROC)(HWND, u32, u64, i64);
typedef struct { u32 cbSize; u32 style; WNDPROC lpfnWndProc; int cbClsExtra; int cbWndExtra; HINSTANCE hInstance; HICON hIcon; HICON hCursor; HBRUSH hbrBackground; u16* lpszMenuName; u16* lpszClassName; HICON hIconSm; } WNDCLASSEXW;
typedef struct { i32 left; i32 top; i32 right; i32 bottom; } RECT;
typedef struct { HWND hwnd; u32 message; u64 wParam; i64 lParam; u32 time; POINT pt;} MSG;

__declspec(dllimport) BOOL __stdcall QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);
__declspec(dllimport) BOOL __stdcall QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
__declspec(dllimport) HANDLE __stdcall GetStdHandle(u32 nStdHandle);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE hFile, u8* lpBuffer, u32 nNumberOfBytesToWrite, u32* lpNumberOfBytesWritten, void* lpOverlapped);
__declspec(dllimport) BOOL __stdcall WriteConsoleOutputAttribute(HANDLE hConsoleOutput, const u16* lpAttribute, u32 nLength, COORD dwWriteCoord, u32* lpNumberOfAttrsWritten);
__declspec(dllimport) HANDLE __stdcall CreateFileW(u16* lpFileName, u32 dwDesiredAccess, u32 dwShareMode, void* lpSecurityAttributes, u32 dwCreationDisposition, u32 dwFlagsAndAttributes, HANDLE hTemplateFile);
__declspec(dllimport) BOOL __stdcall GetFileTime(HANDLE hFile, FILETIME* lpCreationTime, FILETIME* lpLastAccessTime, FILETIME* lpLastWriteTime);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) void __stdcall Sleep(u32 dwMilliseconds);
__declspec(dllimport) HINSTANCE __stdcall LoadLibraryW(u16* lpLibFileName);
__declspec(dllimport) BOOL __stdcall FreeLibrary(HINSTANCE hLibModule);
__declspec(dllimport) void* __stdcall GetProcAddress(HINSTANCE hModule, u8* lpProcName);
__declspec(dllimport) int __stdcall MultiByteToWideChar(u32 CodePage, u32 dwFlags, u8* lpMultiByteStr, int cbMultiByte, u16* lpWideCharStr, int cchWideChar);
__declspec(dllimport) int __stdcall MessageBoxW(HWND hWnd, u16* lpText, u16* lpCaption, u32 uType);
__declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, size_t dwSize, u32 flAllocationType, u32 flProtect);
__declspec(dllimport) BOOL __stdcall VirtualFree(void* lpAddress, size_t dwSize, u32 dwFreeType);
__declspec(dllimport) HANDLE __stdcall GetClipboardData(u32 uFormat);
__declspec(dllimport) void* __stdcall GlobalLock(HANDLE hMem);
__declspec(dllimport) int __stdcall WideCharToMultiByte(u32 CodePage, u32 dwFlags, u16* lpWideCharStr, int cchWideChar, u8* lpMultiByteStr, int cbMultiByte, u8* lpDefaultChar, BOOL* lpUsedDefaultChar);
__declspec(dllimport) u32 __stdcall GetFileAttributesW(u16* lpFileName);
__declspec(dllimport) u32 __stdcall GetFinalPathNameByHandleW(HANDLE hFile, u16* lpszFilePath, u32 cchFilePath, u32 dwFlags);
__declspec(dllimport) u32 __stdcall GetLastError(void);
__declspec(dllimport) BOOL __stdcall FindClose(HANDLE hFindFile);
__declspec(dllimport) BOOL __stdcall ReadFile(HANDLE hFile, void* lpBuffer, u32 nNumberOfBytesToRead, u32* lpNumberOfBytesRead, void* lpOverlapped);
__declspec(dllimport) BOOL __stdcall GetFileSizeEx(HANDLE hFile, LARGE_INTEGER* lpFileSize);
__declspec(dllimport) BOOL __stdcall SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, LARGE_INTEGER* lpNewFilePointer, u32 dwMoveMethod);
__declspec(dllimport) u32 __stdcall SearchPathW(u16* lpPath, u16* lpFileName, u16* lpExtension, u32 nBufferLength, u16* lpBuffer, u16** lpFilePart);
__declspec(dllimport) BOOL __stdcall CreateProcessW( u16* lpApplicationName, u16* lpCommandLine, SECURITY_ATTRIBUTES* lpProcessAttributes, SECURITY_ATTRIBUTES* lpThreadAttributes, BOOL bInheritHandles, u32 dwCreationFlags, void* lpEnvironment, u16* lpCurrentDirectory, STARTUPINFOW* lpStartupInfo, PROCESS_INFORMATION* lpProcessInformation);
__declspec(dllimport) u32 __stdcall WaitForSingleObject(HANDLE hHandle, u32 dwMilliseconds);
__declspec(dllimport) BOOL __stdcall GetExitCodeProcess(HANDLE hProcess, u32* lpExitCode);
__declspec(dllimport) i64 __stdcall GetWindowLongPtrW(HWND hWnd, int nIndex);
__declspec(dllimport) u32 __stdcall GetRawInputData(HRAWINPUT hRawInput, u32 uiCommand, void* pData, u32* pcbSize, u32 cbSizeHeader);
__declspec(dllimport) HICON __stdcall LoadCursorW(HINSTANCE hInstance, u16* lpCursorName);
__declspec(dllimport) u16 __stdcall RegisterClassExW(WNDCLASSEXW*);
__declspec(dllimport) BOOL __stdcall ShowWindow(HWND hWnd, int nCmdShow);
__declspec(dllimport) BOOL __stdcall RegisterRawInputDevices(RAWINPUTDEVICE* pRawInputDevices, u32 uiNumDevices, u32 cbSize);
__declspec(dllimport) BOOL __stdcall SetProcessDPIAware(void);
__declspec(dllimport) BOOL __stdcall AdjustWindowRectEx(RECT* lpRect, u32 dwStyle, BOOL bMenu, u32 dwExStyle);
__declspec(dllimport) BOOL __stdcall PeekMessageW(MSG* lpMsg, HWND hWnd, u32 wMsgFilterMin, u32 wMsgFilterMax, u32 wRemoveMsg);
__declspec(dllimport) BOOL __stdcall TranslateMessage(MSG* lpMsg);
__declspec(dllimport) i64 __stdcall SetWindowLongPtrW(HWND hWnd, int nIndex, i64 dwNewLong);
__declspec(dllimport) i16 __stdcall GetKeyState(int nVirtKey);
__declspec(dllimport) BOOL __stdcall SetCursorPos(int X, int Y);
__declspec(dllimport) int __stdcall ShowCursor(BOOL bShow);
__declspec(dllimport) BOOL __stdcall GetCursorPos(POINT* lpPoint);
__declspec(dllimport) BOOL __stdcall IsDebuggerPresent(void);
__declspec(dllimport) BOOL __stdcall GetConsoleScreenBufferInfo(HANDLE hConsoleOutput, CONSOLE_SCREEN_BUFFER_INFO* lpConsoleScreenBufferInfo);
__declspec(dllimport) BOOL __stdcall CopyFileW(u16* lpExistingFileName, u16* lpNewFileName, BOOL bFailIfExists);
__declspec(dllimport) BOOL __stdcall DeleteFileW(u16* lpFileName);
__declspec(dllimport) BOOL __stdcall SetCurrentDirectoryW(u16* lpPathName);
__declspec(dllimport) u32 __stdcall GetCurrentDirectoryW(u32 nBufferLength, u16* lpBuffer);
__declspec(dllimport) u32 __stdcall GetModuleFileNameW(HINSTANCE hModule, u16* lpFilename, u32 nSize);
__declspec(dllimport) BOOL __stdcall OpenClipboard(HWND hWndNewOwner);
__declspec(dllimport) BOOL __stdcall EmptyClipboard(void);
__declspec(dllimport) HANDLE __stdcall GlobalAlloc(u32 uFlags, size_t dwBytes); // ...do we need this? what's the purpose?
__declspec(dllimport) BOOL __stdcall GlobalUnlock(HANDLE hMem);
__declspec(dllimport) HANDLE __stdcall SetClipboardData(u32 uFormat, HANDLE hMem);
__declspec(dllimport) BOOL __stdcall CloseClipboard(void);
__declspec(dllimport) HANDLE __stdcall FindFirstFileW(u16* lpFileName, WIN32_FIND_DATAW* lpFindFileData);
__declspec(dllimport) BOOL __stdcall FindNextFileW(HANDLE hFindFile, WIN32_FIND_DATAW* lpFindFileData);
__declspec(dllimport) int __stdcall SHFileOperationW(SHFILEOPSTRUCTW* lpFileOp);
__declspec(dllimport) BOOL __stdcall CreateDirectoryW(u16* lpPathName, SECURITY_ATTRIBUTES* lpSecurityAttributes);
__declspec(dllimport) BOOL __stdcall CreatePipe(HANDLE* hReadPipe, HANDLE* hWritePipe, SECURITY_ATTRIBUTES* lpPipeAttributes, u32 nSize);
__declspec(dllimport) BOOL __stdcall SetHandleInformation(HANDLE hObject, u32 dwMask, u32 dwFlags);
__declspec(dllimport) BOOL __stdcall SetHandleInformation(HANDLE hObject, u32 dwMask, u32 dwFlags);
__declspec(dllimport) HINSTANCE __stdcall GetModuleHandleW(u16* lpModuleName);
__declspec(dllimport) HWND __stdcall CreateWindowExW(u32 dwExStyle, u16* lpClassName, u16* lpWindowName, u32 dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, void* lpParam);
__declspec(dllimport) i64 __stdcall DefWindowProcW(HWND hWnd, u32 Msg, u64 Param, i64 lParam);
__declspec(dllimport) BOOL __stdcall UpdateWindow(HWND hWnd);
__declspec(dllimport) i64 __stdcall DispatchMessageW(MSG* lpMsg);
__declspec(dllimport) HICON __stdcall SetCursor(HICON hCursor);
__declspec(dllimport) BOOL __stdcall ScreenToClient(HWND hWnd, POINT* lpPoint);
__declspec(dllimport) __declspec(noreturn) void __stdcall ExitProcess(u32 uExitCode);
// ------------------------
#endif

//DISABLE_PADDING_ERROR
//#include <DbgHelp.h>   tcc cannot find this file
//#define NOMINMAX
//#include <Windows.h>

#define FOS_SANE_MAX_PATH 1024

// -- Global state --

static U64 g_ticks_per_second;
static FOS_MouseCursor g_current_cursor;

static bool g_mouse_cursor_is_hidden;
static POINT g_mouse_cursor_hidden_pos;
static bool g_mouse_cursor_should_hide;

static FOS_Arena* g_temp_arena;

// ------------------

FOS_API void FOS_Init() {
#ifdef FOS_MODE_DEBUG
	OA_ALLOCATOR = oa_init(sizeof(MEMORY), OA_NODES, OA_FREE_NODES, Len(OA_NODES));
	FOS_MutexInit(&OA_MUTEX);
#endif
	
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_ticks_per_second);
	SymInitialize(GetCurrentProcess(), NULL, true);

	//Init();
	
	g_temp_arena = MakeArena(KIB(1));
}

FOS_API FOS_Tick FOS_Now() {
	FOS_Tick tick;
	QueryPerformanceCounter((LARGE_INTEGER*)&tick);
	return tick;
}

static wchar_t* StrToUTF16(String str, Int num_null_terminations, FOS_Arena* arena, Opt(Int*) out_len) {
	if (str.len == 0) {
		if (out_len) *out_len = 0;
		return NULL;
	}
	Assert(str.len < I32_MAX);

	wchar_t* w_text = NULL;

	int w_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)str.data, (int)str.len, NULL, 0);
	w_text = ArenaPushNUndef(wchar_t, arena, w_len + num_null_terminations);

	int w_len1 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)str.data, (int)str.len, w_text, w_len);

	Assert(w_len != 0 && w_len1 == w_len);

	memset(&w_text[w_len], 0, num_null_terminations * sizeof(wchar_t));

	if (out_len) *out_len = w_len;
	return w_text;
}

static String StrFromUTF16(wchar_t* str_utf16, FOS_Arena* arena) {
	if (*str_utf16 == 0) return (String) { 0 };

	int length = WideCharToMultiByte(CP_UTF8, 0, str_utf16, -1, NULL, 0, NULL, NULL);
	if (length <= 0) return (String) { 0 };

	Array(U8) result = {0};
	ArrayResizeUndef(arena, &result, length);  // length includes the null-termination.

	int length2 = WideCharToMultiByte(CP_UTF8, 0, str_utf16, -1, (char*)result.data, (int)result.len, NULL, NULL);
	if (length2 <= 0) return (String) { 0 };

	result.len--;
	return ArrTo(String, result);
}

FOS_API void FOS_MessageBox(String title, String message) {
	//U8 buf[4096]; // we can't use temp_push/F_ScopePop here, because we might be reporting an error about running over the temporary arena
	//FOS_Arena* stack_arena = MakeArenaStatic(buf, sizeof(buf));
	//wchar_t* title_utf16 = StrToUTF16(title, 1, stack_arena, NULL);
	//wchar_t* message_utf16 = StrToUTF16(message, 1, stack_arena, NULL);
	//MessageBoxW(0, message_utf16, title_utf16, MB_OK);
	//DestroyArena(stack_arena);
	// TODO: fixme ^^^^
	ArenaMark T = ArenaGetMark(g_temp_arena);
	wchar_t* title_utf16 = StrToUTF16(title, 1, g_temp_arena, NULL);
	wchar_t* message_utf16 = StrToUTF16(message, 1, g_temp_arena, NULL);
	MessageBoxW(0, message_utf16, title_utf16, MB_OK);
	ArenaSetMark(g_temp_arena, T);
}

FOS_API double FOS_DurationInSeconds(FOS_Tick start, FOS_Tick end) {
	Assert(end.tick >= start.tick);

	// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
	U64 elapsed = end.tick - start.tick;
	return (double)elapsed / (double)g_ticks_per_second;
}

static void FOS_ConsolePrint(FOS_Log* self, FOS_String str) { FOS_PrintString(str); };
FOS_API FOS_Log* FOS_Console() {
	static const FOS_Log log = {.print = FOS_ConsolePrint};
	return (FOS_Log*)&log;
};

static void FOS_DebugLogPrint(FOS_Log* self, FOS_String str) { FOS_DebugPrintString(str); };
FOS_API FOS_Log* FOS_DebugLog() {
	static const FOS_Log log = {.print = FOS_DebugLogPrint};
	return (FOS_Log*)&log;
}

FOS_API void FOS_PrintString(String str) {
	//Scope temp = ScopePush();

	//fuint str_utf16_len;
	//wchar_t* str_utf16 = StrToUTF16(str, 1, temp.arena, &str_utf16_len);
	//f_assert((u32)str_utf16_len == str_utf16_len);
	
	// NOTE: it seems like when we're capturing the output from an outside program,
	//       using WriteConsoleW doesn't get captured, but WriteFile does.
	U32 num_chars_written;
	//WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str_utf16, (u32)str_utf16_len * 2, &num_chars_written, NULL);

	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.data, (U32)str.len, &num_chars_written, NULL);
	//OutputDebugStringW(
	//ScopePop(temp);
}


FOS_API void FOS_DebugPrintString(String str) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	wchar_t* str_utf16 = StrToUTF16(str, 1, g_temp_arena, NULL);
	OutputDebugStringW(str_utf16);
	ArenaSetMark(g_temp_arena, T);
}

FOS_API void FOS_LogPrint(FOS_Log* log, const char* fmt, ...) {
	if (log) {
		ArenaMark T = ArenaGetMark(g_temp_arena);
		StringBuilder s = {.arena = g_temp_arena};

		va_list va; va_start(va, fmt);
		PrintVA(&s, fmt, va);
		va_end(va);

		log->print(log, s.str);
		ArenaSetMark(g_temp_arena, T);
	}
}

FOS_API void FOS_Print(const char* fmt, ...) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	StringBuilder s = {.arena = g_temp_arena};
	
	va_list va; va_start(va, fmt);
	PrintVA(&s, fmt, va);
	va_end(va);

	FOS_PrintString(s.str);
	ArenaSetMark(g_temp_arena, T);
}

FOS_API void FOS_DebugPrint(const char* fmt, ...) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	StringBuilder s = {.arena = g_temp_arena};
	
	va_list va; va_start(va, fmt);
	PrintVA(&s, fmt, va);
	va_end(va);
	
	FOS_DebugPrintString(s.str);
	ArenaSetMark(g_temp_arena, T);
}

// colored write to console
// WriteConsoleOutputAttribute
FOS_API void FOS_PrintStringColor(String str, FOS_ConsoleAttributeFlags attributes_mask) {
	if (str.len == 0) return;

	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	GetConsoleScreenBufferInfo(stdout_handle, &console_info);
	
	//SetConsoleTextAttribute(stdout_handle, attributes_mask);

	// @robustness: strings > 4GB
	U32 num_chars_written;
	WriteFile(stdout_handle, str.data, (U32)str.len, &num_chars_written, NULL);
	
	//SetConsoleTextAttribute(stdout_handle, console_info.wAttributes); // Restore original state
}

FOS_API U64 FOS_ReadCycleCounter() {
	U64 counter = 0;
	BOOL res = QueryPerformanceCounter((LARGE_INTEGER*)&counter);
	Assert(res);
	return counter;
}

static FOS_WorkingDir FOS_PushWorkingDir(FOS_Arena* arena, FOS_WorkingDir dir) {
	FOS_WorkingDir old_cwd = {0};
	if (dir.data) { //  != WORKING_DIR_USE_CURRENT
		Assert(FOS_PathIsAbsolute(dir));
		old_cwd = FOS_GetWorkingDir(arena);
		FOS_SetWorkingDir(old_cwd);
	}
	return old_cwd;
}

static void FOS_PopWorkingDir(FOS_WorkingDir old_cwd) {
	if (old_cwd.data) {
		FOS_SetWorkingDir(old_cwd);
	}
}

FOS_API bool FOS_FileModtime(FOS_WorkingDir working_dir, String filepath, FOS_FileTime* out_modtime) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);

	HANDLE h = CreateFileW(StrToUTF16(filepath, 1, g_temp_arena, NULL), 0, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (h != INVALID_HANDLE_VALUE) {
		GetFileTime(h, NULL, NULL, (FILETIME*)out_modtime);
		CloseHandle(h);
	}

	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return h != INVALID_HANDLE_VALUE;
}

FOS_API int FOS_FileCmpModtime(FOS_FileTime a, FOS_FileTime b) {
	return CompareFileTime((FILETIME*)&a, (FILETIME*)&b);
}

FOS_API bool FOS_CloneFile(FOS_WorkingDir working_dir, String dst_filepath, String src_filepath) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	src_filepath = StrReplace(g_temp_arena, src_filepath, L("/"), L("\\"));
	dst_filepath = StrReplace(g_temp_arena, dst_filepath, L("/"), L("\\"));
	
	wchar_t* src_filepath_w = StrToUTF16(src_filepath, 1, g_temp_arena, NULL);
	wchar_t* dst_filepath_w = StrToUTF16(dst_filepath, 1, g_temp_arena, NULL);
	
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);
	BOOL ok = CopyFileW(src_filepath_w, dst_filepath_w, 0) == 1;
	FOS_PopWorkingDir(old_cwd);
	
	ArenaSetMark(g_temp_arena, T);
	return ok;
}

FOS_API bool FOS_CloneDirectory(FOS_WorkingDir working_dir, String dst_path, String src_path) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);
	
	SHFILEOPSTRUCTW s = {
		.wFunc = FO_COPY,
		.pTo = StrToUTF16(dst_path, 2, g_temp_arena, NULL),
		.pFrom = StrToUTF16(src_path, 2, g_temp_arena, NULL),
		.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI,
	};
    int res = SHFileOperationW(&s);
	
	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return res == 0;
}

FOS_API bool FOS_DeleteFile(FOS_WorkingDir working_dir, String filepath) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);
	bool ok = DeleteFileW(StrToUTF16(filepath, 1, g_temp_arena, NULL)) == 1;
	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return ok;
}

FOS_API void FOS_Sleep(uint64_t ms) {
	Assert(ms < U32_MAX); // TODO
	Sleep((U32)ms);
}

FOS_API bool FOS_LoadDLL(FOS_WorkingDir working_dir, String filepath, FOS_DynamicLibrary* out_dll) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);
	
	HANDLE handle = LoadLibraryW(StrToUTF16(filepath, 1, g_temp_arena, NULL));
	*out_dll = (FOS_DynamicLibrary){handle};
	
	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return handle != NULL;
}

FOS_API void FOS_UnloadDLL(FOS_DynamicLibrary dll) {
	Assert(FreeLibrary((HINSTANCE)dll.handle) == 1);
}

FOS_API void* FOS_GetSymbolAddress(FOS_DynamicLibrary dll, String symbol) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	void* addr = GetProcAddress((HINSTANCE)dll.handle, StrToCStr(symbol, g_temp_arena));
	ArenaSetMark(g_temp_arena, T);
	return addr;
}

FOS_API bool FOS_FilePicker(FOS_Arena* arena, String* out_filepath) {
	wchar_t buffer[FOS_SANE_MAX_PATH];
	buffer[0] = 0;

	OPENFILENAMEW ofn = {
		.lStructSize = sizeof(OPENFILENAMEW),
		.hwndOwner = NULL,
		.lpstrFile = buffer,
		.nMaxFile = Len(buffer) - 1,
		.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0",
		.nFilterIndex = 1,
		.lpstrFileTitle = NULL,
		.nMaxFileTitle = 0,
		.lpstrInitialDir = NULL,
		.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
	};
	
	GetOpenFileNameW(&ofn);

	*out_filepath = StrFromUTF16(buffer, arena);
	return out_filepath->len > 0;
}

//Slice(String) os_file_picker_multi() {
//	ZoneScoped;
//	ASSERT(false); // cancelling does not work!!!!
//	return {};

	//Slice(u8) buffer = mem_alloc(16*KB, TEMP_ALLOCATOR);
	//buffer[0] = '\0';
	//
	//OPENFILENAMEA ofn = {};
	//ofn.lStructSize = sizeof(ofn);
	//ofn.hwndOwner = NULL;
	//ofn.lpstrFile = (char*)buffer.data;
	//ofn.nMaxFile = (u32)buffer.len - 1;
	//ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	//ofn.nFilterIndex = 1;
	//ofn.lpstrFileTitle = NULL;
	//ofn.nMaxFileTitle = 0;
	//ofn.lpstrInitialDir = NULL;
	//ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_NOCHANGEDIR;
	//GetOpenFileNameA(&ofn);
	//
	//Array(String) items = {};
	//items.arena = TEMP_ALLOCATOR;
	//
	//String directory = String{ buffer.data, (isize)strlen((char*)buffer.data) };
	//u8* ptr = buffer.data + directory.len + 1;
	//
	//if (*ptr == NULL) {
	//	if (directory.len == 0) return {};
	//	
	//	array_append(items, directory);
	//	return items.slice;
	//}
	//
	//i64 i = 0;
	//while (*ptr) {
	//	String filename = String{ ptr, (isize)strlen((char*)ptr) };
	//
	//	String fullpath = str_join(slice({ directory, LIT("\\"), filename }), TEMP_ALLOCATOR);
	//	assert(fullpath.len < 270);
	//	array_append(items, fullpath);
	//
	//	ptr += filename.len + 1;
	//	i++;
	//	assert(i < 1000);
	//}
	//
	//return items.slice;
//}

FOS_API bool FOS_SetWorkingDir(FOS_WorkingDir dir) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	BOOL ok = SetCurrentDirectoryW(StrToUTF16(dir, 1, g_temp_arena, NULL));
	ArenaSetMark(g_temp_arena, T);
	return ok;
}

FOS_API FOS_WorkingDir FOS_GetWorkingDir(FOS_Arena* arena) {
	wchar_t buf[FOS_SANE_MAX_PATH];
	buf[0] = 0;
	GetCurrentDirectoryW(FOS_SANE_MAX_PATH, buf);
	return StrFromUTF16(buf, arena);
}

FOS_API String FOS_GetExecutablePath(FOS_Arena* arena) {
	wchar_t buf[FOS_SANE_MAX_PATH];
	U32 n = GetModuleFileNameW(NULL, buf, FOS_SANE_MAX_PATH);
	Assert(n > 0 && n < FOS_SANE_MAX_PATH);
	return StrFromUTF16(buf, arena);
}

FOS_API String FOS_ClipboardGetText(FOS_Arena* arena) {
	U8Array text = {0};
	if (OpenClipboard(NULL)) {
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);

		/*u8* buffer = (u8*)*/GlobalLock(hData);

		int length = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)hData, -1, NULL, 0, NULL, NULL);
		if (length > 0) {
			ArrayResizeUndef(arena, &text, length);
			WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)hData, -1, (char*)text.data, length, NULL, NULL);
			text.len -= 1;
			Assert(text.data[text.len] == 0);
		}

		GlobalUnlock(hData);
		CloseClipboard();
	}
	return ArrTo(String, text);
}

FOS_API void FOS_ClipboardSetText(String text) {
	//ZoneScoped;
	if (!OpenClipboard(NULL)) return;
	ArenaMark T = ArenaGetMark(g_temp_arena);
	{
		EmptyClipboard();

		//int h = 0;
		//int length = MultiByteToWideChar(CP_UTF8, 0, (const char*)text.data, (int)text.len, NULL, 0);
		//
		//u8* utf16 = MakeSlice(u8, (fuint)length * 2 + 2, temp);
		//
		//ASSERT(MultiByteToWideChar(CP_UTF8, 0, (const char*)text.data, (int)text.len, (wchar_t*)utf16.data, length) == length);
		//((u16*)utf16.data)[length] = 0;
		Int utf16_len;
		wchar_t* utf16 = StrToUTF16(text, 1, g_temp_arena, &utf16_len);

		HANDLE clipbuffer = GlobalAlloc(0, utf16_len * 2 + 2);
		U8* buffer = (U8*)GlobalLock(clipbuffer);
		memcpy(buffer, utf16, utf16_len * 2 + 2);

		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_UNICODETEXT, clipbuffer);

		CloseClipboard();
	}
	ArenaSetMark(g_temp_arena, T);
}

FOS_API bool FOS_DirectoryExists(FOS_WorkingDir working_dir, String path) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);

	wchar_t* path_utf16 = StrToUTF16(path, 1, g_temp_arena, NULL);
	U32 dwAttrib = GetFileAttributesW(path_utf16);

	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

FOS_API bool FOS_PathIsAbsolute(String path) {
	return path.len > 2 && path.data[1] == ':';
}

FOS_API bool FOS_PathToCanonical(FOS_Arena* arena, FOS_WorkingDir working_dir, String path, String* out_canonical) {
	// https://pdh11.blogspot.com/2009/05/pathcanonicalize-versus-what-it-says-on.html
	// https://stackoverflow.com/questions/10198420/open-directory-using-createfile

	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);

	wchar_t* path_utf16 = StrToUTF16(path, 1, g_temp_arena, NULL);
	
	HANDLE file_handle = CreateFileW(path_utf16, 0, 0, NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	bool dummy_file_was_created = GetLastError() == 0;
	bool ok = file_handle != INVALID_HANDLE_VALUE;

	U16 result_utf16[FOS_SANE_MAX_PATH];
	if (ok) ok = GetFinalPathNameByHandleW(file_handle, result_utf16, FOS_SANE_MAX_PATH, VOLUME_NAME_DOS) < FOS_SANE_MAX_PATH;
	
	if (file_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(file_handle);
	}
	
	if (dummy_file_was_created) {
		DeleteFileW(result_utf16);
	}

	String result = { 0 };
	if (ok) {
		result = StrFromUTF16(result_utf16, arena);
		StrAdvance(&result, 4); // strings returned have `\\?\` - prefix that we should get rid of
		*out_canonical = result;
	}

	FOS_PopWorkingDir(old_cwd);
	if (arena != g_temp_arena) ArenaSetMark(g_temp_arena, T);
	return ok;
}

FOS_API bool FOS_VisitDirectory(FOS_WorkingDir working_dir, String path, FOS_VisitDirectoryVisitor visitor, void* visitor_userptr) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);

	U8Array match_str = {0};
	ArrayResizeUndef(g_temp_arena, &match_str, path.len + 2);
	memcpy(match_str.data, path.data, path.len);
	match_str.data[path.len] = '\\';
	match_str.data[path.len + 1] = '*';

	wchar_t* match_str_utf16 = StrToUTF16(ArrTo(String, match_str), 1, g_temp_arena, NULL);

	WIN32_FIND_DATAW find_info;
	HANDLE handle = FindFirstFileW(match_str_utf16, &find_info);

	bool ok = handle != INVALID_HANDLE_VALUE;
	if (ok) {
		for (; FindNextFileW(handle, &find_info);) {
			FOS_VisitDirectoryInfo info = {
				.name = StrFromUTF16(find_info.cFileName, g_temp_arena),
				.is_directory = find_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY,
			};

			if (StrEquals(info.name, L(".."))) continue;

			FOS_VisitDirectoryResult result = visitor(&info, visitor_userptr);
			Assert(result == fVisitDirectoryResult_Continue);
		}
		ok = GetLastError() == ERROR_NO_MORE_FILES;
		FindClose(handle);
	}

	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return ok;
}

FOS_API bool FOS_DeleteDirectory(FOS_WorkingDir working_dir, String path) {
	if (!FOS_DirectoryExists(working_dir, path)) return true;
	ArenaMark T = ArenaGetMark(g_temp_arena);
	// NOTE: path must be double null-terminated!
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);

	SHFILEOPSTRUCTW file_op = {
		.hwnd = NULL,
		.wFunc = FO_DELETE,
		.pFrom = StrToUTF16(path, 2, g_temp_arena, NULL),
		.pTo = NULL,
		.fFlags = FOF_NO_UI,
		.fAnyOperationsAborted = false,
		.hNameMappings = 0,
		.lpszProgressTitle = NULL,
	};
	int res = SHFileOperationW(&file_op);

	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return res == 0;
}

FOS_API bool FOS_MakeDirectory(FOS_WorkingDir working_dir, String path) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);

	wchar_t* path_utf16 = StrToUTF16(path, 1, g_temp_arena, NULL);
	bool created = CreateDirectoryW(path_utf16, NULL);
	
	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);

	if (created) return true;
	return GetLastError() == ERROR_ALREADY_EXISTS;
}

// static void FOS_FileUnbufferedWriterFn(Writer* writer, const void* data, Int size) {
// 	bool ok = FOS_FileWriteUnbuffered((FOS_File*)writer->userdata, (String) { (void*)data, size });
// 	Assert(ok);
// }

FOS_API bool FOS_FileOpen(FOS_WorkingDir working_dir, String filepath, FOS_FileOpenMode mode, FOS_File* out_file) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	FOS_WorkingDir old_cwd = FOS_PushWorkingDir(g_temp_arena, working_dir);
	wchar_t* filepath_utf16 = StrToUTF16(filepath, 1, g_temp_arena, NULL);

	out_file->mode = mode;

	if (mode == FOS_FileOpenMode_Read) {
		out_file->os_handle = CreateFileW(filepath_utf16, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	}
	else {
		U32 creation = mode == FOS_FileOpenMode_Append ? OPEN_ALWAYS : CREATE_ALWAYS;
		out_file->os_handle = CreateFileW(filepath_utf16, FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, creation, 0, NULL);
	}

	bool ok = out_file->os_handle != INVALID_HANDLE_VALUE;
	if (ok) {
		//f_leak_tracker_begin_entry(out_file->os_handle, 1);

		if (mode != FOS_FileOpenMode_Read) {
			TODO(); // out_file->backing_writer = (Writer){FOS_FileUnbufferedWriterFn, out_file};
			// OpenBufferedWriter(&out_file->backing_writer, &out_file->buffer[0], FILE_WRITER_BUFFER_SIZE, &out_file->buffered_writer);
		}
	}

	FOS_PopWorkingDir(old_cwd);
	ArenaSetMark(g_temp_arena, T);
	return ok;
}

FOS_API size_t FOS_FileRead(FOS_File* file, void* dst, size_t size) {
	if (dst == NULL) return 0;
	if (size == 0) return 0;

	for (size_t read_so_far = 0; read_so_far < size;) {
		size_t remaining = size - read_so_far;
		U32 to_read = remaining >= U32_MAX ? U32_MAX : (U32)remaining;

		U32 bytes_read;
		BOOL ok = ReadFile(file->os_handle, (U8*)dst + read_so_far, to_read, &bytes_read, NULL);
		read_so_far += bytes_read;

		if (ok != 1 || bytes_read < to_read) {
			return read_so_far;
		}
	}
	return size;
}

FOS_API uint64_t FOS_FileSize(FOS_File* file) {
	U64 size;
	if (GetFileSizeEx(file->os_handle, (LARGE_INTEGER*)&size) != 1) return -1;
	return size;
}

FOS_API bool FOS_FileWriteUnbuffered(FOS_File* file, String data) {
	if (data.len >= U32_MAX) return false; // TODO: writing files greater than 4 GB

	U32 bytes_written;
	return WriteFile(file->os_handle, data.data, (U32)data.len, &bytes_written, NULL) == 1 && bytes_written == data.len;
}

FOS_API uint64_t FOS_FileGetCursor(FOS_File* file) {
	U64 offset;
	if (SetFilePointerEx(file->os_handle, (LARGE_INTEGER){0}, (LARGE_INTEGER*)&offset, FILE_CURRENT) != 1) return -1;
	return offset;
}

FOS_API bool FOS_FileSetCursor(FOS_File* file, uint64_t position) {
	return SetFilePointerEx(file->os_handle, *(LARGE_INTEGER*)&position, NULL, FILE_BEGIN) == 1;
}

FOS_API void FOS_FileFlush(FOS_File* file) {
	Assert(file->mode != FOS_FileOpenMode_Read);
	TODO(); //FlushBufferedWriter(&file->buffered_writer);
}

FOS_API bool FOS_FileClose(FOS_File* file) {
	if (file->mode != FOS_FileOpenMode_Read) {
		FOS_FileFlush(file);
	}

	bool ok = CloseHandle(file->os_handle) == 1;
	//f_leak_tracker_end_entry(file->os_handle);
	return ok;
}

#if 0
FOS_API DebugStackFrameSlice FOS_DebugGetStackTrace(FOS_Arena* arena) {
	HANDLE process = GetCurrentProcess();

	// NOTE: for any of this to work, `SymInitialize` must have been called! We do this in FOS_Init().

	CONTEXT ctx;
	RtlCaptureContext(&ctx);

	STACKFRAME64 stack_frame = (STACKFRAME64){
		.AddrPC.Offset = ctx.Rip,
		.AddrPC.Mode = AddrModeFlat,
		.AddrFrame.Offset = ctx.Rsp,
		.AddrFrame.Mode = AddrModeFlat,
		.AddrStack.Offset = ctx.Rsp,
		.AddrStack.Mode = AddrModeFlat,
	};
	
	Array(FOS_DebugStackFrame) frames = {0};

	for (Int i = 0; i < 64; i++) {
		if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &stack_frame, &ctx,
			NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
			// Maybe it failed, maybe we have finished walking the stack.
			break;
		}

		if (stack_frame.AddrPC.Offset == 0) break;
		if (i == 0) continue; // ignore this function

		struct {
			IMAGEHLP_SYMBOL64 s;
			U8 name_buf[64];
		} sym;

		sym.s.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		sym.s.MaxNameLength = 64;

		if (SymGetSymFromAddr(process, stack_frame.AddrPC.Offset, NULL, &sym.s) != TRUE) break;

		IMAGEHLP_LINE64 Line64;
		Line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD dwDisplacement;
		if (SymGetLineFromAddr64(process, ctx.Rip, &dwDisplacement, &Line64) != TRUE) break;

		FOS_DebugStackFrame frame = {
			.function = StrClone(arena, CStrToStr(sym.s.Name)),
			.file = StrClone(arena, CStrToStr(Line64.FileName)),
			.line = Line64.LineNumber,
		};
		ArrayPush(arena, &frames, frame);

		if (StrEquals(frame.function, L("main"))) break; // Ignore everything past main
	}

	return ArrTo(DebugStackFrameSlice, frames);
}
#endif

FOS_API String FOS_FindExecutable(FOS_Arena* arena, String name) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	wchar_t* name_utf16 = StrToUTF16(name, 1, g_temp_arena, NULL);

	wchar_t path[FOS_SANE_MAX_PATH] = {0};
	U32 dwRet = SearchPathW(NULL, name_utf16, NULL, FOS_SANE_MAX_PATH, path, NULL);
	if (arena != g_temp_arena) ArenaSetMark(g_temp_arena, T);

	if (dwRet == 0 || dwRet >= FOS_SANE_MAX_PATH) {
		return (String) { 0 };
	}
	return StrFromUTF16(path, arena);
}

FOS_API bool FOS_RunCommand(FOS_WorkingDir working_dir, FOS_String* args, uint32_t args_count, uint32_t* out_exit_code, FOS_Log* stdout_log, FOS_Log* stderr_log) {
	ArenaMark T = ArenaGetMark(g_temp_arena);
	
	// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
	
	// Windows expects a single space-separated string that encodes a list of the passed command-line arguments.
	// In order to support spaces within an argument, we must enclose it with quotation marks (").
	// This escaping method is the absolute dumbest thing that has ever existed.
	// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?redirectedfrom=MSDN&view=msvc-170
	// https://stackoverflow.com/questions/1291291/how-to-accept-command-line-args-ending-in-backslash
	// https://daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC

	StringBuilder cmd_string = {.arena = g_temp_arena};

	for (U32 i = 0; i < args_count; i++) {
		String arg = args[i];

		bool contains_space = false;

		StringBuilder arg_string = {.arena = g_temp_arena};
		for (Int j = 0; j < arg.len; j++) {
			U8 c = arg.data[j];
			if (c == '\"') {
				PrintB(&arg_string, '\\'); // escape quotation marks with a backslash
			}
			else if (c == '\\') {
				if (j + 1 == arg.len) {
					// if we have a backslash and it's the last character in the string,
					// we must push \\"
					PrintC(&arg_string, "\\\\");
					break;
				}
				else if (arg.data[j + 1] == '\"') {
					// if we have a backslash and the next character is a quotation mark,
					// we must push \\\"
					PrintC(&arg_string, "\\\\\\\"");
					j++; // also skip the next "
					continue;
				}
			}
			else if (c == ' ') {
				contains_space = true;
			}

			PrintB(&arg_string, c);
		}

		// Only add quotations if the argument contains spaces. You might ask why this matters: couldn't we always just add the quotation?
		// And yes, if the program is only using `argc` and `argv` style parameters, it does not matter. However, if it uses the GetCommandLine FOS_API or
		// the `lpCmdLine` parameter in WinMain, then the argument string will be used as a single string and parsed manually by the program.
		// And so, some programs, for example Visual Studio (devenv.exe), will fail to parse arguments correctly if they're quoted.
		Print(&cmd_string, contains_space ? "\"" : "");
		PrintS(&cmd_string, arg_string.str);
		Print(&cmd_string, contains_space ? "\"" : "");

		if (i < args_count - 1) PrintB(&cmd_string, ' '); // Separate each argument with a space
	}

	wchar_t* cmd_string_utf16 = StrToUTF16(cmd_string.str, 1, g_temp_arena, NULL);
	wchar_t* working_dir_utf16 = StrToUTF16(working_dir, 1, g_temp_arena, NULL);

	PROCESS_INFORMATION process_info = {0};

	// Initialize pipes
	SECURITY_ATTRIBUTES security_attrs = {
		.nLength = sizeof(SECURITY_ATTRIBUTES),
		.lpSecurityDescriptor = NULL,
		.bInheritHandle = 1,
	};

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

	STARTUPINFOW startup_info = {
		.cb = sizeof(STARTUPINFOW),
		.dwFlags = STARTF_USESTDHANDLES,
		.hStdOutput = OUT_Wr,
		.hStdError = stdout_log == stderr_log ? OUT_Wr : ERR_Wr, // When redirecting both stdout AND stderr into the same writer, we want to use the same pipe for both of them to get proper order in the output.
		.hStdInput = GetStdHandle(STD_INPUT_HANDLE),
	};
	
	if (ok) ok = CreateProcessW(NULL, cmd_string_utf16, NULL, NULL, true, CREATE_UNICODE_ENVIRONMENT, NULL, working_dir_utf16, &startup_info, &process_info);
	
	// We don't need these handles for ourselves and we must close them to say that we won't be using them, to let `ReadFile` exit
	// when the process finishes instead of locking. At least that's how I think it works.
	if (OUT_Wr) CloseHandle(OUT_Wr);
	if (ERR_Wr) CloseHandle(ERR_Wr);
	
	if (ok) {
		U8 buf[512];
		U32 num_read_bytes;
		
		// StringBuilder stdout = {.arena = arena};
		// StringBuilder stderr = {.arena = arena};

		for (;;) {
			if (!ReadFile(OUT_Rd, buf, sizeof(buf), &num_read_bytes, NULL)) break;
			if (stdout_log) {
				stdout_log->print(stdout_log, (String){buf, num_read_bytes});
			}
		}
		
		for (;;) {
			if (!ReadFile(ERR_Rd, buf, sizeof(buf), &num_read_bytes, NULL)) break;
			if (stderr_log && stdout_log != stderr_log) {
				stderr_log->print(stderr_log, (String){buf, num_read_bytes});
			}
		}
		
		WaitForSingleObject(process_info.hProcess, INFINITE);
		
		if (out_exit_code && !GetExitCodeProcess(process_info.hProcess, out_exit_code)) ok = false;

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);
	}
	
	if (OUT_Rd) CloseHandle(OUT_Rd);
	if (ERR_Rd) CloseHandle(ERR_Rd);
	
	ArenaSetMark(g_temp_arena, T);
	return ok;
}

// Window creation & input ------------------

typedef struct WindowProcUserData {
	FOS_Window* window;

	Opt(FOS_WindowOnResize) on_resize;
	void* on_resize_user_ptr;
	
	bool should_exit;
Pad(7);
} WindowProcUserData;

static void UpdateKeyInputState(FOS_Inputs* inputs, U64 vk, FOS_InputStateFlags flags) {
	FOS_Input input = FOS_Input_Invalid;

	if ((vk >= FOS_Input_0 && vk <= FOS_Input_9) || (vk >= FOS_Input_A && vk <= FOS_Input_Z)) {
		input = (FOS_Input)vk;
	}
	else switch (vk) {
	case VK_SPACE: input = FOS_Input_Space; break;
	case VK_OEM_7: input = FOS_Input_Apostrophe; break;
	case VK_OEM_COMMA: input = FOS_Input_Comma; break;
	case VK_OEM_MINUS: input = FOS_Input_Minus; break;
	case VK_OEM_PERIOD: input = FOS_Input_Period; break;
	case VK_OEM_2: input = FOS_Input_Slash; break;
	case VK_OEM_1: input = FOS_Input_Semicolon; break;
	case VK_OEM_PLUS: input = FOS_Input_Equal; break;
	case VK_OEM_4: input = FOS_Input_LeftBracket; break;
	case VK_OEM_5: input = FOS_Input_Backslash; break;
	case VK_OEM_6: input = FOS_Input_RightBracket; break;
	case VK_OEM_3: input = FOS_Input_GraveAccent; break;
	case VK_ESCAPE: input = FOS_Input_Escape; break;
	case VK_RETURN: input = FOS_Input_Enter; break;
	case VK_TAB: input = FOS_Input_Tab; break;
	case VK_BACK: input = FOS_Input_Backspace; break;
	case VK_INSERT: input = FOS_Input_Insert; break;
	case VK_DELETE: input = FOS_Input_Delete; break;
	case VK_RIGHT: input = FOS_Input_Right; break;
	case VK_LEFT: input = FOS_Input_Left; break;
	case VK_DOWN: input = FOS_Input_Down; break;
	case VK_UP: input = FOS_Input_Up; break;
	case VK_PRIOR: input = FOS_Input_PageUp; break;
	case VK_NEXT: input = FOS_Input_PageDown; break;
	case VK_HOME: input = FOS_Input_Home; break;
	case VK_END: input = FOS_Input_End; break;
	case VK_CAPITAL: input = FOS_Input_CapsLock; break;
	case VK_NUMLOCK: input = FOS_Input_NumLock; break;
	case VK_SNAPSHOT: input = FOS_Input_PrintScreen; break;
	case VK_PAUSE: input = FOS_Input_Pause; break;
	case VK_SHIFT: {
		// TODO: I think we can also detect the right-vs-left from lParam bit 24
		input = GetKeyState(VK_RSHIFT) & 0x8000 ? FOS_Input_RightShift : FOS_Input_LeftShift;
		inputs->input_states[FOS_Input_Shift] = flags;
	} break;
	case VK_MENU: {
		input = GetKeyState(VK_RMENU) & 0x8000 ? FOS_Input_RightAlt : FOS_Input_LeftAlt;
		inputs->input_states[FOS_Input_Alt] = flags;
	} break;
	case VK_CONTROL: {
		input = GetKeyState(VK_RCONTROL) & 0x8000 ? FOS_Input_RightControl : FOS_Input_LeftControl;
		inputs->input_states[FOS_Input_Control] = flags;
	} break;
	case VK_LWIN: {
		input = FOS_Input_LeftSuper;
		inputs->input_states[FOS_Input_Super] = flags;
	} break;
	case VK_RWIN: {
		input = FOS_Input_RightSuper;
		inputs->input_states[FOS_Input_Super] = flags;
	} break;

	default: break;
	}
	inputs->input_states[input] = flags;
}

static I64 FOS_WindowProc(HWND hWnd, U32 uMsg, U64 wParam, I64 lParam) {
	Opt(WindowProcUserData*) passed = (WindowProcUserData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	Opt(FOS_Inputs*) inputs = passed ? &passed->window->inputs : NULL;

	switch (uMsg) {
	case WM_INPUT: {
		
		RAWINPUT* raw_input = &(RAWINPUT){0};
		U32 dwSize = sizeof(RAWINPUT);
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw_input, &dwSize, sizeof(RAWINPUTHEADER)));

		// Avoid using arena allocations here to get more deterministic counters for debugging.
		//U32 dwSize;
		//GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		//
		//Scope T = ScopePush(NULL);
		//RAWINPUT* raw_input = MemAlloc(dwSize, g_temp_arena);
		//if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw_input, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
		//	Assert(0);
		//}

		if (raw_input->header.dwType == RIM_TYPEMOUSE &&
			!(raw_input->data.mouse.usFlags & MOUSE_MOVE_RELATIVE) &&
			!(raw_input->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) // For instance, drawing tables can give absolute mouse inputs which we want to ignore
		) {
			inputs->mouse_raw_dx = raw_input->data.mouse.lLastX;
			inputs->mouse_raw_dy = raw_input->data.mouse.lLastY;
		}

		//ScopePop(T);
	} break;

	case WM_UNICHAR: {
		Assert(false);
	} break;

	case WM_CHAR: {
		// wParam contains the character as a 32-bit unicode codepoint.
		if (wParam >= 32 && inputs->characters_count < Len(inputs->characters) &&
			wParam != 127 /* ignore the DEL character on ctrl-backspace */)
		{
			inputs->characters[inputs->characters_count++] = (Rune)wParam;
		}
	} break;

	case WM_SETCURSOR: {
		
		// 
		//SetCursor(LoadCursorW(NULL, IDC_HAND));
		//SetCursor(g_current_cursor);


		//if (IsIconic(hwnd)) // when not minimized
		//{
		//	break;
		//}
	} break;

	case WM_SIZE: {
		// "The low-order word of lParam specifies the new width of the client area."
		// "The high-order word of lParam specifies the new height of the client area."
			
		U32 width = (U32)(lParam);
		U32 height = (U32)(lParam >> 16);
		if (passed && passed->on_resize) { // NOTE: `passed` will be NULL during window creation
			passed->on_resize(width, height, passed->on_resize_user_ptr);
		}
	} break;

	case WM_CLOSE: // fallthrough
	case WM_QUIT: {
		passed->should_exit = true;
	} break;
		
	case WM_SYSKEYDOWN: // fallthrough
	case WM_KEYDOWN: {
		bool is_repeat = (lParam & (1 << 30)) != 0;
		
		FOS_InputStateFlags flags = is_repeat ?
			FOS_InputStateFlag_IsPressed | FOS_InputStateFlag_DidRepeat :
			FOS_InputStateFlag_IsPressed | FOS_InputStateFlag_DidBeginPress;

		UpdateKeyInputState(inputs, wParam, flags);
	} break;
	case WM_SYSKEYUP: // fallthrough
	case WM_KEYUP: {
		// Right now, if you press and release a key within a single frame, our input system will miss that.
		// TODO: either make it an event list, or fix this (but still disallow lose information about multiple keypresses per frame).
		UpdateKeyInputState(inputs, wParam, FOS_InputStateFlag_DidEndPress);
	} break;

	// NOTE: releasing mouse buttons is handled manually :ManualMouseReleaseEvents
	case WM_LBUTTONDOWN: { inputs->input_states[FOS_Input_MouseLeft] = FOS_InputStateFlag_DidBeginPress | FOS_InputStateFlag_IsPressed; } break;
	case WM_RBUTTONDOWN: { inputs->input_states[FOS_Input_MouseRight] = FOS_InputStateFlag_DidBeginPress | FOS_InputStateFlag_IsPressed; } break;
	case WM_MBUTTONDOWN: { inputs->input_states[FOS_Input_MouseMiddle] = FOS_InputStateFlag_DidBeginPress | FOS_InputStateFlag_IsPressed; } break;

	case WM_MOUSEWHEEL: {
		if (passed) {
			I16 wheel = (I16)(wParam >> 16);
			inputs->mouse_wheel = (float)wheel / (float)WHEEL_DELTA;
		}
	} break;
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

#define FOS_WINDOW_CLASS_NAME L"FOS_WindowModuleClassName"

static void FOS_RegisterWindowClass() {
	// Windows requires us to first register a "window class" with a given name,
	// which will be used in subsequent calls to CreateWindowExW()
	
	HINSTANCE hInst = GetModuleHandleW(NULL);
	
	//#COLOR_BACKGROUND: 1
	//bg_brush: HBRUSH(COLOR_BACKGROUND)
	
	// leave the background brush to NULL
	// https://stackoverflow.com/questions/6593014/how-to-draw-opengl-content-while-resizing-win32-window
	//wnd_class.hbrBackground
	
	WNDCLASSEXW wnd_class = {
		.cbSize =          sizeof(WNDCLASSEXW),
		.style =           CS_HREDRAW | CS_VREDRAW | CS_OWNDC, // CS_OWNDC is required for OpenGL
		.lpfnWndProc =     FOS_WindowProc,
		.cbClsExtra =      0,
		.cbWndExtra =      0,
		.hInstance =       hInst,
		.hIcon =           NULL,
		.hCursor =         LoadCursorW(0, IDC_ARROW),
		.hbrBackground =   NULL,
		.lpszMenuName =    NULL,
		.lpszClassName =   FOS_WINDOW_CLASS_NAME,
		.hIconSm =         NULL,
	};

	U64 atom = RegisterClassExW(&wnd_class);
	Assert(atom != 0);
}

FOS_API void FOS_WindowShow_(FOS_Window* window) {
	Assert(UpdateWindow((HWND)window->handle) != 0);
	ShowWindow((HWND)window->handle, SW_SHOW);
}

FOS_API FOS_Window FOS_WindowCreate(uint32_t width, uint32_t height, String name) {
	FOS_Window window = FOS_WindowCreateHidden(width, height, name);
	FOS_WindowShow_(&window);
	return window;
}

FOS_Window FOS_WindowCreateHidden(uint32_t width, uint32_t height, String name) {
	// register raw input
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/using-raw-input
	
	RAWINPUTDEVICE input_devices[1];
	input_devices[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
	input_devices[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
	input_devices[0].dwFlags = 0/*RIDEV_NOLEGACY*/;    // adds mouse and also ignores legacy mouse messages
	input_devices[0].hwndTarget = 0;
	Assert(RegisterRawInputDevices(input_devices, Len(input_devices), sizeof(RAWINPUTDEVICE)) == 1);

	// NOTE: When you use a DPI scale on windows that's not 1, the window that we get back from `CreateWindowExW`
	// has an incorrect size that's not what we ask for.
	// Calling `SetProcessDPIAware` seems to resolve this issue, at least for the single monitor case.
	// 
	// TODO: fix for multiple monitors
	// https://stackoverflow.com/questions/71300163/how-to-create-a-window-with-createwindowex-but-ignoring-the-scale-settings-on-wi
	Assert(SetProcessDPIAware() == 1);
	
	// TODO: multiple windows?
	FOS_RegisterWindowClass();
	
	I32 x = 200;
	I32 y = 200;
	RECT rect = {x, y, x + (I32)width, y + (I32)height};
	
	// AdjustWindowRect modifies the window rectangle: we give it the client area rectangle, it gives us back the entire window rectangle.
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 0, 0);

	ArenaMark T = ArenaGetMark(g_temp_arena);
	
	HWND hwnd = CreateWindowExW(0,
		FOS_WINDOW_CLASS_NAME,
		StrToUTF16(name, 1, g_temp_arena, NULL),
		WS_OVERLAPPEDWINDOW,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, 0, 0);
	
	Assert(hwnd != 0);
	
	ArenaSetMark(g_temp_arena, T);
	FOS_Window result = {.handle = (FOS_WindowHandle)hwnd};
	return result;
}

void FOS_SetMouseCursor(FOS_MouseCursor cursor) { g_current_cursor = cursor; }

void FOS_LockAndHideMouseCursor() { g_mouse_cursor_should_hide = true; }

bool FOS_WindowPoll(FOS_Window* window, Opt(FOS_WindowOnResize) on_resize, Opt(void*) on_resize_user_ptr) {
	WindowProcUserData passed = {
		.window = window,
		.on_resize = on_resize,
		.on_resize_user_ptr = on_resize_user_ptr,
		.should_exit = false,
	};

	window->inputs.characters_count = 0;
	window->inputs.mouse_raw_dx = 0;
	window->inputs.mouse_raw_dy = 0;
	window->inputs.mouse_wheel = 0;
	
	FOS_InputStateFlags remove_flags = FOS_InputStateFlag_DidEndPress | FOS_InputStateFlag_DidBeginPress | FOS_InputStateFlag_DidRepeat;
	for (Int i = 0; i < FOS_Input_COUNT; i++) {
		FOS_InputStateFlags* flags = &window->inputs.input_states[i];
		*flags &= ~remove_flags;
	}

	for (;;) {
		MSG msg = {0};
		BOOL result = PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
		if (result != 0) {
			TranslateMessage(&msg);
			
			// https://stackoverflow.com/questions/117792/best-method-for-storing-this-pointer-for-use-in-wndproc
			SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, (I64)&passed);
			DispatchMessageW(&msg);
		}
		else break;
		// redraw window
	}
	
	// :ManualMouseReleaseEvents
	// Manually check for mouse release, because we won't receive a release event if you release the mouse outside of the window.
	// Microsoft advices to use `SetCapture` / `ReleaseCapture`, but that seems like it'd be error prone when there are multiple
	// Set/ReleaseCaptures live at the same time when holding multiple mouse buttons.
	{
		if (FOS_IsPressed(&window->inputs, FOS_Input_MouseLeft) && !(GetKeyState(VK_LBUTTON) & 0x8000)) {
			window->inputs.input_states[FOS_Input_MouseLeft] = FOS_InputStateFlag_DidEndPress;
		}
		if (FOS_IsPressed(&window->inputs, FOS_Input_MouseMiddle) && !(GetKeyState(VK_MBUTTON) & 0x8000)) {
			window->inputs.input_states[FOS_Input_MouseMiddle] = FOS_InputStateFlag_DidEndPress;
		}
		if (FOS_IsPressed(&window->inputs, FOS_Input_MouseRight) && !(GetKeyState(VK_RBUTTON) & 0x8000)) {
			window->inputs.input_states[FOS_Input_MouseRight] = FOS_InputStateFlag_DidEndPress;
		}
	}
	
	/*
	 For the most consistent way of setting the mouse cursor, we're doing it here. Setting it directly in `FOS_SetMouseCursor` would work as well,
	 but that could introduce flickering in some cases.
	*/
	if (g_current_cursor != FOS_MouseCursor_Arrow) {
		U16* cursor_name;
		switch (g_current_cursor) {
		case FOS_MouseCursor_Arrow: cursor_name = IDC_ARROW; break;
		case FOS_MouseCursor_Hand: cursor_name = IDC_HAND; break;
		case FOS_MouseCursor_I_beam: cursor_name = IDC_IBEAM; break;
		case FOS_MouseCursor_Crosshair: cursor_name = IDC_CROSS; break;
		case FOS_MouseCursor_ResizeH: cursor_name = IDC_SIZEWE; break;
		case FOS_MouseCursor_ResizeV: cursor_name = IDC_SIZENS; break;
		case FOS_MouseCursor_ResizeNESW: cursor_name = IDC_SIZENESW; break;
		case FOS_MouseCursor_ResizeNWSE: cursor_name = IDC_SIZENWSE; break;
		case FOS_MouseCursor_ResizeAll: cursor_name = IDC_SIZEALL; break;
		case FOS_MouseCursor_COUNT: break;
		}
		SetCursor(LoadCursorW(NULL, cursor_name));
		g_current_cursor = FOS_MouseCursor_Arrow;
	}

	// Lock & hide mouse
	{
		if (g_mouse_cursor_is_hidden) {
			SetCursorPos(g_mouse_cursor_hidden_pos.x, g_mouse_cursor_hidden_pos.y);
		}

		if (g_mouse_cursor_should_hide != g_mouse_cursor_is_hidden) {
			if (g_mouse_cursor_should_hide) {
				ShowCursor(0);
				GetCursorPos(&g_mouse_cursor_hidden_pos);
			}
			else {
				ShowCursor(1);
			}
		}
		g_mouse_cursor_is_hidden = g_mouse_cursor_should_hide;
		g_mouse_cursor_should_hide = false;
	}
	
	{
		POINT cursor_pos;
		GetCursorPos(&cursor_pos);
		ScreenToClient((HWND)window->handle, &cursor_pos);
		window->inputs.mouse_x = cursor_pos.x;
		window->inputs.mouse_y = cursor_pos.y;
	}

	return !passed.should_exit;
}

// ----------------------------------------------------------------------------------------------------------------

static U32 FOS_ThreadEntryFn(void* args) {
	// Init(); // Init thread-local variables, i.e. the temporary arenas

	FOS_Thread* thread = args;
	thread->fn(thread->user_ptr);

	// Deinit();
	_endthreadex(0);
	return 0;
}

FOS_API void FOS_ThreadStart(FOS_Thread* thread, FOS_ThreadFn fn, void* user_ptr, String debug_name) {
	Assert(thread->os_specific == NULL);

	ArenaMark T = ArenaGetMark(g_temp_arena);

	// FOS_ThreadEntryFn might be called at any time after calling `_beginthreadex`, and we must pass `fn` and `user_ptr` to it somehow. We can't pass them
	// on the stack, because this function might have exited at the point the ThreadEntryFn is entered. So, let's pass them in the FOS_Thread structure
	// and let the user manage the FOS_Thread pointer. Alternatively, we could make the user manually call FOS_ThreadEnter(); and FOS_ThreadExit(); inside
	// their thread function and directly pass the user function into _beginthreadex.
	thread->fn = fn;
	thread->user_ptr = user_ptr;

	// We're using _beginthreadex instead of CreateThread, because we're using the C runtime library.
	// https://stackoverflow.com/questions/331536/windows-threading-beginthread-vs-beginthreadex-vs-createthread-c
	// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex

	int thread_id;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, FOS_ThreadEntryFn, thread, 0/* CREATE_SUSPENDED */, &thread_id);
	thread->os_specific = handle;

#ifdef FOS_MODE_DEBUG
	wchar_t* debug_name_utf16 = StrToUTF16(debug_name, 1, g_temp_arena, NULL);
	SetThreadDescription(handle, debug_name_utf16);
#endif

	ArenaSetMark(g_temp_arena, T);
}

FOS_API void FOS_ThreadJoin(FOS_Thread* thread) {
	WaitForSingleObject((HANDLE)thread->os_specific, INFINITE);
	CloseHandle((HANDLE)thread->os_specific);
	*thread = (FOS_Thread){0}; // Do this so that we can safely start a new thread again using this same struct
}

FOS_API void FOS_MutexInit(FOS_Mutex* mutex) {
	StaticAssert(sizeof(FOS_Mutex) >= sizeof(CRITICAL_SECTION));
	InitializeCriticalSection((CRITICAL_SECTION*)mutex);
}

FOS_API void FOS_MutexDestroy(FOS_Mutex* mutex) {
	DeleteCriticalSection((CRITICAL_SECTION*)mutex);
	DebugFillGarbage(mutex, sizeof(*mutex));
}

FOS_API void FOS_MutexLock(FOS_Mutex* mutex) {
	EnterCriticalSection((CRITICAL_SECTION*)mutex);
}

FOS_API void FOS_MutexUnlock(FOS_Mutex* mutex) {
	LeaveCriticalSection((CRITICAL_SECTION*)mutex);
}

FOS_API void FOS_ConditionVarInit(FOS_ConditionVar* condition_var) {
	StaticAssert(sizeof(FOS_ConditionVar) >= sizeof(CONDITION_VARIABLE));
	InitializeConditionVariable((CONDITION_VARIABLE*)condition_var);
}

FOS_API void FOS_ConditionVarDestroy(FOS_ConditionVar* condition_var) {
	// No need to explicitly destroy
	// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializeconditionvariable
}

FOS_API void FOS_ConditionVarWait(FOS_ConditionVar* condition_var, FOS_Mutex* lock) {
	SleepConditionVariableCS((CONDITION_VARIABLE*)condition_var, (CRITICAL_SECTION*)lock, INFINITE);
}

FOS_API void FOS_ConditionVarSignal(FOS_ConditionVar* condition_var) {
	WakeConditionVariable((CONDITION_VARIABLE*)condition_var);
}

FOS_API void FOS_ConditionVarBroadcast(FOS_ConditionVar* condition_var) {
	WakeAllConditionVariable((CONDITION_VARIABLE*)condition_var);
}

#endif // FOS_WINDOWS
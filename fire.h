/*
                               __ _            _     
                              / _(_)          | |    
                             | |_ _ _ __ ___  | |__  
                             |  _| | '__/ _ \ | '_ \ 
                             | | | | | |  __/_| | | |   Author:       Eero Mutka
                             |_| |_|_|  \___(_)_| |_|   Version date: Jan 7. 2024

LICENSE: This code is released under the MIT-0 license, which you can find at https://opensource.org/license/mit-0/.
         Attribution is not required, but is appreciated :)

fire.h is my personal single-file programming utility library. It defines functions and data structures that come in
handy all the time, such as arrays, hash maps, strings and memory arenas. The code here is currently very experimental,
unfinished and I edit it frequently, so use at your own risk!

USAGE:
	Just #include this file into your code. Right now there's no option to #define F_IMPLEMENTATION, everything is just marked static. Also,
	I should make this code compile in C++ as well. TODO!
*/

#ifndef F_INCLUDED
#define F_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>  // memcpy, memmove, memset, memcmp, strlen
#include <math.h>    // isnan, isinf

typedef intptr_t Int; // Pointer-sized signed integer. The benefit over `int` is that it can be very big and it removes the need of using `size_t` for things like allocation sizes.
typedef uint8_t   U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t    I8;
typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;

// -- Declaration helper macros -----------------------------

/*
Opt() can be used for marking optionally-NULL pointers. In fire.h, the convention is that any pointer parameter that accepts NULL
is marked with this unless stated otherwise.
*/
#define Opt(ptr) ptr

#define THREAD_LOCAL __declspec(thread)

#ifndef F_FN
#ifdef __cplusplus
#define F_FN extern "C"
#else
#define F_FN static
#endif
#endif

// Using a macro to do string concatenation rather than doing it directly means that if a preprocessor macro
// is passed as an argument, it will be expanded before the stringification.
#define Concat_(x, y) x##y

// ----------------------------------------------------------

#define Slice(T) struct { const T* data; Int len; }
#define Array(T) struct { T* data; Int len; Int capacity; }

// Slice/Arrays for common types
typedef Slice(Int) IntSlice;
typedef Slice(U8) U8Slice; typedef Slice(U16) U16Slice; typedef Slice(U32) U32Slice; typedef Slice(U64) U64Slice;
typedef Slice(I8) I8Slice; typedef Slice(I16) I16Slice; typedef Slice(I32) I32Slice; typedef Slice(I64) I64Slice;
typedef Array(Int) IntArray;
typedef Array(U8) U8Array; typedef Array(U16) U16Array; typedef Array(U32) U32Array; typedef Array(U64) U64Array;
typedef Array(I8) I8Array; typedef Array(I16) I16Array; typedef Array(I32) I32Array; typedef Array(I64) I64Array;

typedef uint32_t Rune;

typedef Slice(void) SliceRaw;
typedef Array(void) ArrayRaw;

// -------------------------------------------------------------------

typedef struct DefaultArenaBlockHeader {
	Int size_including_header;
	Opt(struct DefaultArenaBlockHeader*) next;
#ifdef FIRE_H_MODE_DEBUG
	U32 metadata_1;
	U32 metadata_2;
#endif
} DefaultArenaBlockHeader;

typedef struct DefaultArenaMark {
	DefaultArenaBlockHeader* block;
	U8* ptr;
} DefaultArenaMark;

typedef struct DefaultArena {
	Int block_size;
	Opt(DefaultArenaBlockHeader*) first_block;
	DefaultArenaMark mark;
} DefaultArena;

// -------------------------------------------------------------------

#ifndef FIRE_H_CUSTOM_ARENA
typedef DefaultArena Arena;
typedef DefaultArenaMark ArenaMark;
#endif

// -------------------------------------------------------

// WARNING!!! Make sure you don't have any padding in your key-type, otherwise you'll run into nasty UB.

// Hash maps. What's the best strategy? I think Jai's strategy is the best. For a growable hash map, it's best to store the hashes alongside the main lookup. This way resizing
// is very cheap. ... Or maybe we shoulnd't, because in our new approach, we would be compute-bound rather than memory bound! ...But we still need two bits to tell the slot state.
// So, an optimized special case map wouldn't maybe store the 32-bit hash and instead use sentinel values, but let's just use a 32-bit hash ourselves. For instance, a pointer-map could do that.

// NOTE: the value/key pointers aren't stable in a Map2.
// Map is a robin-hood hash map.

// Fun macro tricks! We should do the same thing for Array to get the best alignment.
#define TypeInfoDummy(T) struct{char a; T b;}
#define TypeInfoSize(ptr) sizeof((ptr)->a)
#define TypeInfoAlign(ptr) ((Int)&(ptr)->b - (Int)(ptr))

#define Map2(K, V) struct { \
	struct{ U32 hash; K key; V value; }* data; \
	Int len; /* Number of currently alive elements */ \
	I32 capacity; /* Number of reserved slots */ \
	/*I16 max_probe_seq_length; */}

typedef Map2(char, char) MapRaw;

// TODO: for set, we should not use char map, because it uses 1 byte
#define Set2(T) Map2(T, char)
typedef Set2(char) fSetRaw;

#ifndef SlotArray
#define SlotArray(T) SlotArrayRaw
#endif

#ifndef String
typedef U8Slice String;
#endif
typedef Slice(String) StringSlice;

typedef struct {
	Arena* arena;
	union {
		String str;
		Array(U8) buffer;
	};
} StringBuilder;

// -----------------------------------------

typedef struct{Int min, max;} StrRange;
typedef Slice(StrRange) StrRangeSlice;

I64 FloorToI64(float x);
#define f_round_to_i64(x) FloorToI64(x + 0.5f)

// -- Internal implementations for macro utilities ----------------

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define FOS_WINDOWS
#else
#error "Sorry, only windows is supported for now!"
#endif

// Detect which compiler we're compiling on
// https://stackoverflow.com/questions/28166565/detect-gcc-as-opposed-to-msvc-clang-with-macro
#if defined(__clang__)
#define COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_GCC
#elif defined(_MSC_VER)
#define COMPILER_MSVC
#elif defined(__TINYC__)
#define COMPILER_TCC
#endif

// https://www.wambold.com/Martin/writings/alignof.html
#ifdef __cplusplus
template<typename T> struct alignment_trick { char c; T member; };
#define AlignOf_(type) OffsetOf(alignment_trick<type>, member)
#define StrInit_(x)   L(x)
#define StructLit_(T) T
#else
#define AlignOf_(type) (OffsetOf(struct Concat(_dummy, __COUNTER__) { char c; type member; }, member))
#define StrInit_(x)   {(const U8*)x, sizeof(x)-1}
#define StructLit_(T) (T)
#endif

#ifdef FIRE_H_MODE_DEBUG
#define ArrayBoundsCheck(array_or_slice, i) AssertFn((size_t)(i) < (size_t)(array_or_slice).len)
#else
#define ArrayBoundsCheck(array_or_slice, i) (void)0
#endif

// Breakpoint macro - this can be useful when debugging.
#ifdef FIRE_H_MODE_DEBUG
#if defined(COMPILER_MSVC)
#define BP() __debugbreak()
#elif defined(COMPILER_TCC)
#define BP() asm("int3")
#else
#error TODO: breakpoint instruction
#endif
#else // When not in FIRE_H_MODE_DEBUG, define BP() to do nothing.
#define BP()
#endif

#define CB_(name, break_on_idx) \
	static U64 name##_counter = 0; \
	name##_counter ++; \
	U64 name = name##_counter; \
	if (name == (break_on_idx)) { BP(); }

inline bool MapIter(MapRaw* map, Int* i, void** out_key, void** out_value, Int key_offset, Int val_offset, Int elem_size) {
	U32* hash;
	for (;;) {
		if (*i >= map->capacity) return false;

		hash = (U32*)((U8*)map->data + (*i) * elem_size);
		if (*hash == 0 || (*hash >> 31) != 0) { // A hash value of 0 means empty, while the most significant bit says if it's a tombstone
			*i = *i + 1;
			continue;
		}
		break;
	}

	*out_key = (U8*)hash + key_offset;
	*out_value = (U8*)hash + val_offset;
	*i = *i + 1;
	return true;
}

#define DListRemove_(list, elem, next_name) { \
	if (elem->next_name[0]) elem->next_name[0]->next_name[1] = elem->next_name[1]; \
	else list[0] = elem->next_name[1]; \
	if (elem->next_name[1]) elem->next_name[1]->next_name[0] = elem->next_name[0]; \
	else list[1] = elem->next_name[0]; \
	}

#define DListPushBack_(list, elem, next_name) { \
	elem->next_name[0] = list[1]; \
	elem->next_name[1] = NULL; \
	if (list[1]) list[1]->next_name[1] = elem; \
	else list[0] = elem; \
	list[1] = elem; \
	}

inline void* ListPopFront_(void** p_list, void* next) {
	void* first = *p_list;
	*p_list = next;
	return first;
}

#ifdef FIRE_H_MODE_DEBUG
#define DebugFillGarbage_(ptr, len) memset(ptr, 0xCC, len);
#else
#define DebugFillGarbage_(ptr, len)
#endif

// == Overridable functions =======================================================

void* FIRE_H_IMPL_Realloc(void* old_ptr, Int new_size, Int new_alignment);

#ifndef FIRE_H_CUSTOM_IMPL
#define FIRE_H_CUSTOM_IMPL
#include <stdlib.h>
void* FIRE_H_IMPL_Realloc(void* old_ptr, Int new_size, Int new_alignment) { return _aligned_realloc(old_ptr, new_size, new_alignment); }
#endif

// ================================== fire.h API ==================================

//F_FN void Init();
//F_FN void Deinit();

/* -- Exported preprocessor definitions ----------------------------------------

 To get information about the environment (target OS, compiler), you can look at the
 preprocessor definitions created by this file.
 
 Possible values:
  Target OS:  FOS_WINDOWS
  Compiler:   COMPILER_CLANG, COMPILER_GCC, COMPILER_MSVC, COMPILER_TCC
  
 For example, if you want some piece of code to only be compiled on windows, you can do:
    
	#if defined(FOS_WINDOWS)
	// Do something windows specific
	#else
	...
	#endif

*/

// -- Helper macros ------------------------------------------------------------

#define Concat(x, y) Concat_(x, y)

#define AlignOf(type) AlignOf_(type)

#define Pad(x) char Concat(_pad, __LINE__)[x]

/*
 C/C++ agnostic struct literal.
 Example:
    return StructLit(Vector3){10, 5, 3};
*/
#define StructLit(T)  StructLit_(T)

// String literals
#define L(x)        StructLit(String){(const U8*)x, sizeof(x)-1} // String literal
#define StrInit(x)  StrInit_(x)  // String literal that works in C initializer lists

#define ThisFilePath()   CStrToStr(__FILE__)

/*
 Len() gives the length of a fixed length array.
 Example:
    int values[3] = {2, 3, 4}
	Assert(Len(values) == 3);
*/
#define Len(x)  (sizeof(x) / sizeof(x[0]))

/*
 OffsetOf gives the offset of a struct member.
 Example:
    struct Foo { int a; int b; };
	Assert(OffsetOf(struct Foo, b) == 4);
*/
#define OffsetOf(T, f) ((Int)&((T*)0)->f)

#define I8_MIN   (-127(I8) - 1)
#define I16_MIN  (-(I16)32767 - 1)
#define I32_MIN  (-(I32)2147483647 - 1)
#define I64_MIN  (-(I64)9223372036854775807 - 1)
#define I8_MAX   (I8)127
#define I16_MAX  (I16)32767
#define I32_MAX  (I32)2147483647
#define I64_MAX  (I64)9223372036854775807
#define U8_MAX   (U8)0xFF
#define U16_MAX  (U16)0xFFFF
#define U32_MAX  (U32)0xFFFFFFFF
#define U64_MAX  (U16)0xFFFFFFFFFFFFFFFF

#define StaticAssert(x) enum { Concat(_static_assert_, __LINE__) = 1 / ((int)!!(x)) }

#define Unused(x) (void)(x)  // Can be used to suppress compiler warnings with unused local variables.

// #ifndef IsDebuggerPresent
// // Is it possible to somehow check if a function WILL be implemented? I don't think so :(
// #define IsDebuggerPresent() false /* if no implementation for FOS_IsDebuggerPresent is provided, then assume we aren't running in a debugger. */
// #endif
// 
// // Breakpoint that only hits if inside the debugger
// #define BP() do { if (IsDebuggerPresent()) { BP(); } } while(0)

/*
 Counted breakpoint;
 Debugging helper that counts how many times it gets hit and allows for breaking at an index.
 The index starts at 1, meaning break_on_idx=0 will never break.
*/
#define CB(name, break_on_idx)  CB_(name, break_on_idx)

/*
 NOTE: Because it's useful to say `F_Assert(do_some_computation_that_modifies_some_state(...))`,
 `F_Assert` shouldn't be cut from release builds. Some other codebases may define assert to expand into
 nothingness on release builds, but that would limit the expressiveness of assert, as the above example wouldn't be allowed.
*/
#ifndef Assert
#define Assert(x) { if (!(x)) { BP(); } }
//inline void 
//
#endif

#define TODO() Assert(0)
inline void AssertFn(bool x) { Assert(x); } // This wrapper is required if we want to use Assert inside a comma expression

// Cast with range checking
// #define Cast(T, x) ((T)(x) == (x) ? (T)(x) : (Crash(), (T)0))
#define Cast(T, x) (T)x

#define KIB(x) ((Int)(x) << 10)
#define MIB(x) ((Int)(x) << 20)
#define GIB(x) ((Int)(x) << 30)
#define TIB(x) ((U64)(x) << 40)

#define Abs(x) ((x) < 0 ? -(x) : x)
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Clamp(x, minimum, maximum) ((x) < (minimum) ? (minimum) : (x) > (maximum) ? (maximum) : (x))
#define Swap(T, a, b) do { T temp = (a); (a) = (b); (b) = temp; } while (0)

// e.g. 0b0010101000
// =>   0b0010100000
#define FlipRightmostOneBit(x) ((x) & ((x) - 1))

// e.g. 0b0010101111
// =>   0b0010111111
#define FlipRighmostZeroBit(x) ((x) | ((x) + 1))

// e.g. 0b0010101000
// =>   0b0010101111
#define FlipRightmostZeroes(x) (((x) - 1) | (x))

// When x is a power of 2, it must only contain a single 1-bit.
// `x == 0` will output 1.
#define IsPowerOf2(x) (FlipRightmostOneBit(x) == 0)

// `p` must be a power of 2.
// `x` is allowed to be negative as well.
#define AlignUpPow2(x, p) (((x) + (p) - 1) & ~((p) - 1)) // e.g. (x=30, p=16) -> 32
#define AlignDownPow2(x, p) ((x) & ~((p) - 1)) // e.g. (x=30, p=16) -> 16

#define HasPow2Alignment(x, p) ((x) % (p) == 0) // p must be a power of 2

#define AsBytes(x) StructLit(String){(const U8*)&x, sizeof(x)}
#define SliceAsBytes(x) StructLit(String){(const U8*)(x).data, (x).len * sizeof((x).data[0])}

// Results in a compile-error if `x` is not of type `T*`
#define TypecheckPtr(T, x) (void)((T*)0 == x)

#define ZeroInit(x) memset(x, 0, sizeof(*x))

/*
Remove a node from doubly-linked list, e.g.
    struct Node { Node* next[2]; };
    Node* list[2] = ...;
    Node* node = ...;
    DListRemove(node, list, next);
*/
#define DListRemove(list, elem, next_name)   DListRemove_(list, elem, next_name)

/*
Push a node to the end of doubly-linked list, e.g.
    struct Node { Node* next[2]; };
    Node* list[2] = ...;
    Node* node = ...;
    DListPushBack(node, list, next);
*/
#define DListPushBack(list, elem, next_name)  DListPushBack_(list, elem, next_name)

/*
Remove the first node of a singly linked list, e.g.
	struct Node { Node* next; };
    Node* list = ...;
    Node* first = ListPopFront(&list, next);
*/
#define ListPopFront(list, next_name)  ListPopFront_(list, (*list)->next_name)

#define ListPushFront(list, elem, next_name) {(elem)->next_name = *(list); *(list) = (elem);}

#define Clone(T, arena, value)   ((T*)0 == &(value), (T*)CloneSize(arena, &(value), sizeof(T)))

// TODO: add proper alignment to New
#define New(T, arena, ...) (T*)CloneSize(arena, &(T){__VA_ARGS__}, sizeof(T))

// TODO: ArenaPushN
#define ArenaPushNUndef(T, ARENA, N)  ArenaPushUndef(ARENA, sizeof(T) * (N), AlignOf(T))

#define DebugFillGarbage(ptr, len)  DebugFillGarbage_(ptr, len)

//#define Addressable(T, x) (struct Concat(Fire_, __LINE__){T dummy;}){x}.dummy
#define Bitcast(T, x) (*(T*)&(x))

// Array to slice
#define ArrTo(T, x) \
	((void)((x).data == (T){0}.data), /* Typecheck T */ \
	(*(T*)&(x)))

// -- Map operations -----------------------

#define MapTypecheckK(MAP, PTR) (PTR == &(MAP)->data->key)
#define MapTypecheckV(MAP, PTR) (PTR == &(MAP)->data->value)

#define MapKeySize(MAP) sizeof((MAP)->data->key)
#define MapValSize(MAP) sizeof((MAP)->data->value)
#define MapElemSize(MAP) sizeof(*(MAP)->data)
#define MapKeyOffset(MAP) ((Int)&(MAP)->data->key - (Int)(MAP)->data)
#define MapValOffset(MAP) ((Int)&(MAP)->data->value - (Int)(MAP)->data)

//#define MapReserve(K, V, arena, map, capacity) { \
//	/* Make sure that there is no compiler-generated padding inside the key struct */ \
//	const static K _dummy_zero; K _dummy; memset(&_dummy, 0xFF, sizeof(K)); _dummy = (K){0}; Assert(memcmp(&_dummy_zero, &_dummy, sizeof(K)) == 0); \
//	MapReserveRaw(arena, (fMapRaw*)map, sizeof(K), sizeof(V), AlignOf(K), AlignOf(V), capacity); }

// * If the element is not found, then OUT_VALUE will not be written to.
#define MapFind(MAP, KEY, OUT_VALUE) \
	(MapTypecheckK(MAP, KEY), \
	MapFindRaw((MapRaw*)MAP, KEY, 0, OUT_VALUE, MapKeySize(MAP), MapValSize(MAP), MapElemSize(MAP), MapKeyOffset(MAP), MapValOffset(MAP)))

#define MapFindPtr(MAP, KEY) \
	(MapTypecheckK(MAP, KEY), \
	MapFindPtrRaw((MapRaw*)MAP, KEY, 0, MapKeySize(MAP), MapValSize(MAP), MapElemSize(MAP), MapKeyOffset(MAP), MapValOffset(MAP)))

#define MapAssign(ARENA, MAP, KEY, VALUE) \
	(MapTypecheckK(MAP, KEY) && MapTypecheckV(MAP, VALUE), \
	MapAssignRaw(ARENA, (MapRaw*)(MAP), KEY, 0, VALUE, MapKeySize(MAP), MapValSize(MAP), MapElemSize(MAP), MapKeyOffset(MAP), MapValOffset(MAP)))

#define MapAdd(ARENA, MAP, KEY, OUT_VALUE) \
	(MapTypecheckK(MAP, KEY) && MapTypecheckV(MAP, *(OUT_VALUE)), \
	MapAddRaw(ARENA, (MapRaw*)(MAP), KEY, 0, (void**)OUT_VALUE, MapKeySize(MAP), MapValSize(MAP), MapElemSize(MAP), MapKeyOffset(MAP), MapValOffset(MAP)))

#define MapEach(K, V, MAP, IT) \
	struct Concat(Fire_, __LINE__) { Int i_next; K* key; V* value; }; \
	if (MAP.len > 0) for (struct Concat(Fire_, __LINE__) IT = {0}; \
		MapIter((MapRaw*)&(MAP), &IT.i_next, (void**)&IT.key, (void**)&IT.value, MapKeyOffset(&(MAP)), MapValOffset(&(MAP)), MapElemSize(&(MAP))); )

// * Returns `true` if the element did not exist in the map before.
// * If you pass 0 into `hash`, the hash will be computed from the key.
F_FN bool MapAddRaw(Arena* arena, MapRaw* map, const void* key, U32 hash, Opt(void**) out_val_ptr, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset);

// * Returns `true` if the element did not exist in the map before
// * If you pass 0 into `hash`, the hash will be computed from the key.
F_FN bool MapAssignRaw(Arena* arena, MapRaw* map, const void* key, U32 hash, Opt(void*) val, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset);

// * If the element is not found, then `out_val` will not be written to.
F_FN bool MapFindRaw(MapRaw* map, const void* key, U32 hash, Opt(void*) out_val, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset);

F_FN Opt(void*) MapFindPtrRaw(MapRaw* map, const void* key, U32 hash, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset);

// -- Set operations -----------------------

#define SetReserve(T, arena, set, slot_count) MapReserve(T, void, arena, set, slot_count)  //MapReserveRaw(arena, (fMapRaw*)set, sizeof(*(map)->T_dummy), slot_count)

// returns `true` if the key did not exist before
#define SetAdd(arena, set, elem)  MapAssign(arena, set, elem, NULL) //          f_map_set_raw(arena, (fMapRaw*)set, sizeof(*(set)->T_dummy), 0, &(elem), NULL, NULL, NULL)

// -- Default Arena ------------------------

F_FN DefaultArena* MakeDefaultArena(Int block_size);
F_FN void DestroyDefaultArena(DefaultArena* arena);

F_FN void* DefaultArenaPushUndef(Arena* arena, Int size, Int alignment);
F_FN ArenaMark DefaultArenaGetMark(Arena* arena);
F_FN void DefaultArenaSetMark(Arena* arena, ArenaMark mark);
F_FN void DefaultArenaReset(Arena* arena);

// -- Arena --------------------------------

#ifndef FIRE_CUSTOM_ARENA
inline Arena* MakeArena(Int block_size)                            { return MakeDefaultArena(block_size); }
inline void DestroyArena(Arena* arena)                             { DestroyDefaultArena(arena); }

inline void* ArenaPushUndef(Arena* arena, Int size, Int alignment) { return DefaultArenaPushUndef(arena, size, alignment); }
inline ArenaMark ArenaGetMark(Arena* arena)                        { return DefaultArenaGetMark(arena); }
inline void ArenaSetMark(Arena* arena, ArenaMark mark)             { DefaultArenaSetMark(arena, mark); }
inline void ArenaReset(Arena* arena)                               { DefaultArenaReset(arena); }
#endif

inline Int AutoAlign(Int size) {
	const static Int small_aligns[] = {1, 1, 2, 4, 4, 8, 8, 8, 8};
	return size > 8 ? 16 : small_aligns[size];
}

inline void* ArenaPushZero(Arena* arena, Int size, Int align) { void* p = ArenaPushUndef(arena, size, align); memset(p, 0, size); return p; }

#define MAX_ALIGNMENT 16

#define MemAllocEx(size, alignment) FIRE_H_IMPL_Realloc(NULL, size, alignment)
#define MemAlloc(size)              FIRE_H_IMPL_Realloc(NULL, size, MAX_ALIGNMENT)
#define MemFree(ptr)                FIRE_H_IMPL_Realloc(ptr, 0, 1)

inline void* CloneSize(Arena* arena, const void* value, Int size) { void* p = ArenaPushUndef(arena, size, AutoAlign(size)); return memcpy(p, value, size); }
inline void* CloneSizeEx(Arena* arena, const void* value, Int size, Int align) { void* p = ArenaPushUndef(arena, size, align); return memcpy(p, value, size); }

// For profiling, these may be removed from non-profiled builds.
// It makes sense to have these as a common interface, as they can be used with any profiler.
// ... but if we have multiple translation units, how can we define these? I guess we have to do custom translation units then.
#ifndef OVERWRITE_PROFILER_MACROS
// Function-level profiler scope. A single function may only have one of these, and it should span the entire function.
#define ProfEnter()
#define ProfExit()

// Nested profiler scope. NAME should NOT be in quotes.
#define ProfEnterN(NAME)
#define ProfExitN(NAME)
#endif

// NOTE: does not work with negative numbers!
F_FN Int RoundUpToPow2(Int v); // todo: move this into the macros section as an inline function?

// -- Array & Slice --------------------------------

//#define f_arr2slice(T, array) ()

// Results in a compile-error if `elem` does not match the array's elemen type
#define ArrayPtrTypecheck(array, elem) (void)((array)->data == &(elem))

#define ArrayGet(array_or_slice, i)        (ArrayBoundsCheck(array_or_slice, i), ((array_or_slice).data)[i])
#define ArrayGetPtr(array_or_slice, i)     (ArrayBoundsCheck(array_or_slice, i), &((array_or_slice).data)[i])
#define ArraySet(array_or_slice, i, value) (ArrayBoundsCheck(array_or_slice, i), ((array_or_slice).data)[i] = value)
#define ArrayElemSize(array_or_slice)      sizeof(*(array_or_slice).data)
#define ArrayPeek(array)                   (ArrayBoundsCheck(array, (array).len - 1), (array).data[(array).len - 1])

// NOTE: this doesn't reserve any extra capacity at the end... maybe I should change that.
#define ArrayClone(T, arena, array_or_slice) StructLit(T){CloneSize(arena, array_or_slice.data, array_or_slice.len * ArrayElemSize(array_or_slice)), array_or_slice.len, array_or_slice.len}
#define SliceClone(T, arena, array_or_slice) StructLit(T){CloneSize(arena, array_or_slice.data, array_or_slice.len * ArrayElemSize(array_or_slice)), array_or_slice.len}

// TODO: I think the macros where we require an address, i.e. ArrayPush(), we should make the user give the address rather than do it implicitly.
// That would make code where we pass things like (U32){0} more self explanatory.
#define ArrayReserve(arena, array, capacity)  ArrayReserveRaw(arena, (ArrayRaw*)(array), capacity, ArrayElemSize(*array))
#define ArrayPush(arena, array, elem)         (ArrayPtrTypecheck(array, elem), ArrayPushRaw(arena, (ArrayRaw*)(array), &(elem), ArrayElemSize(*array)))
#define ArrayPushN(arena, array, elems)       (ArrayPtrTypecheck(array, *elems.data), ArrayPushNRaw(arena, (ArrayRaw*)(array), elems.data, elems.len, ArrayElemSize(*array)))
#define ArrayInsert(arena, array, at, elem)   (ArrayPtrTypecheck(array, elem), ArrayInsertRaw(arena, (ArrayRaw*)(array), at, &(elem), ArrayElemSize(*array)))
#define ArrayInsertN(arena, array, at, elems) (ArrayPtrTypecheck(array, *elems.data), ArrayInsertNRaw(arena, (ArrayRaw*)(array), at, elems.data, elems.len, ArrayElemSize(*array)))
#define ArrayRemove(array, i)                 ArrayRemoveRaw((ArrayRaw*)(array), i, ArrayElemSize(*array))
#define ArrayRemoveN(array, i, n)             ArrayRemoveNRaw((ArrayRaw*)(array), i, n, ArrayElemSize(*array))
#define ArrayPop(array)                       (array)->data[--(array)->len]
#define ArrayResize(arena, array, elem, len)  (ArrayPtrTypecheck(array, elem), ArrayResizeRaw(arena, (ArrayRaw*)(array), len, &(elem), ArrayElemSize(*array)))
#define ArrayResizeUndef(arena, array, len)   ArrayResizeRaw(arena, (ArrayRaw*)(array), len, NULL, ArrayElemSize(*array))
#define ArrayClear(array)                     (array)->len = 0

/*
Magical array iteration macro - works both on Array and Slice types.
e.g.
Array(float) foo;
ArrayEach(float, foo, it) {
printf("foo at index %d has the value of %f\n", it.i, it.elem);
}
TODO: we should check the generated assembly and compare it to a hand-written loop. @speed
*/
#define ArrayEach(T, array, it_name) \
	ArrayPtrTypecheck(&(array), (T){0}); \
	struct Concat(FIRE_H_, __LINE__) {Int i; T* elem;}; \
	for (struct Concat(FIRE_H_, __LINE__) it_name = {0}; ArrayIter(&(array), &it_name.i, (void**)&it_name.elem, sizeof(T)); it_name.i++)

// -- Array initialization macros --

// Array/slice from varargs
#define Arr(T, ...) {(T[]){__VA_ARGS__}, sizeof((T[]){__VA_ARGS__}) / sizeof(T)}

// Array/slice from c-style array
#define ArrC(T, x) StructLit(T){x, sizeof(x) / sizeof(x[0])}

F_FN Int ArrayPushRaw(Arena* arena, ArrayRaw* array, const void* elem, Int elem_size);
F_FN Int ArrayPushNRaw(Arena* arena, ArrayRaw* array, const void* elems, Int n, Int elem_size);
F_FN void ArrayInsertRaw(Arena* arena, ArrayRaw* array, Int at, const void* elem, Int elem_size);
F_FN void ArrayInsertNRaw(Arena* arena, ArrayRaw* array, Int at, const void* elems, Int n, Int elem_size);
F_FN void ArrayRemoveRaw(ArrayRaw* array, Int i, Int elem_size);
F_FN void ArrayRemoveNRaw(ArrayRaw* array, Int i, Int n, Int elem_size);
F_FN void ArrayPopRaw(ArrayRaw* array, Opt(void*) out_elem, Int elem_size);
F_FN void ArrayReserveRaw(Arena* arena, ArrayRaw* array, Int capacity, Int elem_size);

// * This will reserve extra space at the end / round up to power-of-2
F_FN void ArrayResizeRaw(Arena* arena, ArrayRaw* array, Int len, Opt(const void*) value, Int elem_size); // set value to NULL to not initialize the memory

inline bool ArrayIter(const void* array, Int* i, void** elem, Int elem_size) {
	if ((size_t)*i < (size_t)((ArrayRaw*)array)->len) {
		*elem = (U8*)((ArrayRaw*)array)->data + (*i) * elem_size;
		return true;
	}
	return false;
}

inline ArrayRaw ArrayMakeCapRaw(Arena* arena, Int capacity, Int elem_size) {
	ArrayRaw result = {0};
	ArrayReserveRaw(arena, &result, capacity, elem_size);
	return result;
}

inline ArrayRaw ArrayMakeLenRaw(Arena* arena, Int len, const void* initial_value, Int elem_size, Int elem_align) {
	ArrayRaw result = {ArenaPushUndef(arena, len * elem_size, elem_align), len, len};
	DebugFillGarbage(result.data, len * elem_size);

	for (Int i=0; i<len; i++) {
		memcpy((U8*)result.data + elem_size*i, initial_value, elem_size);
	}
	return result;
}

inline ArrayRaw ArrayMakeLenUndefRaw(Arena* arena, Int len, Int elem_size, Int elem_align) {
	ArrayRaw result = {ArenaPushUndef(arena, len * elem_size, elem_align), len, len};
	DebugFillGarbage(result.data, len * elem_size);
	return result;
}

// -- Hash --------------------------------------------------------------------

inline U64 Mix64(U64 x) {
	// https://github.com/jonmaiga/mx3/blob/master/mx3.h
	U64 C = 0xbea225f9eb34556d;
	x ^= x >> 32;
	x *= C;
	x ^= x >> 29;
	x *= C;
	x ^= x >> 32;
	x *= C;
	x ^= x >> 29;
	return x;
}

inline U64 Mix64_Stream(U64 h, U64 x) {
	// https://github.com/jonmaiga/mx3/blob/master/mx3.h
	U64 C = 0xbea225f9eb34556d;
	x *= C;
	x ^= x >> 39;
	h += x * C;
	h *= C;
	return h;
}

F_FN U64 Hash64Ex(String s, U64 seed);
#define Hash64(s) Hash64Ex(s, 0)

typedef struct Hasher64 { U64 h; } Hasher64;
inline Hasher64 HasherBegin(U64 seed) { Hasher64 h = {seed}; return h; }
inline void HasherMix(Hasher64* h, U64 data) { h->h = Mix64_Stream(h->h, data); }
inline U64 HasherEnd(Hasher64* h) { return Mix64(h->h); }

// -- String ------------------------------------------------------------------

#define StrIsUtf8FirstByte(c) (((c) & 0xC0) != 0x80) /* is c the start of a utf8 sequence? */
#define StrEach(str, r, i) (Int i=0, r = 0, i##_next=0; (r=StrNextRune(str, &i##_next)); i=i##_next)
#define StrEachReverse(str, r, i) (Int i=str.len, r = 0; r=StrPrevRune(str, &i);)

inline String StrFromData(const U8* data, Int len) { return StructLit(String){(U8*)data, len}; }

// Formatting rules:
// ~s                       - String
// ~c                       - c-style string
// ~~                       - escape for ~
// TODO: ~r                 - rune
// ~u8, ~u16, ~u32, ~u64    - unsigned integers
// ~i8, ~i16, ~i32, ~i64    - signed integers
// ~x8, ~x16, ~x32, ~x64    - hexadecimal integers
// ~f                       - double
// TODO: we could allow for custom user-provided formatters?
/*
-- New formatting rules proposal --
\as  -  String
\ac  -  c-string
\ai  -  Int
\af  -  float/double
\ar  -  rune (UTF8 codepoint)
\aU8  \aU16  \aU32  \aU64   - sized unsigned integer
\aI8  \aI16  \aI32  \aI64   - sized signed integer
\aX8  \aX16  \aX32  \aX64   - sized hexadecimal value
\aB8  \aB16  \aB32  \aB64   - sized binary value
*/
F_FN void Print(StringBuilder* s, const char* fmt, ...);
F_FN void PrintVA(StringBuilder* s, const char* fmt, va_list args);

F_FN void PrintS(StringBuilder* s, String str); // write string
F_FN void PrintSRepeat(StringBuilder* s, String str, Int count);
F_FN void PrintC(StringBuilder* s, const char* str);
F_FN void PrintB(StringBuilder* s, uint8_t b); // print ASCII-byte. hmm.. maybe we should just have writer

#ifndef DebugPrint
#define DebugPrint(fmt, ...)
#endif

F_FN String APrint(Arena* arena, const char* fmt, ...); // arena-print

F_FN String StrAdvance(String* str, Int len);
F_FN String StrClone(Arena* arena, String str);

// TODO: move these to OS
F_FN String PathStem(String path); // Returns the name of a file without extension, e.g. "matty/boo/billy.txt" => "billy"
F_FN String PathExtension(String path); // returns the file extension, e.g. "matty/boo/billy.txt" => "txt"
F_FN String PathTail(String path); // Returns the last part of a path, e.g. "matty/boo/billy.txt" => "billy.txt"
F_FN String PathDir(String path); // returns the directory, e.g. "matty/boo/billy.txt" => "matty/boo". If there are no more slashes in the string, returns "."

F_FN bool StrLastIdxOfAnyChar(String str, String chars, Int* out_index);
F_FN bool StrFind(String str, String substr, Opt(StrRange*) out_range);
#define StrContains(str, substr) StrFind(str, substr, NULL)

F_FN String StrReplace(Arena* arena, String str, String search_for, String replace_with);
F_FN String StrReplaceMulti(Arena* arena, String str, StringSlice search_for, StringSlice replace_with);
F_FN String StrToLower(Arena* arena, String str);
F_FN Rune ToLower(Rune r);

F_FN bool StrEndsWith(String str, String end);
F_FN bool StrStartsWith(String str, String start);
F_FN bool StrCutEnd(String* str, String end);
F_FN bool StrCutStart(String* str, String start);

F_FN StrRangeSlice StrSplit(Arena* arena, String str, U8 character);

F_FN String StrJoinN(StringSlice args, Arena* arena);
#define StrJoin(arena, ...) StrJoinN((StringSlice)Arr(String, __VA_ARGS__), arena)

F_FN bool StrEquals(String a, String b);
F_FN bool StrEqualsCaseInsensitive(String a, String b);

F_FN String StrSlice(String str, Int lo, Int hi);
F_FN String StrSliceBefore(String str, Int mid);
F_FN String StrSliceAfter(String str, Int mid);

// * Underscores are allowed and skipped
// * Works with any base up to 16 (i.e. binary, base-10, hex)
F_FN bool StrToU64Ex(String s, Int base, U64* out_value);
#define StrToU64(s, out_value) StrToU64Ex(s, 10, out_value)

// * A single minus or plus is accepted preceding the digits
// * Works with any base up to 16 (i.e. binary, base-10, hex)
// * Underscores are allowed and skipped
F_FN bool StrToI64Ex(String s, Int base, I64* out_value);
#define StrToI64(s, out_value) StrToI64Ex(s, 10, out_value)

F_FN bool StrToFloat(String s, double* out);

F_FN String IntToStr(Arena* arena, Int value);
F_FN String IntToStrEx(Arena* arena, U64 data, bool is_signed, Int radix);
F_FN String FloatToStr(Arena* arena, double value);

F_FN Int IntToStr_(U8* buffer, U64 data, bool is_signed, Int radix); // NOT null-terminated, the length of the string is returned.
F_FN Int FloatToStr_(U8* buffer, double value); // NOT null-terminated, the length of the string is returned.

F_FN char* StrToCStr(String s, Arena* arena);
F_FN String CStrToStr(const char* s);

F_FN Int StrEncodeRune(U8* output, Rune r); // returns the number of bytes written

// Returns the character on `byteoffset`, then increments it.
// Will returns 0 if byteoffset >= str.len
F_FN Rune StrNextRune(String str, Int* byteoffset);
F_FN Rune StrDecodeRune(String str);

// Decrements `bytecounter`, then returns the character on it.
// Will returns 0 if byteoffset < 0
F_FN Rune StrPrevRune(String str, Int* byteoffset);

F_FN Int StrRuneCount(String str);

/// IMPLEMENTATION /////////////////////////////////////////////////////////////////

#define PATH_SEPARATOR_CHARS L("/\\")

typedef struct ASCIISet {
	U8 bytes[32];
} ASCIISet;

static ASCIISet ASCIISetMake(String chars) {
	ProfEnter();
	ASCIISet set = {0};
	for (Int i = 0; i < chars.len; i++) {
		U8 c = chars.data[i];
		set.bytes[c / 8] |= 1 << (c % 8);
	}
	ProfExit();
	return set;
}

static bool ASCIISetContains(ASCIISet set, U8 c) {
	return (set.bytes[c / 8] & 1 << (c % 8)) != 0;
}

F_FN bool StrLastIdxOfAnyChar(String str, String chars, Int* out_index) {
	ProfEnter();
	ASCIISet char_set = ASCIISetMake(chars);

	bool found = false;
	for (Int i = str.len - 1; i >= 0; i--) {
		if (ASCIISetContains(char_set, str.data[i])) {
			*out_index = i;
			found = true;
			break;
		}
	}

	ProfExit();
	return found;
}

F_FN bool StrEquals(String a, String b) {
	return a.len == b.len && memcmp(a.data, b.data, a.len) == 0;
}

F_FN bool StrEqualsCaseInsensitive(String a, String b) {
	ProfEnter();
	bool equals = false;
	if (a.len == b.len) {
		for (Int i = 0; i < a.len; i++) {
			if (ToLower(a.data[i]) != ToLower(b.data[i])) {
				equals = false;
				break;
			}
		}
	}
	ProfExit();
	return equals;
}

F_FN String StrSlice(String str, Int lo, Int hi) {
	Assert(hi >= lo && hi <= str.len);
	return (String){str.data + lo, hi - lo};
}

F_FN String StrSliceAfter(String str, Int mid) {
	Assert(mid <= str.len);
	return (String){str.data + mid, str.len - mid};
}

F_FN String StrSliceBefore(String str, Int mid) {
	Assert(mid <= str.len);
	return (String){str.data, mid};
}

F_FN bool StrFind(String str, String substr, Opt(StrRange*) out_range) {
	ProfEnter();
	Int i_last = str.len - substr.len;
	bool found = false;
	for (Int i = 0; i <= i_last; i++) {
		if (StrEquals(StrSlice(str, i, i + substr.len), substr)) {
			*out_range = (StrRange){i, i + substr.len};
			found = true;
			break;
		}
	}
	ProfExit();
	return found;
};

F_FN String PathExtension(String path) {
	ProfEnter();
	Int idx;
	String result = {0};
	if (StrLastIdxOfAnyChar(path, L("."), &idx)) {
		result = StrSliceAfter(path, idx + 1);
	}
	ProfExit();
	return result;
}

F_FN String PathDir(String path) {
	ProfEnter();
	Int last_sep;
	String result = L(".");
	if (StrLastIdxOfAnyChar(path, PATH_SEPARATOR_CHARS, &last_sep)) {
		result = StrSliceBefore(path, last_sep);
	}
	ProfExit();
	return result;
}

F_FN void PrintS(StringBuilder* s, String str) {
	ArrayPushN(s->arena, &s->buffer, str);
}

F_FN void PrintB(StringBuilder* s, uint8_t b) {
	ArrayPush(s->arena, &s->buffer, b);
}

F_FN void PrintSRepeat(StringBuilder* s, String str, Int count) {
	for (Int i = 0; i < count; i++) PrintS(s, str);
}

F_FN void PrintC(StringBuilder* s, const char* str) {
	ArrayPushNRaw(s->arena, (ArrayRaw*)&s->buffer, str, strlen(str), 1);
}

F_FN String APrint(Arena* arena, const char* fmt, ...) {
	ProfEnter();
	va_list args;
	va_start(args, fmt);

	StringBuilder builder = {0};
	builder.arena = arena;
	PrintVA(&builder, fmt, args);

	va_end(args);
	ProfExit();
	return builder.str;
}

// The format string is assumed to be valid, as currently no error handling is done.
F_FN void PrintVA(StringBuilder* s, const char* fmt, va_list args) {
	ProfEnter();
	for (const char* c = fmt; *c; c++) {
		if (*c == '~') {
			c++;
			if (*c == 0) break;

			switch (*c) {
			case 's': {
				PrintS(s, va_arg(args, String));
			} break;

			case 'c': {
				PrintC(s, va_arg(args, char*));
			} break;

			case '~': {
				PrintC(s, "`~");
			} break;

			case 'f': {
				U8 buffer[64];
				Int size = FloatToStr_(buffer, va_arg(args, double));
				PrintS(s, (String){buffer, size});
			} break;

			case 'x': // fallthrough
			case 'i': // fallthrough
			case 'u': {
				char format_c = *c;
				c++;
				if (*c == 0) break;

				uint64_t value = 0;
				switch (*c) {
				case '8': { value = va_arg(args, uint8_t); } break; // U8
				case '1': { value = va_arg(args, uint16_t); } break; // U16
				case '3': { value = va_arg(args, uint32_t); } break; // U32
				case '6': { value = va_arg(args, uint64_t); } break; // U16
				default: Assert(false);
				}

				if (*c != '8') { // U8 is the only one with only 1 character
					c++;
					if (*c == 0) break;
				}
				
				U8 buffer[100];
				Int size = IntToStr_(buffer, value, format_c == 'i', format_c == 'x' ? 16 : 10);
				PrintS(s, (String){buffer, size});
			} break;

			default: Assert(false);
			}
		}
		else {
			PrintB(s, *c);
		}
	}
	ProfExit();
}

F_FN void Print(StringBuilder* s, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	PrintVA(s, fmt, args);
	va_end(args);
}

F_FN String PathTail(String path) {
	ProfEnter();
	Int last_sep;
	if (StrLastIdxOfAnyChar(path, PATH_SEPARATOR_CHARS, &last_sep)) {
		path = StrSliceAfter(path, last_sep + 1);
	}
	ProfExit();
	return path;
}

F_FN String PathStem(String path) {
	ProfEnter();
	Int last_sep;
	if (StrLastIdxOfAnyChar(path, PATH_SEPARATOR_CHARS, &last_sep)) {
		path = StrSliceAfter(path, last_sep + 1);
	}

	Int last_dot;
	if (StrLastIdxOfAnyChar(path, L("."), &last_dot)) {
		path = StrSliceBefore(path, last_dot);
	}

	ProfExit();
	return path;
}

F_FN String StrClone(Arena* arena, String str) {
	ProfEnter();
	Array(U8) copied = {0};
	ArrayResizeUndef(arena, &copied, str.len);
	memcpy(copied.data, str.data, str.len);
	ProfExit();
	return ArrTo(String, copied);
}

F_FN bool StrEndsWith(String str, String end) {
	return str.len >= end.len && StrEquals(end, StrSliceAfter(str, str.len - end.len));
}

F_FN bool StrStartsWith(String str, String start) {
	return str.len >= start.len && StrEquals(start, StrSliceBefore(str, start.len));
}

F_FN bool StrCutEnd(String* str, String end) {
	ProfEnter();
	bool ends_with = StrEndsWith(*str, end);
	if (ends_with) {
		str->len -= end.len;
	}
	ProfExit();
	return ends_with;
}

F_FN bool StrCutStart(String* str, String start) {
	ProfEnter();
	bool starts_with = StrStartsWith(*str, start);
	if (starts_with) {
		*str = StrSliceAfter(*str, start.len);
	}
	ProfExit();
	return starts_with;
}

F_FN StrRangeSlice StrSplit(Arena* arena, String str, U8 character) {
	ProfEnter();
	Int required_len = 1;
	for (Int i = 0; i < str.len; i++) {
		if (str.data[i] == character) required_len++;
	}

	Array(StrRange) splits = {0};
	ArrayReserve(arena, &splits, required_len);

	Int prev = 0;
	for (Int i = 0; i < str.len; i++) {
		if (str.data[i] == character) {
			StrRange range = {prev, i};
			ArrayPush(arena, &splits, range);
			prev = i + 1;
		}
	}

	StrRange range = {prev, str.len};
	ArrayPush(arena, &splits, range);
	ProfExit();
	return ArrTo(StrRangeSlice, splits);
}

inline uint64_t Mix64Stream5(uint64_t h, uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
	ProfEnter();
	// https://github.com/jonmaiga/mx3/blob/master/mx3.h
	uint64_t C = 0xbea225f9eb34556d;
	a *= C;
	b *= C;
	c *= C;
	d *= C;
	a ^= a >> 39;
	b ^= b >> 39;
	c ^= c >> 39;
	d ^= d >> 39;
	h += a * C;
	h *= C;
	h += b * C;
	h *= C;
	h += c * C;
	h *= C;
	h += d * C;
	h *= C;
	ProfExit();
	return h;
}

F_FN U64 Hash64Ex(String data, U64 seed) {
	ProfEnter();
	// https://github.com/jonmaiga/mx3/blob/master/mx3.h
	size_t len = data.len;
	U64* budouble = (U64*)data.data;
	uint64_t h = Mix64_Stream(seed, len + 1);
	while (len >= 64) {
		len -= 64;
		h = Mix64Stream5(h, budouble[0], budouble[1], budouble[2], budouble[3]);
		h = Mix64Stream5(h, budouble[4], budouble[5], budouble[6], budouble[7]);
		budouble += 8;
	}

	while (len >= 8) {
		len -= 8;
		h = Mix64_Stream(h, budouble[0]);
		budouble += 1;
	}

	U8* tail8 = (U8*)budouble;
	U64* tail16 = (U64*)budouble;
	U32* tail32 = (U32*)budouble;

	U64 result;
	switch (len) {
	case 0: result = Mix64(h); break;
	case 1: result = Mix64(Mix64_Stream(h, tail8[0])); break;
	case 2: result = Mix64(Mix64_Stream(h, tail16[0])); break;
	case 3: result = Mix64(Mix64_Stream(h, tail16[0] | (U64)(tail8[2]) << 16)); break;
	case 4: result = Mix64(Mix64_Stream(h, tail32[0])); break;
	case 5: result = Mix64(Mix64_Stream(h, tail32[0] | (U64)(tail8[4]) << 32)); break;
	case 6: result = Mix64(Mix64_Stream(h, tail32[0] | (U64)(tail16[2]) << 32)); break;
	case 7: result = Mix64(Mix64_Stream(h, tail32[0] | (U64)(tail16[2]) << 32 | (U64)(tail8[6]) << 48)); break;
	default: result = Mix64(h); break;
	}
	ProfExit();
	return result;
}

F_FN void ArrayReserveRaw(Arena* arena, ArrayRaw* array, Int capacity, Int elem_size) {
	ProfEnter();
	if (capacity > array->capacity) {
		ProfEnterN(Resize);
		Int new_capacity = RoundUpToPow2(capacity);
		if (new_capacity < 8) new_capacity = 8;

		void* new_data = ArenaPushUndef(arena, new_capacity * elem_size, AutoAlign(elem_size));
		memcpy(new_data, array->data, array->len * elem_size);
		DebugFillGarbage(array->data, array->len * elem_size);

		array->data = new_data;
		array->capacity = new_capacity;
		ProfExitN(Resize);
	}
	ProfExit();
}

F_FN void ArrayRemoveRaw(ArrayRaw* array, Int i, Int elem_size) { ArrayRemoveNRaw(array, i, 1, elem_size); }

F_FN void ArrayRemoveNRaw(ArrayRaw* array, Int i, Int n, Int elem_size) {
	ProfEnter();
	Assert(i + n <= array->len);

	U8* dst = (U8*)array->data + i * elem_size;
	U8* src = dst + elem_size * n;
	memmove(dst, src, ((U8*)array->data + array->len * elem_size) - src);

	array->len -= n;
	ProfExit();
}

F_FN void ArrayInsertRaw(Arena* arena, ArrayRaw* array, Int at, const void* elem, Int elem_size) {
	ProfEnter();
	ArrayInsertNRaw(arena, array, at, elem, 1, elem_size);
	ProfExit();
}

F_FN void ArrayInsertNRaw(Arena* arena, ArrayRaw* array, Int at, const void* elems, Int n, Int elem_size) {
	ProfEnter();
	Assert(at <= array->len);
	ArrayReserveRaw(arena, array, array->len + n, elem_size);

	// Move existing elements forward
	U8* offset = (U8*)array->data + at * elem_size;
	memmove(offset + n * elem_size, offset, (array->len - at) * elem_size);

	memcpy(offset, elems, n * elem_size);
	array->len += n;
	ProfExit();
}

F_FN Int ArrayPushNRaw(Arena* arena, ArrayRaw* array, const void* elems, Int n, Int elem_size) {
	ProfEnter();
	ArrayReserveRaw(arena, array, array->len + n, elem_size);

	memcpy((U8*)array->data + elem_size * array->len, elems, elem_size * n);

	Int result = array->len;
	array->len += n;
	ProfExit();
	return result;
}

F_FN void ArrayResizeRaw(Arena* arena, ArrayRaw* array, Int len, Opt(const void*) value, Int elem_size) {
	ProfEnter();
	ArrayReserveRaw(arena, array, len, elem_size);

	if (value) {
		for (Int i = array->len; i < len; i++) {
			memcpy((U8*)array->data + i * elem_size, value, elem_size);
		}
	}

	array->len = len;
	ProfExit();
}

F_FN Int ArrayPushRaw(Arena* arena, ArrayRaw* array, const void* elem, Int elem_size) {
	ProfEnter();
	ArrayReserveRaw(arena, array, array->len + 1, elem_size);

	memcpy((U8*)array->data + array->len * elem_size, elem, elem_size);
	ProfExit();
	return array->len++;
}

F_FN void ArrayPopRaw(ArrayRaw* array, Opt(void*) out_elem, Int elem_size) {
	ProfEnter();
	Assert(array->len >= 1);
	array->len--;
	if (out_elem) {
		memcpy(out_elem, (U8*)array->data + array->len * elem_size, elem_size);
	}
	ProfExit();
}

F_FN String FloatToStr(Arena* arena, double value) {
	U8 buffer[100];
	Int size = FloatToStr_(buffer, value);
	return StrClone(arena, (String){buffer, size});
}

F_FN String IntToStr(Arena* arena, Int value) { return IntToStrEx(arena, value, true, 10); }
F_FN String IntToStrEx(Arena* arena, U64 data, bool is_signed, Int radix) {
	U8 buffer[100];
	Int size = IntToStr_(buffer, data, is_signed, radix);
	return StrClone(arena, (String){buffer, size});
}

F_FN Int IntToStr_(U8* buffer, U64 data, bool is_signed, Int radix) {
	Assert(radix >= 2 && radix <= 16);
	Int offset = 0;

	U64 remaining = data;

	bool is_negative = is_signed && (I64)remaining < 0;
	if (is_negative) {
		remaining = -(I64)remaining; // Negate the value, then print it as it's now positive
	}

	for (;;) {
		U64 temp = remaining;
		remaining /= radix;

		U64 digit = temp - remaining * radix;
		buffer[offset++] = "0123456789ABCDEF"[digit];
		
		if (remaining == 0) break;
	}
	
	if (is_negative) buffer[offset++] = '-'; // Print minus sign

	// It's now printed in reverse, so let's reverse the digits
	Int i = 0;
	Int j = offset - 1;
	while (i < j) {
		U8 temp = buffer[j];
		buffer[j] = buffer[i];
		buffer[i] = temp;
		i++;
		j--;
	}
	return offset;
}

// Technique from: https://blog.benoitblanchon.fr/lightweight-float-to-string/
// - Supports printing up to nine digits after the decimal point
F_FN Int FloatToStr_(U8* buffer, double value) {
	Int offset = 0;
	if (isnan(value)) {
		memcpy(buffer + offset, "nan", 3), offset += 3;
		return offset;
	}

	if (value < 0.0) {
		buffer[offset++] = '-';
		value = -value;
	}

	if (isinf(value)) {
		memcpy(buffer + offset, "inf", 3), offset += 3;
		return offset;
	}

	// Split float
	uint32_t integralPart, decimalPart;
	int16_t exponent = 0;
	{
		// Normalize float
		{
			const double positiveExpThreshold = 1e7;
			const double negativeExpThreshold = 1e-5;

			if (value >= positiveExpThreshold) {
				if (value >= 1e256) {
					value /= 1e256;
					exponent += 256;
				}
				if (value >= 1e128) {
					value /= 1e128;
					exponent += 128;
				}
				if (value >= 1e64) {
					value /= 1e64;
					exponent += 64;
				}
				if (value >= 1e32) {
					value /= 1e32;
					exponent += 32;
				}
				if (value >= 1e16) {
					value /= 1e16;
					exponent += 16;
				}
				if (value >= 1e8) {
					value /= 1e8;
					exponent += 8;
				}
				if (value >= 1e4) {
					value /= 1e4;
					exponent += 4;
				}
				if (value >= 1e2) {
					value /= 1e2;
					exponent += 2;
				}
				if (value >= 1e1) {
					value /= 1e1;
					exponent += 1;
				}
			}

			if (value > 0 && value <= negativeExpThreshold) {
				if (value < 1e-255) {
					value *= 1e256;
					exponent -= 256;
				}
				if (value < 1e-127) {
					value *= 1e128;
					exponent -= 128;
				}
				if (value < 1e-63) {
					value *= 1e64;
					exponent -= 64;
				}
				if (value < 1e-31) {
					value *= 1e32;
					exponent -= 32;
				}
				if (value < 1e-15) {
					value *= 1e16;
					exponent -= 16;
				}
				if (value < 1e-7) {
					value *= 1e8;
					exponent -= 8;
				}
				if (value < 1e-3) {
					value *= 1e4;
					exponent -= 4;
				}
				if (value < 1e-1) {
					value *= 1e2;
					exponent -= 2;
				}
				if (value < 1e0) {
					value *= 1e1;
					exponent -= 1;
				}
			}
		}

		integralPart = (uint32_t)value;
		double remainder = value - integralPart;

		remainder *= 1e9;
		decimalPart = (uint32_t)remainder;  

		// rounding
		remainder -= decimalPart;
		if (remainder >= 0.5) {
			decimalPart++;
			if (decimalPart >= 1000000000) {
				decimalPart = 0;
				integralPart++;
				if (exponent != 0 && integralPart >= 10) {
					exponent++;
					integralPart = 1;
				}
			}
		}
	}

	offset += IntToStr_(buffer + offset, integralPart, false, 10); // writeInteger(integralPart);

	if (decimalPart) { // Write decimals
		int width = 9;

		// remove trailing zeros
		while (decimalPart % 10 == 0 && width > 0) {
			decimalPart /= 10;
			width--;
		}

		char temp_buffer[16];
		char* temp_str = temp_buffer + sizeof(temp_buffer);

		for (; width > 0; width--) {
			*(--temp_str) = '0' + decimalPart % 10;
			decimalPart /= 10;
		}
		*(--temp_str) = '.';
		
		Int temp_str_len = (Int)(temp_buffer + sizeof(temp_buffer) - temp_str);
		memcpy(buffer + offset, temp_str, temp_str_len), offset += temp_str_len;
	}

	if (exponent < 0) {
		memcpy(buffer + offset, "e-", 2), offset += 2;
		offset += IntToStr_(buffer + offset, -exponent, false, 10);
	}

	if (exponent > 0) {
		memcpy(buffer + offset, "e", 1), offset += 1;
		offset += IntToStr_(buffer + offset, exponent, false, 10);
	}
	return offset;
}

#define MAP_HASH_MASK ~(1u << 31u)  // Removes the tombstone bit from the hash
#define DEBUG_MAX_PROBE_SEQUENCE_LENGTH 16

F_FN Opt(void*) MapFindPtrRaw(MapRaw* map, const void* key, U32 hash, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset) {
	if (map->capacity == 0) return NULL;
	ProfEnter();

	if (hash == 0) {
		hash = (U32)Hash64Ex(StrFromData(key, K_size), 0) & MAP_HASH_MASK;
		if (hash == 0) hash = 1;
	}

	U32 mask = map->capacity - 1;
	U32 idx = hash & mask;
	U32 psl = 0;

	Opt(void*) found = NULL;
	for (;;) {
		U8* elem = (U8*)map->data + idx * elem_size;
		U32 elem_hash = *(U32*)elem;
		if (elem_hash == 0) break;

		U8* elem_key = elem + key_offset;
		U8* elem_val = elem + val_offset;

		U32 elem_home_idx = elem_hash & mask;
		U32 elem_psl = idx >= elem_home_idx ?
			idx - elem_home_idx :
			idx + (U32)map->capacity - elem_home_idx;

		if (elem_psl > DEBUG_MAX_PROBE_SEQUENCE_LENGTH) BP();
		if (psl > DEBUG_MAX_PROBE_SEQUENCE_LENGTH) BP();

		// If this element has a smaller probe sequence length than the current psl, then it cannot be the right element,
		// nor can the right element be after that, because then the table would have an unfair distribution.
		if (elem_psl < psl) break;

		if (memcmp(key, elem_key, K_size) == 0) {
			found = elem_val;
			break;
		}

		idx = (idx + 1) & mask;
		psl++;
	}

	ProfExit();
	return found;
}

F_FN bool MapFindRaw(MapRaw* map, const void* key, U32 hash, Opt(void*) out_val, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset) {
	ProfEnter();
	Opt(void*) ptr = MapFindPtrRaw(map, key, hash, K_size, V_size, elem_size, key_offset, val_offset);
	if (ptr) {
		if (out_val) memcpy(out_val, ptr, V_size);
	}
	ProfExit();
	return ptr != NULL;
}

F_FN bool MapAddRaw(Arena* arena, MapRaw* map, const void* key, U32 hash, Opt(void**) out_val_ptr, Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset) {
	ProfEnter();
	Int elem_align = AutoAlign(elem_size);

	if (hash == 0) {
		hash = (U32)Hash64Ex(StrFromData(key, K_size), 0) & MAP_HASH_MASK;
		if (hash == 0) hash = 1;
	}
	else {
		U32 test_hash = (U32)Hash64Ex(StrFromData(key, K_size), 0) & MAP_HASH_MASK;
		if (test_hash == 0) test_hash = 1;
		Assert(hash == test_hash);
	}

	// We need to first check if we need to grow the map
	// map->len/map->capacity > 70/100
	if (100*(map->len + 1) > 70*map->capacity) {
		ProfEnterN(Grow);
		U8* old_data = (U8*)map->data;
		Int old_capacity = map->capacity;

		//map->capacity = Max(map->capacity, 8) * 2; // Start at 16 slots initially, then double on every growth
		// TODO
		map->capacity = 4096;
		Assert(map->len < 700);

		map->data = ArenaPushUndef(arena, map->capacity * elem_size, elem_align);
		memset(map->data, 0, map->capacity * elem_size); // to set hash values to 0

		//if (map->data == (void*)0x0000000140ae6c60) BP();

		map->len = 0;
		//map->max_probe_seq_length = 0;

		for (Int i = 0; i < old_capacity; i++) {
			U8* elem = old_data + elem_size * i;
			U32 elem_hash = *(U32*)elem;
			void* elem_key = elem + key_offset;
			void* elem_val = elem + val_offset;

			if (elem_hash != 0 && (elem_hash >> 31) == 0) {
				void* new_val_ptr;
				Assert(MapAddRaw(arena, map, elem_key, elem_hash, &new_val_ptr, K_size, V_size, elem_size, key_offset, val_offset));
				memcpy(new_val_ptr, elem_val, V_size);
			}
		}

		DebugFillGarbage(old_data, old_capacity * elem_size);
		ProfExitN(Grow);
	}

	U32 mask = map->capacity - 1;
	U32 idx = hash & mask;

	U64 tmp1[4096/8], tmp2[4096/8];
	Assert(elem_size < 4096);

	// HOW THE HASH MAP WORKS:
	// With hashes, 0 is a special value and means "unused", while the most significant bit tells if it's tombstone or not

	//Scope T = ScopePush(arena);
	//ProfEnterN(AllocTemps);
	//U8* tmp1 = ArenaPushUndef(T.arena, elem_size, elem_align);
	//U8* tmp2 = ArenaPushUndef(T.arena, elem_size, elem_align);
	//ProfExitN(AllocTemps);

	U32 inserting_hash = hash;
	const void* inserting_key = key;
	Opt(const void*) inserting_val = NULL;

	U32 psl = 0; // probe sequence length
	bool added_new = false;

	for (;;) {
		ProfEnterN(Probe);
		//CB(_c, 105);
		U8* elem = (U8*)map->data + idx * elem_size;
		//if (elem == (void*)0x0000000140aedd88) BP();

		U32* elem_hash = (U32*)elem;
		U8* elem_key = elem + key_offset;
		U8* elem_val = elem + val_offset;

		if (*elem_hash == 0) {
			*elem_hash = inserting_hash;
			memcpy(elem_key, inserting_key, K_size);
			if (inserting_val) memcpy(elem_val, inserting_val, V_size);

			if (out_val_ptr) *out_val_ptr = elem_val;
			added_new = true;
			ProfExitN(Probe); break;
		}

		bool is_tombstone = (*elem_hash >> 31) != 0;
		Assert(!is_tombstone);

		if (memcmp(key, elem_key, K_size) == 0) {
			if (out_val_ptr) *out_val_ptr = elem_val;
			ProfExitN(Probe); break;
		}

		U32 elem_home_idx = *elem_hash & mask;
		U32 elem_psl = idx >= elem_home_idx ?
			idx - elem_home_idx :
			idx + (U32)map->capacity - elem_home_idx;

		if (elem_psl > DEBUG_MAX_PROBE_SEQUENCE_LENGTH) BP();
		if (psl > DEBUG_MAX_PROBE_SEQUENCE_LENGTH) BP();

		// Steal from the rich?
		if (elem_psl < psl) {
			ProfEnterN(StealFromRich);

			// Insert our value into this slot, then take the victim to be inserted.
			memcpy(tmp1, elem, elem_size);

			*elem_hash = inserting_hash;
			memcpy(elem_key, inserting_key, K_size);
			if (inserting_val) memcpy(elem_val, inserting_val, V_size);

			memcpy(tmp2, tmp1, elem_size);
			inserting_hash = *(U32*)tmp2;
			inserting_key = tmp2 + key_offset;
			inserting_val = tmp2 + val_offset;

			psl = elem_psl;
			ProfExitN(StealFromRich);
		}

		idx = (idx + 1) & mask;
		psl++;
		ProfExitN(Probe);
	}

	ProfEnterN(Finalize);
	map->len++;
	//ScopePop(T);
	ProfExitN(Finalize);
	ProfExit();
	return added_new;
}

F_FN bool MapAssignRaw(Arena* arena, MapRaw* map, const void* key, U32 hash, Opt(void*) val,
	Int K_size, Int V_size, Int elem_size, Int key_offset, Int val_offset)
{
	ProfEnter();
	void* val_ptr;
	bool added = MapAddRaw(arena, map, key, hash, &val_ptr, K_size, V_size, elem_size, key_offset, val_offset);
	memcpy(val_ptr, val, V_size);
	ProfExit();
	return added;
}

F_FN Int RoundUpToPow2(Int v) {
	ProfEnter();
	// todo: use the following formula from Tilde Backend
	// x == 1 ? 1 : 1 << (64 - _lzcnt_u64(x - 1));

	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;
	ProfExit();
	return v;
}

// `value` must be a power of 2
F_FN Int log2_uint(Int value) {
	ProfEnter();
	Assert(IsPowerOf2(value));
	Int result = 0;
	for (; value > 1;) {
		value >>= 1;
		result++;
	}
	ProfExit();
	return result;
}

F_FN String StrJoinN(StringSlice args, Arena* arena) {
	ProfEnter();
	Int offset = 0;
	ArrayEach(String, args, it) {
		offset += it.elem->len;
	}

	String result = { ArenaPushUndef(arena, offset + 1, 1), offset };

	offset = 0;
	ArrayEach(String, args, it) {
		memcpy((U8*)result.data + offset, it.elem->data, it.elem->len);
		offset += it.elem->len;
	}

	*((U8*)result.data + offset) = 0; // Add a null termination for better visualization in a debugger
	ProfExit();
	return result;
}

F_FN DefaultArena* MakeDefaultArena(Int block_size) {
	// FIXME: If you want to make individual allocations (block_size=0), then this method of shoving the arena structure into itself will currently turn that into 2 individual allocations.
	DefaultArena _arena = {.block_size = block_size};
	DefaultArena* ptr = (DefaultArena*)DefaultArenaPushUndef(&_arena, sizeof(DefaultArena), 1);
	*ptr = _arena;
	return ptr;
}

F_FN void DestroyDefaultArena(DefaultArena* arena) {
	DefaultArenaReset(arena); // this will fill the arena memory with garbage in debug builds

	for (DefaultArenaBlockHeader* block = arena->first_block; block;) {
		DefaultArenaBlockHeader* next = block->next;
		MemFree(block);
		block = next;
	}

	DebugFillGarbage(arena, sizeof(DefaultArena));
}

F_FN void* DefaultArenaPushUndef(Arena* arena, Int size, Int alignment) {
	Assert(alignment != 0 && IsPowerOf2(alignment));

	DefaultArenaMark mark = arena->mark;
	if (mark.block) Assert((Int)mark.ptr >= (Int)mark.block && (Int)mark.ptr <= (Int)mark.block + mark.block->size_including_header);

	Int block_remaining_size = mark.ptr - ((U8*)mark.block + sizeof(DefaultArenaBlockHeader));
	if (mark.block == NULL || (Int)size > block_remaining_size) {
		// Need new block!
		Assert(alignment <= 16);

		Int new_block_size = Max(AlignUpPow2((Int)sizeof(DefaultArenaBlockHeader), (Int)alignment) + (Int)size, arena->block_size);
		DefaultArenaBlockHeader* new_block = MemAlloc(new_block_size);

		new_block->size_including_header = new_block_size;
		new_block->next = NULL;

		// TODO: ListPushBack
		if (mark.block) mark.block->next = new_block;
		else arena->first_block = new_block;
		mark.block = new_block;
		mark.ptr = (U8*)AlignDownPow2((Int)mark.block + new_block_size - size, alignment);
		Assert(mark.ptr >= (U8*)mark.block + sizeof(DefaultArenaBlockHeader));
	}
	else {
		mark.ptr = (U8*)AlignDownPow2((Int)mark.ptr - size, alignment);
	}

	arena->mark = mark;
	if (arena->mark.block) {
		Assert((Int)arena->mark.ptr >= (Int)arena->mark.block && (Int)arena->mark.ptr <= (Int)arena->mark.block + arena->mark.block->size_including_header);
	}
	return mark.ptr;
}

F_FN ArenaMark DefaultArenaGetMark(Arena* arena) {
	return arena->mark;
}

F_FN void DefaultArenaSetMark(Arena* arena, ArenaMark mark) {
#ifdef FIRE_H_MODE_DEBUG
	//Assert((Int)mark.ptr >= (Int)mark.block && (Int)mark.ptr <= (Int)mark.block + mark.block->size_including_header);
	// TODO: why is the following so slow??

	//DebugFillGarbage((U8*)mark.block + sizeof(ArenaBlockHeader), mark.ptr - (U8*)(mark.block + 1));
	//for (ArenaBlockHeader* block = mark.block->next; block && block != arena->mark.block; block = block->next) {
	//	DebugFillGarbage(block + 1, block->size_including_header - sizeof(ArenaBlockHeader));
	//}
#endif
	arena->mark = mark;
}

F_FN void DefaultArenaReset(Arena* arena) {
	arena->mark.block = arena->first_block;
	arena->mark.ptr = (U8*)arena->first_block + sizeof(DefaultArenaBlockHeader) + sizeof(DefaultArena);
}

F_FN I64 FloorToI64(float x) {
	ProfEnter();
	//Assert(x > (float)I64_MIN && x < (float)I64_MAX);

	I64 x_i64 = (I64)x;
	if (x < 0) {
		float fraction = (float)x_i64 - x;
		if (fraction != 0) x_i64 -= 1;
	}
	ProfExit();
	return x_i64;
}

static const U32 UTF8_OFFSETS[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

F_FN Int StrEncodeRune(U8* output, Rune r) {
	ProfEnter();
	// Original implementation by Jeff Bezanson from https://www.cprogramming.com/tutorial/utf8.c (public domain)
	U32 ch;

	ch = r;
	Int result;
	if (ch < 0x80) {
		*output++ = (char)ch;
		result = 1;
	}
	else if (ch < 0x800) {
		*output++ = (ch >> 6) | 0xC0;
		*output++ = (ch & 0x3F) | 0x80;
		result = 2;
	}
	else if (ch < 0x10000) {
		*output++ = (ch >> 12) | 0xE0;
		*output++ = ((ch >> 6) & 0x3F) | 0x80;
		*output++ = (ch & 0x3F) | 0x80;
		result = 3;
	}
	else if (ch < 0x110000) {
		*output++ = (ch >> 18) | 0xF0;
		*output++ = ((ch >> 12) & 0x3F) | 0x80;
		*output++ = ((ch >> 6) & 0x3F) | 0x80;
		*output++ = (ch & 0x3F) | 0x80;
		result = 4;
	}
	else Assert(false);
	ProfExit();
	return result;
}

F_FN Rune StrDecodeRune(String str) {
	Int offset = 0;
	return StrNextRune(str, &offset);
}

F_FN Rune StrNextRune(String str, Int* byteoffset) {
	if (*byteoffset >= str.len) return 0;
	ProfEnter();
	Assert(*byteoffset >= 0);

	// Original implementation by Jeff Bezanson from https://www.cprogramming.com/tutorial/unicode.html (public domain)

	U32 ch = 0;
	int sz = 0;

	do {
		ch <<= 6;
		ch += str.data[(*byteoffset)++];
		sz++;
	} while (*byteoffset < str.len && !StrIsUtf8FirstByte(str.data[*byteoffset]));
	ch -= UTF8_OFFSETS[sz - 1];

	ProfExit();
	return (Rune)ch;
}

F_FN Rune StrPrevRune(String str, Int* byteoffset) {
	// Original implementation by Jeff Bezanson from https://www.cprogramming.com/tutorial/unicode.html (public domain)
	if (*byteoffset <= 0) return 0;

	(void)(StrIsUtf8FirstByte(str.data[--(*byteoffset)]) ||
		StrIsUtf8FirstByte(str.data[--(*byteoffset)]) ||
		StrIsUtf8FirstByte(str.data[--(*byteoffset)]) || --(*byteoffset));

	Int b = *byteoffset;
	return StrNextRune(str, &b);
}

F_FN Int StrRuneCount(String str) {
	ProfEnter();
	Int i = 0;
	for StrEach(str, r, offset) { i++; Unused(offset); }
	ProfExit();
	return i;
}

// https://graphitemaster.github.io/aau/#unsigned-multiplication-can-overflow
inline bool DoesMulOverflow(U64 x, U64 y) { return y && x > ((U64)-1) / y; }
inline bool DoesAddOverflow(U64 x, U64 y) { return x + y < x; }
//inline bool DoesSubUnderflow(U64 x, U64 y) { return x - y > x; }

F_FN bool StrToU64Ex(String s, Int base, U64* out_value) {
	ProfEnter();
	Assert(2 <= base && base <= 16);

	U64 value = 0;
	Int i = 0;
	for (; i < s.len; i++) {
		Int c = s.data[i];
		if (c == '_') continue;

		if (c >= 'A' && c <= 'Z') c += 32; // ASCII convert to lowercase

		Int digit;
		if (c >= '0' && c <= '9') {
			digit = c - '0';
		}
		else if (c >= 'a' && c <= 'f') {
			digit = 10 + c - 'a';
		}
		else break;

		if (digit >= base) break;

		if (DoesMulOverflow(value, base)) break;
		value *= base;
		if (DoesAddOverflow(value, digit)) break;
		value += digit;
	}
	*out_value = value;
	ProfExit();
	return i == s.len;
}

F_FN bool StrToI64Ex(String s, Int base, I64* out_value) {
	ProfEnter();
	Assert(2 <= base && base <= 16);

	I64 sign = 1;
	if (s.len > 0) {
		if (s.data[0] == '+') StrAdvance(&s, 1);
		else if (s.data[0] == '-') {
			StrAdvance(&s, 1);
			sign = -1;
		}
	}

	I64 val;
	bool ok = StrToU64Ex(s, base, (U64*)&val) && val >= 0;
	if (ok) *out_value = val * sign;
	ProfExit();
	return ok;
}

F_FN char* StrToCStr(String s, Arena* arena) {
	ProfEnter();
	char* bytes = ArenaPushUndef(arena, s.len + 1, 1);
	memcpy(bytes, s.data, s.len);
	bytes[s.len] = 0;
	ProfExit();
	return bytes;
}

static double StrToFloat_(const char *str, int *success) {
	// Original implementation by Michael Hartmann https://github.com/michael-hartmann/parsefloat/blob/master/strtodouble.c (public domain)

	double intpart = 0, fracpart = 0;
	int sign = +1, len = 0, conversion = 0, exponent = 0;

	// skip whitespace
	//while(isspace(*str)) str++;

	// check for sign (optional; either + or -)
	if (*str == '-') {
		sign = -1;
		str++;
	}
	else if (*str == '+') str++;

	// check for nan and inf
	if (ToLower(str[0]) == 'n' && ToLower(str[1]) == 'a' && ToLower(str[2]) == 'n') {
		if(success != NULL) *success = 1;
		return NAN;
	}
	if (ToLower(str[0]) == 'i' && ToLower(str[1]) == 'n' && ToLower(str[2]) == 'f') {
		if(success != NULL) *success = 1;
		return sign * INFINITY;
	}

	// find number of digits before decimal point
	{
		const char *p = str;
		len = 0;
		while (*p >= '0' && *p <= '9') {
			p++;
			len++;
		}
	}

	if (len) conversion = 1;

	// convert intpart part of decimal point to a float
	{
		double f = 1;
		for (int i = 0; i < len; i++) {
			int v = str[len-1-i] - '0';
			intpart += v*f;
			f *= 10;
		}
		str += len;
	}

	// check for decimal point (optional)
	if (*str == '.') {
		const char *p = ++str;

		// find number of digits after decimal point
		len = 0;
		while (*p >= '0' && *p <= '9') {
			p++;
			len++;
		}

		if (len) conversion = 1;

		// convert fracpart part of decimal point to a float
		double f = 0.1;
		for (int i = 0; i < len; i++) {
			int v = str[i] - '0';
			fracpart += v*f;
			f *= 0.1;
		}

		str = p;
	}

	if (conversion && (*str == 'e' || *str == 'E')) {
		int expsign = +1;
		const char *p = ++str;

		if (*p == '+') p++;
		else if (*p == '-') {
			expsign = -1;
			p++;
		}

		str = p;
		len = 0;
		while (*p >= '0' && *p <= '9') {
			len++;
			p++;
		}

		int f = 1;
		for (int i = 0; i < len; i++) {
			int v = str[len - 1 - i] - '0';
			exponent += v*f;
			f *= 10;
		}

		exponent *= expsign;
	}

	if (!conversion) {
		if (success) *success = 0;
		return NAN;
	}

	if (success) *success = 1;

	return sign * (intpart + fracpart) * pow(10.0, exponent);
}

F_FN bool StrToFloat(String s, double* out) {
	ProfEnter();

	Assert(s.len < 63);
	char cstr[64] = {0};
	memcpy(cstr, s.data, s.len);

	// https://github.com/michael-hartmann/parsefloat/blob/master/strtodouble.c
	char* end;
	int success;
	*out = StrToFloat_(cstr, &success);

	ProfExit();
	return success == 1;
}

// supports UTF8
F_FN String StrReplace(Arena* arena, String str, String search_for, String replace_with) {
	if (search_for.len > str.len) return str;
	ProfEnter();

	Array(U8) result = {0};
	ArrayReserve(arena, &result, str.len * 2);

	Int last = str.len - search_for.len;

	for (Int i = 0; i <= last;) {
		if (memcmp(str.data + i, search_for.data, search_for.len) == 0) {
			ArrayPushN(arena, &result, replace_with);
			i += search_for.len;
		}
		else {
			ArrayPush(arena, &result, str.data[i]);
			i++;
		}
	}
	ProfExit();
	return ArrTo(String, result);
}

F_FN String StrReplaceMulti(Arena* arena, String str, StringSlice search_for, StringSlice replace_with) {
	ProfEnter();
	Assert(search_for.len == replace_with.len);
	Int n = search_for.len;

	Array(U8) result = {0};
	ArrayReserve(arena, &result, str.len*2);

	for (Int i = 0; i < str.len;) {
		for (Int j = 0; j < n; j++) {
			String search_for_j = ((String*)search_for.data)[j];
			if (i + search_for_j.len > str.len) continue;

			if (memcmp(str.data + i, search_for_j.data, search_for_j.len) == 0) {
				String replace_with_j = ((String*)replace_with.data)[j];
				ArrayPushN(arena, &result, replace_with_j);
				i += search_for_j.len;
				goto continue_outer;
			}
		}

		ArrayPush(arena, &result, str.data[i]);
		i++;
	continue_outer:;
	}
	ProfExit();
	return ArrTo(String, result);
}

F_FN String CStrToStr(const char* s) {
	return (String){(U8*)s, strlen(s)};
}

F_FN String StrAdvance(String* str, Int len) {
	ProfEnter();
	String result = StrSliceBefore(*str, len);
	*str = StrSliceAfter(*str, len);
	ProfExit();
	return result;
}

F_FN Rune ToLower(Rune r) { // TODO: utf8
	return r >= 'A' && r <= 'Z' ? r + 32 : r;
}

F_FN String StrToLower(Arena* arena, String str) {
	ProfEnter();
	U8Array out = ArrayClone(U8Array, arena, str);
	for (Int i = 0; i < out.len; i++) {
		out.data[i] = ToLower(out.data[i]);
	}
	ProfExit();
	return ArrTo(String, out);
}

#endif // F_INCLUDED
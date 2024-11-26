// fire_ds.h - by Eero Mutka (https://eeromutka.github.io/)
//
// Features:
// - Dynamic arrays
// - Hash maps & sets
// - Memory arenas
// - Bucket arrays
// - Slot allocators
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than DS_, simply do a find and replace in this file.
// 

#ifndef FIRE_DS_INCLUDED
#define FIRE_DS_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>  // memcpy, memmove, memset, memcmp, strlen

#ifndef DS_NO_MALLOC
#include <stdlib.h>
#endif

#ifndef DS_PROFILER_MACROS_OVERRIDE
#define DS_ProfEnter()  // Function-level profiler scope. A single function may only have one of these, and it should span the entire function.
#define DS_ProfExit()
#endif

#ifndef DS_ASSERT
#include <assert.h>
#define DS_ASSERT(x) assert(x)
#endif

#ifndef DS_API
#define DS_API static
#endif

// DS_OUT is to mark output parameters; you may pass NULL to these to ignore them.
#ifndef DS_OUT
#define DS_OUT
#endif

#ifndef DS_MAX_ELEM_SIZE
#define DS_MAX_ELEM_SIZE 2048
#endif

#ifdef __cplusplus
#define DS_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#else
#define DS_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#endif

#ifndef DS_ARENA_BLOCK_ALIGNMENT
#define DS_ARENA_BLOCK_ALIGNMENT 16
#endif

#ifndef DS_DEFAULT_ALLOCATOR_PROC_ALIGNMENT
#define DS_DEFAULT_ALLOCATOR_PROC_ALIGNMENT 16 // MSVC's malloc uses 16-byte alignment when building in 64-bit mode
#endif

#ifndef DS_DEFAULT_ARENA_PUSH_ALIGNMENT
#define DS_DEFAULT_ARENA_PUSH_ALIGNMENT 8
#endif

typedef struct DS_AllocatorBase {
	// AllocatorProc is a combination of malloc, free and realloc.
	// A new allocation is made when new_size > 0.
	// An existing allocation is freed when new_size == 0; in this case the old_size parameter is ignored.
	// To resize an existing allocation, pass the existing pointer into `ptr` and its size into `old_size`.
	void* (*AllocatorProc)(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align);
} DS_AllocatorBase;

// ----------------------------------------------------------

typedef struct DS_Arena DS_Arena;

// For convenience, DS_Allocator is type-defined to be DS_Arena to make it easy to pass an arena pointer to any parameter
// of type DS_Allocator*, but it can be anything that points to DS_AllocatorBase.
typedef DS_Arena DS_Allocator;

typedef struct DS_ArenaBlockHeader {
	size_t size_including_header;
	struct DS_ArenaBlockHeader* next; // may be NULL
} DS_ArenaBlockHeader;

typedef struct DS_ArenaMark {
	DS_ArenaBlockHeader* block; // If the arena has no blocks allocated yet, then we mark the beginning of the arena by setting this member to NULL.
	char* ptr;
} DS_ArenaMark;

struct DS_Arena {
	DS_AllocatorBase base;
	DS_ArenaBlockHeader* first_block; // may be NULL
	DS_ArenaMark mark;
	DS_Allocator* allocator;
	size_t block_size;
	size_t total_mem_reserved;
};

// --- Internal helpers -------------------------------------

#define DS_Concat_(a, b) a ## b
#define DS_Concat(a, b) DS_Concat_(a, b)

#ifdef __cplusplus
#define DS_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#define DS_LangAgnosticZero(T) T{}
#else
#define DS_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#define DS_LangAgnosticZero(T) (T){0}
#endif

#ifndef DS_NO_MALLOC
static void* DS_HeapAllocatorProc(DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	if (size == 0) {
		_aligned_free(ptr);
		return NULL;
	} else {
		return _aligned_realloc(ptr, size, align);
	}
}
static const DS_AllocatorBase DS_HEAP_ = { DS_HeapAllocatorProc };
#define DS_HEAP (DS_Allocator*)(&DS_HEAP_)
#endif

// Results in a compile-error if `elem` does not match the array's element type
#define DS_ArrTypecheck(array, elem) (void)((array)->data == elem)

#define DS_MapTypecheckK(MAP, PTR) ((PTR) == &(MAP)->data->key)
#define DS_MapTypecheckV(MAP, PTR) ((PTR) == &(MAP)->data->value)
#define DS_MapKSize(MAP) sizeof((MAP)->data->key)
#define DS_MapVSize(MAP) sizeof((MAP)->data->value)
#define DS_MapElemSize(MAP) sizeof(*(MAP)->data)
#define DS_MapKOffset(MAP) (int)((uintptr_t)&(MAP)->data->key - (uintptr_t)(MAP)->data)
#define DS_MapVOffset(MAP) (int)((uintptr_t)&(MAP)->data->value - (uintptr_t)(MAP)->data)

#define DS_BucketElemSize(ARRAY) sizeof(*(ARRAY)->buckets->elems)

#ifdef __cplusplus
template<typename T> static inline T* DS_Clone__(DS_Arena* a, const T& v) { T* x = (T*)DS_ArenaPush(a, sizeof(T)); *x = v; return x; }
#define DS_Clone_(T, ARENA, ...) DS_Clone__<T>(ARENA, __VA_ARGS__)
#else
#define DS_Clone_(T, ARENA, ...) ((T*)0 == &(__VA_ARGS__), (T*)DS_MemClone(ARENA, &(__VA_ARGS__), sizeof(__VA_ARGS__)))
#endif

// -------------------------------------------------------------------

#define DS_ArrayCount(x) (sizeof(x) / sizeof(x[0]))

// `p` must be a power of 2.
// `x` is allowed to be negative as well.
#define DS_AlignUpPow2(x, p) (((x) + (p) - 1) & ~((p) - 1)) // e.g. (x=30, p=16) -> 32
#define DS_AlignDownPow2(x, p) ((x) & ~((p) - 1)) // e.g. (x=30, p=16) -> 16

#ifdef DS_MODE_DEBUG
static inline void DS_ArrBoundsCheck_(bool x) { DS_ASSERT(x); }
#define DS_ArrBoundsCheck(ARR, INDEX) DS_ArrBoundsCheck_((size_t)(INDEX) < (size_t)(ARR).count)
#else
#define DS_ArrBoundsCheck(ARR, INDEX) (void)0
#endif

#define DS_KIB(x) ((uint64_t)(x) << 10)
#define DS_MIB(x) ((uint64_t)(x) << 20)
#define DS_GIB(x) ((uint64_t)(x) << 30)
#define DS_TIB(x) ((uint64_t)(x) << 40)

#define DS_Clone(T, ARENA, ...) DS_Clone_(T, ARENA, __VA_ARGS__)

#define DS_New(T, ARENA) (T*)DS_Clone(T, (ARENA), DS_LangAgnosticZero(T))

#ifndef DS_DebugFillGarbage
#define DS_DebugFillGarbage(ptr, size) memset(ptr, 0xCC, size)
#endif

// -- Map & Set ---------------------------------------------
//
// Generic hash map & hash set implementation.
//
// Map example:
//   DS_Arena *arena = ...
//   
//   DS_Map(int, float) map = {0};
//   map.arena = arena; // This is optional, otherwise the heap allocator will be used.
//   
//   int foo = 20;
//   float bar = 0.7f;
//   if (DS_MapInsert(&map, &foo, &bar)) { /* This scope will run, because the key was newly added. */ }
//   if (DS_MapInsert(&map, &foo, &bar)) { /* This scope won't run, because the key already exists. */ }
//   
//   float result_1;
//   if (DS_MapFind(&map, &foo, &result_1)) { /* This scope will run, because the key exists and result_1 will be 0.7f. */ }
//   
//   float *result_1_ptr;
//   if (DS_MapFindPtr(&map, &foo, &result_1_ptr)) { /* This scope will also run. */ }
//   
//   /* Iterate through the key-value pairs */
//   DS_ForMapEach(int, float, &map, it) {
//      int key = *it.key;
//      float value = *it.value;
//      ...
//   }
//   
//   if (DS_MapRemove(&map, &foo)) { /* This scope will run, because the key exists. */ }
//   if (DS_MapRemove(&map, &foo)) { /* This scope won't run, because the key doesn't exist anymore. */ }
//   
//   int baz = -100;
//   float *qux;
//   if (DS_MapGetOrAddPtr(&map, &baz, &qux)) {
//      /* This scope will run, because baz was newly added to the map. */
//      *qux = 123.4f;
//   }
//   else {
//      /* This scope won't run, because baz did not exist before, but if it did, we could look at the value of qux. */
//   }
//   
//   DS_MapDeinit(&map); // Reset the map and free the memory if using the heap allocator
// 

// Basic hash functions
DS_API uint32_t DS_MurmurHash3(const void* key, size_t size, uint32_t seed);
DS_API uint64_t DS_MurmurHash64A(const void* key, size_t size, uint64_t seed);

// If using a struct as the key type in a DS_Map, it must not contain any compiler-generated padding, as that could cause
// unpredictable behaviour when memcmp is used on them.
#define DS_NoteAboutKeyTypePadding

#define DS_Map(K, V) \
	struct { DS_Allocator* allocator; struct{ uint32_t hash; K key; V value; }* data; int32_t count; int32_t capacity; }
typedef DS_Map(char, char) DS_MapRaw;

#define DS_MapInit(MAP, ALLOCATOR)            DS_MapInitRaw((DS_MapRaw*)(MAP), (ALLOCATOR))

#define DS_MapInitClone(MAP, SRC, ALLOCATOR)  DS_MapInitCloneRaw((DS_MapRaw*)(MAP), (DS_MapRaw*)(SRC), (ALLOCATOR), DS_MapElemSize(SRC))

// * Returns true if the key was found.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapFind(MAP, KEY, OUT_VALUE) /* (DS_Map(K, V)* MAP, K KEY, (optional null) V* OUT_VALUE) */ \
	(DS_MapTypecheckK((MAP), &(KEY)) && DS_MapTypecheckV(MAP, OUT_VALUE), \
	DS_MapFindRaw((DS_MapRaw*)(MAP), &(KEY), OUT_VALUE, DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Returns the address of the value if the key was found, otherwise NULL.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapFindPtr(MAP, KEY) /* (DS_Map(K, V)* MAP, K KEY) */ \
	(DS_MapTypecheckK((MAP), &(KEY)), \
	DS_MapFindPtrRaw((DS_MapRaw*)(MAP), &(KEY), DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Returns true if the key was newly added.
// * Existing keys get overwritten with the new value.
// * KEY and VALUE must be l-values, otherwise this macro won't compile.
#define DS_MapInsert(MAP, KEY, VALUE) /* (DS_Map(K, V)* MAP, K KEY, V VALUE) */ \
	(DS_MapTypecheckK(MAP, &(KEY)) && DS_MapTypecheckV(MAP, &(VALUE)), \
	DS_MapInsertRaw((DS_MapRaw*)(MAP), &(KEY), &(VALUE), DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Returns true if the key was found and removed.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapRemove(MAP, KEY) /* (DS_Map(K, V)* MAP, K KEY) */ \
	(DS_MapTypecheckK(MAP, &(KEY)), \
	DS_MapRemoveRaw((DS_MapRaw*)(MAP), &(KEY), DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Return true if the key was newly added.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapGetOrAddPtr(MAP, KEY, OUT_VALUE) /* (DS_Map(K, V)* MAP, K KEY, V** OUT_VALUE) */ \
	(DS_MapTypecheckK(MAP, &(KEY)) && DS_MapTypecheckV(MAP, *(OUT_VALUE)), \
	DS_MapGetOrAddRaw((DS_MapRaw*)(MAP), &(KEY), (void**)OUT_VALUE, DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

#define DS_MapClear(MAP) \
	DS_MapClearRaw((DS_MapRaw*)(MAP), DS_MapElemSize(MAP))

// * Reset the map to a default state and free its memory if using the heap allocator.
#define DS_MapDeinit(MAP) /* (DS_Map(K, V)* MAP) */ \
	DS_MapDeinitRaw((DS_MapRaw*)(MAP), DS_MapElemSize(MAP))

#define DS_ForMapEach(K, V, MAP, IT) /* (type K, type V, DS_Map(K, V)* MAP, name IT) */ \
	struct DS_Concat(_dummy_, __LINE__) { int i_next; K *key; V *value; }; \
	if ((MAP)->count > 0) for (struct DS_Concat(_dummy_, __LINE__) IT = {0}; \
		DS_MapIter((DS_MapRaw*)(MAP), &IT.i_next, (void**)&IT.key, (void**)&IT.value, DS_MapKOffset(MAP), DS_MapVOffset(MAP), DS_MapElemSize(MAP)); )

//
// A Set is just a Map without values.
// 
// Set example:
//   DS_Arena *arena = ...
//   
//   DS_Set(int) set = {0};
//   set.arena = arena; // This is optional, otherwise the heap allocator will be used.
//   
//   int foo = 20;
//   if (DS_SetAdd(&set, &foo)) { /* This scope will run, because the key was newly added. */ }
//   if (DS_SetAdd(&set, &foo)) { /* This scope won't run, because the key already exists. */ }
//   
//   if (DS_SetContains(&set, &foo)) { /* This scope will run, because the key exists. */ }
//   
//   /* Iterate through the keys */
//   DS_ForSetEach(int, &set, it) {
//      int key = *it.key;
//      ...
//   }
//   
//   if (DS_SetRemove(&set, &foo)) { /* This scope will run, because the key exists. */ }
//   if (DS_SetRemove(&set, &foo)) { /* This scope won't run, because the key doesn't exist anymore. */ }
//
//   DS_SetDeinit(&set); // Reset the set and free the memory if using the heap allocator
//

#define DS_Set(K) \
	struct { DS_Allocator* allocator; struct{ uint32_t hash; K key; } *data; int32_t count; int32_t capacity; }
typedef DS_Set(char) DS_SetRaw;

#define DS_SetInit(SET, ALLOCATOR)        DS_MapInitRaw((DS_MapRaw*)(SET), (ALLOCATOR))

// * Returns true if the key was found.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_SetContains(SET, KEY) /* (DS_Set(K) *SET, K KEY) */ \
	(DS_MapTypecheckK(SET, &(KEY)), \
	DS_MapFindRaw((DS_MapRaw*)SET, &(KEY), NULL, DS_MapKSize(SET), 0, DS_MapElemSize(SET), DS_MapKOffset(SET), 0))

// * Returns true if the key was newly added.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_SetAdd(SET, KEY) /* (DS_Set(K) *SET, K KEY) */ \
	(DS_MapTypecheckK(SET, &(KEY)), \
	DS_MapInsertRaw((DS_MapRaw*)(SET), &(KEY), NULL, DS_MapKSize(SET), 0, DS_MapElemSize(SET), DS_MapKOffset(SET), 0))

// * Returns true if the key was found and removed.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_SetRemove(SET, KEY) /* (DS_Set(K) *SET, K KEY) */ \
	(DS_MapTypecheckK(SET, &(KEY)), \
	DS_MapRemoveRaw((DS_MapRaw*)(SET), &(KEY), DS_MapKSize(SET), 0, DS_MapElemSize(SET), DS_MapKOffset(SET), 0))

#define DS_ForSetEach(K, SET, IT) /* (type K, DS_Set(K) *SET, name IT) */ \
	struct DS_Concat(_dummy_, __LINE__) { int i_next; K *elem; }; \
	if ((SET)->count > 0) for (struct DS_Concat(_dummy_, __LINE__) IT = {0}; \
		DS_MapIter((DS_MapRaw*)(SET), &IT.i_next, (void**)&IT.elem, NULL, DS_MapKOffset(SET), 0, DS_MapElemSize(SET)); )

#define DS_SetClear(SET) \
	DS_MapClearRaw((DS_MapRaw*)(SET), DS_MapElemSize(SET))

// * Reset the set to a default state and free its memory if using the heap allocator.
#define DS_SetDeinit(SET) /* (DS_Set(K) *SET) */ \
	DS_MapDeinitRaw((DS_MapRaw*)(SET), DS_MapElemSize(SET))

// * Return true if the key was newly added.
static inline bool DS_MapGetOrAddRaw(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
static bool DS_MapGetOrAddRawEx(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset, uint32_t hash);

// * Returns true if the key was found and removed.
static inline bool DS_MapRemoveRaw(DS_MapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset);

// `dst` must be an uninitialized map
static inline void DS_MapInitCloneRaw(DS_MapRaw* map, DS_MapRaw* src, DS_Allocator* allocator, int elem_size);

// * Returns true if the key was newly added
static inline bool DS_MapInsertRaw(DS_MapRaw* map, const void* key, DS_OUT void* val, int K_size, int V_size, int elem_size, int key_offset, int val_offset);

// * Returns true if the key was found.
static inline bool DS_MapFindRaw(DS_MapRaw* map, const void* key, DS_OUT void* out_val, int K_size, int V_size, int elem_size, int key_offset, int val_offset);

// * Returns the address of the value if the key was found, otherwise NULL.
static inline void* DS_MapFindPtrRaw(DS_MapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset);

static inline bool DS_MapIter(DS_MapRaw* map, int* i, void** out_key, void** out_value, int key_offset, int val_offset, int elem_size) {
	char* elem_base;
	for (;;) {
		if (*i >= map->capacity) return false;

		elem_base = (char*)map->data + (*i) * elem_size;
		if (*(uint32_t*)elem_base == 0) {
			*i = *i + 1;
			continue;
		}

		break;
	}
	*out_key = elem_base + key_offset;
	if (out_value) *out_value = elem_base + val_offset;
	*i = *i + 1;
	return true;
}

// -- Arena interface --------------------------------

DS_API void DS_ArenaInit(DS_Arena* arena, size_t block_size, DS_Allocator* allocator);
DS_API void DS_ArenaDeinit(DS_Arena* arena);

DS_API char* DS_ArenaPush(DS_Arena* arena, size_t size);
DS_API char* DS_ArenaPushZero(DS_Arena* arena, size_t size);
DS_API char* DS_ArenaPushAligned(DS_Arena* arena, size_t size, size_t alignment);

DS_API DS_ArenaMark DS_ArenaGetMark(DS_Arena* arena);
DS_API void DS_ArenaSetMark(DS_Arena* arena, DS_ArenaMark mark);
DS_API void DS_ArenaReset(DS_Arena* arena);

// -- MemScope ----------------------------------------------

// When passing a temporary arena to a function, it's often not immediately clear from the call-site whether the arena parameter
// is for temporary allocations within the function or for allocating caller result data. To make this distinction clear,
// DS_MemScopeNone can be used for passing temporary arenas only.
typedef struct DS_MemScopeNone {
	DS_Arena* temp;
} DS_MemScopeNone;

// DS_MemScope is used for passing a temporary arena and a results arena to a function.
// It can also be used for temporary child scopes that get immediately
// freed using the DS_ScopeBegin / DS_ScopeEnd functions.
typedef struct DS_MemScope {
	DS_Arena* arena;
	DS_MemScopeNone temp;
	DS_ArenaMark reset_arena_to; // Optional field, used by DS_ScopeBegin / DS_ScopeEnd functions.
} DS_MemScope;

static inline DS_MemScope DS_ScopeBegin(DS_MemScope* parent) {
	DS_MemScope new_scope = { parent->temp.temp, parent->arena };

	if (parent->arena != parent->temp.temp) {
		// When working with just one arena, both for results and temporary allocations, the results arena and the temp arena
		// are allowed to be the same.
		// If parent->arena = parent->temp, we must not reset at the end. If we did, then any allocations
		// that should outlive the scope (allocations from parent->arena) would be reset too.

		new_scope.reset_arena_to = parent->temp.temp->mark;
	}

	return new_scope;
}

static inline DS_MemScope DS_ScopeBeginT(DS_MemScopeNone* parent) {
	DS_MemScope new_scope = { parent->temp, parent->temp, parent->temp->mark };
	return new_scope;
}

static inline DS_MemScope DS_ScopeBeginA(DS_Arena* arena) {
	DS_MemScope new_scope = { arena, arena, arena->mark };
	return new_scope;
}

static inline void DS_ScopeEnd(DS_MemScope* scope) {
	if (scope->reset_arena_to.block != NULL) {
		DS_ArenaSetMark(scope->arena, scope->reset_arena_to);
	}
}

// -- Memory allocation --------------------------------

#define DS_MemAlloc(ALLOCATOR, SIZE)                               (ALLOCATOR)->base.AllocatorProc(&(ALLOCATOR)->base,  NULL,          0, (SIZE), DS_DEFAULT_ALLOCATOR_PROC_ALIGNMENT)
#define DS_MemResize(ALLOCATOR, PTR, OLD_SIZE, SIZE)               (ALLOCATOR)->base.AllocatorProc(&(ALLOCATOR)->base, (PTR), (OLD_SIZE), (SIZE), DS_DEFAULT_ALLOCATOR_PROC_ALIGNMENT)
#define DS_MemFree(ALLOCATOR, PTR)                                 (ALLOCATOR)->base.AllocatorProc(&(ALLOCATOR)->base, (PTR),          0,      0, DS_DEFAULT_ALLOCATOR_PROC_ALIGNMENT)
#define DS_MemAllocAligned(ALLOCATOR, SIZE, ALIGN)                 (ALLOCATOR)->base.AllocatorProc(&(ALLOCATOR)->base,  NULL,          0, (SIZE), ALIGN)
#define DS_MemResizeAligned(ALLOCATOR, PTR, OLD_SIZE, SIZE, ALIGN) (ALLOCATOR)->base.AllocatorProc(&(ALLOCATOR)->base, (PTR), (OLD_SIZE), (SIZE), ALIGN)

static inline void* DS_MemClone(DS_Arena* arena, const void* value, int size) { void* p = DS_ArenaPush(arena, size); return memcpy(p, value, size); }
static inline void* DS_MemCloneAligned(DS_Arena* arena, const void* value, int size, int align) { void* p = DS_ArenaPushAligned(arena, size, align); return memcpy(p, value, size); }

// -- Dynamic array --------------------------------

#ifdef __cplusplus
template<class T> struct DS_DynArray {
	DS_Allocator* allocator; T* data; int32_t count; int32_t capacity;
	inline T& operator [](size_t i)       { return DS_ArrBoundsCheck((*this), i), data[i]; }
	inline T operator [](size_t i) const  { return DS_ArrBoundsCheck((*this), i), data[i]; }
};
#define DS_DynArray(T) DS_DynArray<T>
typedef struct { DS_Allocator* allocator; void* data; int32_t count; int32_t capacity; } DS_DynArrayRaw;
#else
#define DS_DynArray(T) struct { DS_Allocator* allocator; T* data; int32_t count; int32_t capacity; }
typedef DS_DynArray(void) DS_DynArrayRaw;
#endif

#define DS_ArrGet(ARR, INDEX)          (DS_ArrBoundsCheck(ARR, INDEX), ((ARR).data)[INDEX])
#define DS_ArrGetPtr(ARR, INDEX)       (DS_ArrBoundsCheck(ARR, INDEX), &((ARR).data)[INDEX])
#define DS_ArrSet(ARR, INDEX, VALUE)   (DS_ArrBoundsCheck(ARR, INDEX), ((ARR).data)[INDEX] = VALUE)
#define DS_ArrElemSize(ARR)            sizeof(*(ARR).data)
#define DS_ArrPeek(ARR)                (DS_ArrBoundsCheck(ARR, (ARR).count - 1), (ARR).data[(ARR).count - 1])
#define DS_ArrPeekPtr(ARR)             (DS_ArrBoundsCheck(ARR, (ARR).count - 1), &(ARR).data[(ARR).count - 1])

#define DS_ArrClone(ARENA, ARR)        DS_ArrCloneRaw(ARENA, (DS_DynArrayRaw*)(ARR), DS_ArrElemSize(*ARR))

#define DS_ArrReserve(ARR, CAPACITY)   DS_ArrReserveRaw((DS_DynArrayRaw*)(ARR), CAPACITY, DS_ArrElemSize(*ARR))

#define DS_ArrInit(ARR, ALLOCATOR)     DS_ArrInitRaw((DS_DynArrayRaw*)(ARR), (ALLOCATOR))

#define DS_ArrPush(ARR, ...) do { \
	DS_ArrReserveRaw((DS_DynArrayRaw*)(ARR), (ARR)->count + 1, DS_ArrElemSize(*(ARR))); \
	(ARR)->data[(ARR)->count++] = __VA_ARGS__; } while (0)

#define DS_ArrPushN(ARR, ELEMS_DATA, ELEMS_COUNT) (DS_ArrTypecheck(ARR, ELEMS_DATA), DS_ArrPushNRaw((DS_DynArrayRaw*)(ARR), ELEMS_DATA, ELEMS_COUNT, DS_ArrElemSize(*ARR)))

#define DS_ArrPushArr(ARR, ELEMS)       (DS_ArrTypecheck(ARR, ELEMS.data), DS_ArrPushNRaw((DS_DynArrayRaw*)(ARR), ELEMS.data, ELEMS.count, DS_ArrElemSize(*ARR)))

#define DS_ArrInsert(ARR, AT, ELEM)     (DS_ArrTypecheck(ARR, &(ELEM)), DS_ArrInsertRaw((DS_DynArrayRaw*)(ARR), AT, &(ELEM), DS_ArrElemSize(*ARR)))

#define DS_ArrInsertN(ARR, AT, ELEMS_DATA, ELEMS_COUNT) (DS_ArrTypecheck(ARR, ELEMS_DATA), DS_ArrInsertNRaw((DS_DynArrayRaw*)(ARR), AT, ELEMS_DATA, ELEMS_COUNT, DS_ArrElemSize(*ARR)))

#define DS_ArrRemove(ARR, INDEX)       DS_ArrRemoveRaw((DS_DynArrayRaw*)(ARR), INDEX, DS_ArrElemSize(*ARR))

#define DS_ArrReverseOrder(ARR)        DS_GeneralArrayReverseOrder((ARR)->data, (ARR)->count, DS_ArrElemSize(*ARR))

#define DS_ArrRemoveN(ARR, INDEX, N)   DS_ArrRemoveNRaw((DS_DynArrayRaw*)(ARR), INDEX, N, DS_ArrElemSize(*ARR))

#define DS_ArrPop(ARR)                 (ARR)->data[--(ARR)->count]

#define DS_ArrResize(ARR, ELEM, LEN)   (DS_ArrTypecheck(ARR, &(ELEM)), DS_ArrResizeRaw((DS_DynArrayRaw*)(ARR), LEN, &(ELEM), DS_ArrElemSize(*ARR)))

#define DS_ArrResizeUndef(ARR, LEN)    DS_ArrResizeRaw((DS_DynArrayRaw*)(ARR), LEN, NULL, DS_ArrElemSize(*ARR))

#define DS_ArrClear(ARR)               (ARR)->count = 0

// Reset the array to a default state and free its memory if using the heap allocator.
#define DS_ArrDeinit(ARR)             DS_ArrDeinitRaw((DS_DynArrayRaw*)(ARR), DS_ArrElemSize(*ARR))

// Array iteration macro.
// Example:
//   DS_DynArray(float) foo;
//   DS_ForArrEach(float, &foo, it) {
//       printf("foo at index %d has the value of %f\n", it.i, it.elem);
//   }
#define DS_ForArrEach(T, ARR, IT) \
	(void)((T*)0 == (ARR)->data); /* Trick the compiler into checking that T is the same as the elem type of ARR */ \
	struct DS_Concat(_dummy_, __LINE__) {int i; T *ptr;}; /* Declaring new struct types in for-loop initializers is not standard C */ \
	for (struct DS_Concat(_dummy_, __LINE__) IT = {0, (ARR)->data}; IT.i < (ARR)->count; IT.i++, IT.ptr++)

DS_API void DS_GeneralArrayReverseOrder(void* data, int count, int elem_size);

DS_API int DS_ArrPushRaw(DS_DynArrayRaw* array, const void* elem, int elem_size);
DS_API int DS_ArrPushNRaw(DS_DynArrayRaw* array, const void* elems, int n, int elem_size);
DS_API void DS_ArrInsertRaw(DS_DynArrayRaw* array, int at, const void* elem, int elem_size);
DS_API void DS_ArrInsertNRaw(DS_DynArrayRaw* array, int at, const void* elems, int n, int elem_size);
DS_API void DS_ArrRemoveRaw(DS_DynArrayRaw* array, int i, int elem_size);
DS_API void DS_ArrRemoveNRaw(DS_DynArrayRaw* array, int i, int n, int elem_size);
DS_API void DS_ArrPopRaw(DS_DynArrayRaw* array, DS_OUT void* out_elem, int elem_size);
DS_API void DS_ArrReserveRaw(DS_DynArrayRaw* array, int capacity, int elem_size);
DS_API void DS_ArrCloneRaw(DS_Arena* arena, DS_DynArrayRaw* array, int elem_size);
DS_API void DS_ArrResizeRaw(DS_DynArrayRaw* array, int count, const void* value, int elem_size); // set value to NULL to not initialize the memory

// -- Bucket Array --------------------------------------------------------------------

// DS_BucketArrayIndex encodes the following struct: { uint32_t bucket_index_plus_one; uint32_t slot_index; }
// This means that the index 0 is always invalid.
typedef uint64_t DS_BucketArrayIndex;

#define DS_EncodeBucketArrayIndex(BUCKET, SLOT) ((uint64_t)((BUCKET) + 1) | ((uint64_t)(SLOT) << 32))
#define DS_BucketFromIndex(INDEX) ((uint32_t)(INDEX) - 1)
#define DS_SlotFromIndex(INDEX)   (uint32_t)((INDEX) >> 32)

// hmm... Maybe we can initialize the bucket array with an option for an inital array allocation.
#define DS_BucketArray(T) struct { \
	DS_Allocator* allocator; \
	struct { T* elems; }* buckets; \
	uint32_t buckets_count; \
	uint32_t buckets_capacity; \
	uint32_t elems_per_bucket; \
	uint32_t last_bucket_end; \
	uint32_t count : 31; \
	uint32_t using_small_ptr_array : 1; }

typedef DS_BucketArray(void) DS_BucketArrayRaw;

#define DS_BkArrInit(ARRAY, ALLOCATOR, ELEMS_PER_BUCKET) \
	DS_BucketArrayInitRaw((DS_BucketArrayRaw*)(ARRAY), (ALLOCATOR), (ELEMS_PER_BUCKET));

#define DS_BkArrInitUsingSmallPtrArray(ARRAY, ALLOCATOR, ELEMS_PER_BUCKET, SMALL_PTR_ARRAY, SMALL_PTR_ARRAY_COUNT) \
	DS_BucketArrayInitUsingSmallArrayRaw((DS_BucketArrayRaw*)(ARRAY), (ALLOCATOR), (ELEMS_PER_BUCKET), (SMALL_PTR_ARRAY), (SMALL_PTR_ARRAY_COUNT));

#define DS_BkArrPush(ARRAY, ...) do { \
	DS_BucketArrayPushRaw((DS_BucketArrayRaw*)(ARRAY), DS_BucketElemSize(ARRAY)); \
	(ARRAY)->buckets[(ARRAY)->buckets_count - 1].elems[(ARRAY)->last_bucket_end - 1] = (__VA_ARGS__); \
	} while (0)

#define DS_BkArrPushUndef(ARRAY) DS_BucketArrayPushRaw((DS_BucketArrayRaw*)(ARRAY), DS_BucketElemSize(ARRAY))
#define DS_BkArrPushZero(ARRAY)  memset(DS_BucketArrayPushRaw((DS_BucketArrayRaw*)(ARRAY), DS_BucketElemSize(ARRAY)), 0, DS_BucketElemSize(ARRAY))

#define DS_BkArrFirst() DS_EncodeBucketArrayIndex(0, 0)
#define DS_BkArrLast(ARRAY) DS_EncodeBucketArrayIndex((ARRAY)->buckets_count - 1, (ARRAY)->last_bucket_end - 1)

#define DS_BkArrNext(ARRAY, INDEX) \
	(DS_SlotFromIndex(INDEX) == (ARRAY)->elems_per_bucket - 1) ? \
		DS_EncodeBucketArrayIndex(DS_BucketFromIndex(INDEX) + 1, 0) : \
		DS_EncodeBucketArrayIndex(DS_BucketFromIndex(INDEX), DS_SlotFromIndex(INDEX) + 1)

#define DS_BkArrEnd(ARRAY) DS_BkArrNext(ARRAY, DS_BkArrLast(ARRAY))

#define DS_BkArrPushN(ARRAY, ELEMS_DATA, ELEMS_COUNT)  DS_BucketArrayPushNRaw((DS_BucketArrayRaw*)(ARRAY), (ELEMS_DATA), (uint32_t)(ELEMS_COUNT), DS_BucketElemSize(ARRAY), (uint32_t)DS_BucketNextPtrOffset(ARRAY))

#define DS_BkArrEach(ARRAY, IT) DS_BucketArrayIndex IT = DS_BkArrFirst(), IT##_end = DS_BkArrEnd(ARRAY); IT != IT##_end; IT = DS_BkArrNext(ARRAY, IT)

#define DS_BkArrGet(ARRAY, INDEX) (&(ARRAY)->buckets[DS_BucketFromIndex(INDEX)].elems[DS_SlotFromIndex(INDEX)])

// Read N elems into the memory address at DST starting from the index INDEX and move the index forward by N.
#define DS_BkArrReadN(ARRAY, DST, N, INDEX)  DS_BucketArrayReadNRaw((DS_BucketArrayRaw*)(ARRAY), (DST), (uint32_t)(N), (INDEX), DS_BucketElemSize(ARRAY), (uint32_t)DS_BucketNextPtrOffset(ARRAY))

#define DS_BkArrDeinit(ARRAY) DS_BucketArrayDeinitRaw((DS_BucketArrayRaw*)(ARRAY), DS_BucketNextPtrOffset(ARRAY))

// #define DS_BucketArraySetViewToArray(ARRAY, ELEMS_DATA, ELEMS_COUNT) DS_BucketArraySetViewToArrayRaw((DS_BucketArrayRaw*)(ARRAY), (ELEMS_DATA), (uint32_t)(ELEMS_COUNT))

// -- C++ extras -----------------------------------

#ifdef __cplusplus
template<class T, int32_t COUNT>
struct DS_Array { // fixed-length array
	T data[COUNT];
	inline T& operator [](size_t i)       { return data[i]; }
	inline T operator [](size_t i) const  { return data[i]; }
};

template<class T>
struct DS_ArrayView {
	T* data; int32_t count;
	DS_ArrayView() : data(0), count(0) {}
	DS_ArrayView(T* _data, int32_t _count) : data(_data), count(_count) {}
	DS_ArrayView(const DS_DynArray<T>& other) : data(other.data), count(other.count) {}
	template<int32_t COUNT> DS_ArrayView(DS_Array<T, COUNT>& other) : data(&other.data[0]), count(COUNT) {}
	inline T& operator [](size_t i)       { return DS_ArrBoundsCheck((*this), i), data[i]; }
	inline T operator [](size_t i) const  { return DS_ArrBoundsCheck((*this), i), data[i]; }
};

#define DS_Dup(ARENA, ...) DS_Clone__(ARENA, __VA_ARGS__)

#endif

// -- IMPLEMENTATION ------------------------------------------------------------------

/*static inline void DS_BucketArraySetViewToArrayRaw(DS_BucketArrayRaw* array, const void* elems_data, uint32_t elems_count) {
	DS_BucketArrayRaw result = {0};
	result.last_bucket_end = (int)elems_count;
	result.count = (int)elems_count;
	*(const void**)&result.first_bucket = elems_data;
	*(const void**)&result.last_bucket = elems_data;
	*array = result;
}*/

static inline void DS_BucketArrayInitUsingSmallArrayRaw(DS_BucketArrayRaw* array, DS_Allocator* allocator, int elems_per_bucket, void** small_ptr_array, int small_ptr_array_count) {
	DS_BucketArrayRaw result = {0};
	result.allocator = allocator;
	result.elems_per_bucket = elems_per_bucket;
	result.last_bucket_end = elems_per_bucket;
	*(void**)&result.buckets = small_ptr_array;
	result.buckets_count = small_ptr_array_count;
	result.using_small_ptr_array = 1;
	*array = result;
}

static inline void DS_BucketArrayInitRaw(DS_BucketArrayRaw* array, DS_Allocator* allocator, int elems_per_bucket) {
	DS_BucketArrayRaw result = {0};
	result.allocator = allocator;
	result.elems_per_bucket = elems_per_bucket;
	result.last_bucket_end = elems_per_bucket;
	*array = result;
}

static inline void DS_BucketArrayDeinitRaw(DS_BucketArrayRaw* array) {
	DS_ProfEnter();
	
	for (uint32_t i = 0; i < array->buckets_count; i++) {
		DS_MemFree(array->allocator, array->buckets[i].elems);
	}
	
	if (!array->using_small_ptr_array) {
		DS_MemFree(array->allocator, array->buckets);
	}

	DS_DebugFillGarbage(array, sizeof(*array));
	DS_ProfExit();
}

static void DS_BucketArrayMoveToNextBucket(DS_BucketArrayRaw* array, uint32_t elem_size) {
	DS_ASSERT(array->elems_per_bucket > 0); // did you remember to call DS_BkArrInit?

	if (array->buckets_count == array->buckets_capacity) {
		uint32_t new_cap = array->buckets_capacity == 0 ? 8 : array->buckets_capacity * 2;
		
		if (array->using_small_ptr_array) {
			void* new_buckets = DS_MemAlloc(array->allocator, new_cap * sizeof(void*));
			memcpy(new_buckets, array->buckets, array->buckets_capacity * sizeof(void*));
			*(void**)&array->buckets = new_buckets;
			array->using_small_ptr_array = 0;
		}
		else {
			*(void**)&array->buckets = DS_MemResize(array->allocator, array->buckets, array->buckets_capacity * sizeof(void*), new_cap * sizeof(void*));
		}
		
		array->buckets_capacity = new_cap;
	}
	
	array->buckets[array->buckets_count].elems = DS_MemAlloc(array->allocator, elem_size * array->elems_per_bucket);
	array->buckets_count += 1;
	array->last_bucket_end = 0;
}

static void DS_BucketArrayReadNRaw(const DS_BucketArrayRaw* array, void* dst, uint32_t elems_count, DS_BucketArrayIndex* index, uint32_t elem_size, uint32_t next_bucket_ptr_offset)
{
	while (elems_count > 0) {
		uint32_t bucket = DS_BucketFromIndex(*index);
		uint32_t slot = DS_SlotFromIndex(*index);
		uint32_t elems_in_bucket = bucket == array->buckets_count - 1 ? array->last_bucket_end : array->elems_per_bucket;
		uint32_t slots_left_in_bucket = elems_in_bucket - slot;
		
		if (elems_count >= slots_left_in_bucket) { // go past this bucket
			uint32_t bytes_to_add_now = slots_left_in_bucket * elem_size;

			memcpy(dst, (char*)array->buckets[bucket].elems + slot * elem_size, bytes_to_add_now);
			dst = (char*)dst + bytes_to_add_now;
			elems_count -= slots_left_in_bucket;

			*index = DS_EncodeBucketArrayIndex(bucket + 1, 0);
		} else {
			uint32_t bytes_to_add_now = elems_count * elem_size;
			memcpy(dst, (char*)array->buckets[bucket].elems + slot * elem_size, bytes_to_add_now);
			
			*index = DS_EncodeBucketArrayIndex(bucket, slot + elems_count);
			break;
		}
	}
}

static void DS_BucketArrayPushNRaw(DS_BucketArrayRaw* array, const void* elems_data, uint32_t elems_count, uint32_t elem_size) {
	DS_ProfEnter();

	array->count += elems_count;
	while (elems_count > 0) {
		if (array->last_bucket_end == array->elems_per_bucket) {
			DS_BucketArrayMoveToNextBucket(array, elem_size);
		}
		
		void* bucket = array->buckets[array->buckets_count - 1].elems;
		uint32_t slots_left_in_bucket = array->elems_per_bucket - array->last_bucket_end;

		if (elems_count >= slots_left_in_bucket) {
			uint32_t bytes_to_add_now = slots_left_in_bucket * elem_size;

			void* dst = (char*)bucket + elem_size * array->last_bucket_end;
			memcpy(dst, elems_data, (size_t)bytes_to_add_now);

			elems_data = (char*)elems_data + bytes_to_add_now;
			elems_count -= slots_left_in_bucket;
			array->last_bucket_end += slots_left_in_bucket;
		}
		else {
			uint32_t bytes_to_add_now = elems_count * elem_size;

			void* dst = (char*)bucket + elem_size * array->last_bucket_end;
			memcpy(dst, elems_data, (size_t)bytes_to_add_now);
			
			array->last_bucket_end += elems_count;
			break;
		}
	}

	DS_ProfExit();
}

static inline void* DS_BucketArrayPushRaw(DS_BucketArrayRaw* array, uint32_t elem_size) {
	DS_ProfEnter();

	if (array->last_bucket_end == array->elems_per_bucket) {
		DS_BucketArrayMoveToNextBucket(array, elem_size);
	}

	void* bucket = array->buckets[array->buckets_count - 1].elems;
	uint32_t slot_idx = array->last_bucket_end++;
	void* result = (char*)bucket + elem_size * slot_idx;

	array->count++;
	DS_ProfExit();
	return result;
}

DS_API void DS_ArrCloneRaw(DS_Arena* arena, DS_DynArrayRaw* array, int elem_size) {
	array->data = DS_MemClone(arena, array->data, array->count * elem_size);
}

DS_API void DS_ArrReserveRaw(DS_DynArrayRaw* array, int capacity, int elem_size) {
	DS_ProfEnter();

	DS_ASSERT(array->allocator != NULL); // Have you called DS_ArrInit?

	int new_capacity = array->capacity;
	while (capacity > new_capacity) {
		new_capacity = new_capacity == 0 ? 8 : new_capacity * 2;
	}

	if (new_capacity != array->capacity) {
		array->data = DS_MemResize(array->allocator, array->data, array->capacity * elem_size, new_capacity * elem_size);
		array->capacity = new_capacity;
	}

	DS_ProfExit();
}

DS_API void DS_ArrRemoveRaw(DS_DynArrayRaw* array, int i, int elem_size) { DS_ArrRemoveNRaw(array, i, 1, elem_size); }

DS_API void DS_ArrRemoveNRaw(DS_DynArrayRaw* array, int i, int n, int elem_size) {
	DS_ProfEnter();
	DS_ASSERT(i + n <= array->count);

	char* dst = (char*)array->data + i * elem_size;
	char* src = dst + elem_size * n;
	memmove(dst, src, ((char*)array->data + array->count * elem_size) - src);

	array->count -= n;
	DS_ProfExit();
}

DS_API void DS_ArrInsertRaw(DS_DynArrayRaw* array, int at, const void* elem, int elem_size) {
	DS_ProfEnter();
	DS_ArrInsertNRaw(array, at, elem, 1, elem_size);
	DS_ProfExit();
}

DS_API void DS_ArrInsertNRaw(DS_DynArrayRaw* array, int at, const void* elems, int n, int elem_size) {
	DS_ProfEnter();
	DS_ASSERT(at <= array->count);
	DS_ArrReserveRaw(array, array->count + n, elem_size);

	// Move existing elements forward
	char* offset = (char*)array->data + at * elem_size;
	memmove(offset + n * elem_size, offset, (array->count - at) * elem_size);

	memcpy(offset, elems, n * elem_size);
	array->count += n;
	DS_ProfExit();
}

DS_API int DS_ArrPushNRaw(DS_DynArrayRaw* array, const void* elems, int n, int elem_size) {
	DS_ProfEnter();
	DS_ArrReserveRaw(array, array->count + n, elem_size);

	memcpy((char*)array->data + elem_size * array->count, elems, elem_size * n);

	int result = array->count;
	array->count += n;
	DS_ProfExit();
	return result;
}

DS_API void DS_ArrResizeRaw(DS_DynArrayRaw* array, int count, DS_OUT const void* value, int elem_size) {
	DS_ProfEnter();
	DS_ArrReserveRaw(array, count, elem_size);

	if (value) {
		for (int i = array->count; i < count; i++) {
			memcpy((char*)array->data + i * elem_size, value, elem_size);
		}
	}

	array->count = count;
	DS_ProfExit();
}

DS_API void DS_ArrInitRaw(DS_DynArrayRaw* array, DS_Allocator* allocator) {
	DS_DynArrayRaw empty = {0};
	*array = empty;
	array->allocator = allocator;
}

DS_API void DS_ArrDeinitRaw(DS_DynArrayRaw* array, int elem_size) {
	DS_DebugFillGarbage(array->data, array->capacity * elem_size);
	DS_MemFree(array->allocator, array->data);
	DS_DebugFillGarbage(array, sizeof(*array));
}

DS_API int DS_ArrPushRaw(DS_DynArrayRaw* array, const void* elem, int elem_size) {
	DS_ProfEnter();
	DS_ArrReserveRaw(array, array->count + 1, elem_size);

	memcpy((char*)array->data + array->count * elem_size, elem, elem_size);
	DS_ProfExit();
	return array->count++;
}

DS_API void DS_GeneralArrayReverseOrder(void* data, int count, int elem_size) {
	int i = 0;
	int j = (count - 1) * elem_size;

	char temp[DS_MAX_ELEM_SIZE];
	DS_ASSERT(DS_MAX_ELEM_SIZE >= elem_size);

	while (i < j) {
		memcpy(temp, (char*)data + i, elem_size);
		memcpy((char*)data + i, (char*)data + j, elem_size);
		memcpy((char*)data + j, temp, elem_size);
		i += elem_size;
		j -= elem_size;
	}
}

DS_API void DS_ArrPopRaw(DS_DynArrayRaw* array, DS_OUT void* out_elem, int elem_size) {
	DS_ProfEnter();
	DS_ASSERT(array->count >= 1);
	array->count--;
	if (out_elem) {
		memcpy(out_elem, (char*)array->data + array->count * elem_size, elem_size);
	}
	DS_ProfExit();
}

static inline void DS_MapInitRaw(DS_MapRaw* map, DS_Allocator* allocator) {
	DS_MapRaw result = {allocator};
	*map = result;
}

static inline void DS_MapClearRaw(DS_MapRaw* map, int elem_size) {
	memset(map->data, 0, map->capacity * elem_size);
	map->count = 0;
}

static inline void DS_MapDeinitRaw(DS_MapRaw* map, int elem_size) {
	DS_ProfEnter();
	DS_DebugFillGarbage(map->data, map->capacity * elem_size);
	DS_MemFree(map->allocator, map->data);
	DS_MapRaw empty = {0};
	*map = empty;
	DS_ProfExit();
}

#if defined(_MSC_VER)
#include <stdlib.h>
#define DS_ROTL32(x, y) _rotl(x, y)
#else
#define DS_ROTL32(x, y) ((x << y) | (x >> (32 - y)))
#endif

// See https://github.com/aappleby/smhasher/blob/master/src/MurmurHash2.cpp
static uint64_t DS_MurmurHash64A(const void* key, size_t size, uint64_t seed) {
	DS_ProfEnter();
	const uint64_t m = 0xc6a4a7935bd1e995LLU;
	const int r = 47;

	uint64_t h = seed ^ (size * m);

	const uint64_t* data = (const uint64_t*)key;
	const uint64_t* end = data + (size / 8);

	while (data != end) {
		uint64_t k = *data++;
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;
	}

	const unsigned char* data2 = (const unsigned char*)data;

	switch (size & 7) {
	case 7: h ^= ((uint64_t)data2[6]) << 48;
	case 6: h ^= ((uint64_t)data2[5]) << 40;
	case 5: h ^= ((uint64_t)data2[4]) << 32;
	case 4: h ^= ((uint64_t)data2[3]) << 24;
	case 3: h ^= ((uint64_t)data2[2]) << 16;
	case 2: h ^= ((uint64_t)data2[1]) << 8;
	case 1: h ^= ((uint64_t)data2[0]);
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	DS_ProfExit();
	return h;
}

// See https://github.com/aappleby/smhasher/blob/master/src/DS_MurmurHash3.cpp
DS_API uint32_t DS_MurmurHash3(const void* key, size_t size, uint32_t seed) {
	DS_ProfEnter();
	const uint8_t* data = (const uint8_t*)key;
	const intptr_t nblocks = size / 4;

	uint32_t h1 = seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	// body

	const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
	for (intptr_t i = -nblocks; i; i++) {
		uint32_t k1 = blocks[i];

		k1 *= c1;
		k1 = DS_ROTL32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = DS_ROTL32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	// tail

	const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

	uint32_t k1 = 0;
	switch (size & 3) {
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 *= c1; k1 = DS_ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
	};

	// finalization

	h1 ^= size;
	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;
	DS_ProfExit();
	return h1;
}

static inline void* DS_MapFindPtrRaw(DS_MapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset) {
	if (map->capacity == 0) return NULL;
	DS_ProfEnter();

	// TODO: it'd be nice to use an integer hash function for small keys... just not sure how to implement that in an ergonomic way.
	// Maybe just ask for the hash?

	uint32_t hash = DS_MurmurHash3(key, K_size, 989898);
	if (hash == 0) hash = 1;

	uint32_t mask = (uint32_t)map->capacity - 1;
	uint32_t index = hash & mask;

	void* found = NULL;
	for (;;) {
		char* elem = (char*)map->data + index * elem_size;
		uint32_t elem_hash = *(uint32_t*)elem;
		if (elem_hash == 0) break;

		if (hash == elem_hash && memcmp(key, elem + key_offset, K_size) == 0) {
			found = elem + val_offset;
			break;
		}

		index = (index + 1) & mask;
	}

	DS_ProfExit();
	return found;
}

static inline bool DS_MapFindRaw(DS_MapRaw* map, const void* key, DS_OUT void* out_val, int K_size, int V_size, int elem_size, int key_offset, int val_offset) {
	DS_ProfEnter();
	void* ptr = DS_MapFindPtrRaw(map, key, K_size, V_size, elem_size, key_offset, val_offset);
	if (ptr) {
		if (out_val) memcpy(out_val, ptr, V_size);
	}
	DS_ProfExit();
	return ptr != NULL;
}

static inline bool DS_MapGetOrAddRaw(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset) {
	uint32_t hash = DS_MurmurHash3((char*)key, K_size, 989898);
	if (hash == 0) hash = 1;
	bool result = DS_MapGetOrAddRawEx(map, key, out_val_ptr, K_size, V_size, elem_size, key_offset, val_offset, hash);
	return result;
}

static bool DS_MapGetOrAddRawEx(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset, uint32_t hash) {
	DS_ProfEnter();
	DS_ASSERT(map->allocator != NULL); // Have you called DS_MapInit?

	if (100 * (map->count + 1) > 70 * map->capacity) {
		// Grow the map

		char* old_data = (char*)map->data;
		int old_capacity = map->capacity;

		map->capacity = old_capacity == 0 ? 8 : old_capacity * 2;
		map->count = 0;

		void* new_data = DS_MemAlloc(map->allocator, map->capacity * elem_size);

		memcpy(&map->data, &new_data, sizeof(void*));
		memset(map->data, 0, map->capacity * elem_size); // set hash values to 0

		for (int i = 0; i < old_capacity; i++) {
			char* elem = old_data + elem_size * i;
			uint32_t elem_hash = *(uint32_t*)elem;
			void* elem_key = elem + key_offset;
			void* elem_val = elem + val_offset;

			if (elem_hash != 0) {
				void* new_val_ptr;
				DS_MapGetOrAddRawEx(map, elem_key, &new_val_ptr, K_size, V_size, elem_size, key_offset, val_offset, elem_hash);
				memcpy(new_val_ptr, elem_val, V_size);
			}
		}

		DS_DebugFillGarbage(old_data, old_capacity * elem_size);
		DS_MemFree(map->allocator, old_data);
	}

	uint32_t mask = (uint32_t)map->capacity - 1;
	uint32_t idx = hash & mask;

	bool added_new = false;

	for (;;) {
		char* elem_base = (char*)map->data + idx * elem_size;

		uint32_t* elem_hash = (uint32_t*)elem_base;
		char* elem_key = elem_base + key_offset;
		char* elem_val = elem_base + val_offset;

		if (*elem_hash == 0) {
			// We found an empty slot
			memcpy(elem_key, key, K_size);
			*elem_hash = hash;

			if (out_val_ptr) *out_val_ptr = elem_val;
			map->count++;
			added_new = true;
			break;
		}

		if (hash == *elem_hash && memcmp(key, elem_key, K_size) == 0) {
			// This key already exists
			if (out_val_ptr) *out_val_ptr = elem_val;
			break;
		}

		idx = (idx + 1) & mask;
	}

	DS_ProfExit();
	return added_new;
}

static inline bool DS_MapRemoveRaw(DS_MapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset) {
	if (map->capacity == 0) return false;
	DS_ProfEnter();

	uint32_t hash = DS_MurmurHash3((char*)key, K_size, 989898);
	if (hash == 0) hash = 1;

	uint32_t mask = (uint32_t)map->capacity - 1;
	uint32_t index = hash & mask;

	bool ok = true;

	char temp[DS_MAX_ELEM_SIZE];
	DS_ASSERT(DS_MAX_ELEM_SIZE >= elem_size);

	for (;;) {
		char* elem_base = (char*)map->data + index * elem_size;
		uint32_t elem_hash = *(uint32_t*)elem_base;

		if (elem_hash == 0) {
			// Empty slot, the key does not exist in the map
			ok = false;
			break;
		}

		if (hash == elem_hash && memcmp(key, elem_base + key_offset, K_size) == 0) {
			// Remove element
			memset(elem_base, 0, elem_size);
			map->count--;

			// backwards-shift deletion.
			// First remove all elements directly after this element from the map, then add them back to the map, starting from the first one to the right.
			for (;;) {
				index = (index + 1) & mask;

				char* shifting_elem_base = (char*)map->data + index * elem_size;
				uint32_t shifting_elem_hash = *(uint32_t*)shifting_elem_base;
				if (shifting_elem_hash == 0) break;

				memcpy(temp, shifting_elem_base, elem_size);
				memset(shifting_elem_base, 0, elem_size);
				map->count--;

				void* new_val_ptr;
				DS_MapGetOrAddRawEx(map, temp + key_offset, &new_val_ptr, K_size, V_size, elem_size, key_offset, val_offset, shifting_elem_hash);
				memcpy(new_val_ptr, temp + val_offset, V_size);
			}

			break;
		}

		index = (index + 1) & mask;
	}

	DS_ProfExit();
	return ok;
}

static inline void DS_MapInitCloneRaw(DS_MapRaw* map, DS_MapRaw* src, DS_Allocator* allocator, int elem_size) {
	*map = *src;
	map->allocator = allocator;
	*(void**)&map->data = DS_MemAlloc(allocator, src->capacity * elem_size);
	memcpy(map->data, src->data, src->capacity * elem_size);
}

static inline bool DS_MapInsertRaw(DS_MapRaw* map, const void* key, DS_OUT void* val,
	int K_size, int V_size, int elem_size, int key_offset, int val_offset)
{
	DS_ProfEnter();
	void* val_ptr;
	bool added = DS_MapGetOrAddRaw(map, key, &val_ptr, K_size, V_size, elem_size, key_offset, val_offset);
	memcpy(val_ptr, val, V_size);
	DS_ProfExit();
	return added;
}

static void* DS_ArenaAllocatorProc(DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	char* data = DS_ArenaPushAligned((DS_Arena*)allocator, (int)size, (int)align); // TODO: use size_t for arenas instead of int
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

DS_API void DS_ArenaInit(DS_Arena* arena, size_t block_size, DS_Allocator* allocator) {
	memset(arena, 0, sizeof(*arena));
	arena->base.AllocatorProc = DS_ArenaAllocatorProc;
	arena->block_size = block_size;
	arena->allocator = allocator;
}

DS_API void DS_ArenaDeinit(DS_Arena* arena) {
	for (DS_ArenaBlockHeader* block = arena->first_block; block;) {
		DS_ArenaBlockHeader* next = block->next;
		DS_MemFree(arena->allocator, block);
		block = next;
	}
	DS_DebugFillGarbage(arena, sizeof(DS_Arena));
}

DS_API char* DS_ArenaPush(DS_Arena* arena, size_t size) {
	return DS_ArenaPushAligned(arena, size, DS_DEFAULT_ARENA_PUSH_ALIGNMENT);
}

DS_API char* DS_ArenaPushZero(DS_Arena* arena, size_t size) {
	char* ptr = DS_ArenaPushAligned(arena, size, DS_DEFAULT_ARENA_PUSH_ALIGNMENT);
	memset(ptr, 0, size);
	return ptr;
}

DS_API char* DS_ArenaPushAligned(DS_Arena* arena, size_t size, size_t alignment) {
	DS_ProfEnter();

	bool alignment_is_power_of_2 = ((alignment) & ((alignment)-1)) == 0;
	DS_ASSERT(alignment != 0 && alignment_is_power_of_2);
	DS_ASSERT(alignment <= DS_ARENA_BLOCK_ALIGNMENT);

	DS_ArenaBlockHeader* curr_block = arena->mark.block; // may be NULL
	void* curr_ptr = arena->mark.ptr;

	char* result_address = (char*)DS_AlignUpPow2((intptr_t)curr_ptr, alignment);
	intptr_t remaining_space = curr_block ? curr_block->size_including_header - ((intptr_t)result_address - (intptr_t)curr_block) : 0;

	if ((intptr_t)size > remaining_space) { // We need a new block!
		intptr_t result_offset = DS_AlignUpPow2(sizeof(DS_ArenaBlockHeader), alignment);
		intptr_t new_block_size = result_offset + size;
		if ((intptr_t)arena->block_size > new_block_size) new_block_size = arena->block_size;

		DS_ArenaBlockHeader* new_block = NULL;
		DS_ArenaBlockHeader* next_block = NULL;

		// If there is a block at the end of the list that we have used previously, but aren't using anymore, then try to start using that one.
		if (curr_block && curr_block->next) {
			next_block = curr_block->next;

			intptr_t next_block_remaining_space = next_block->size_including_header - result_offset;
			if ((intptr_t)size <= next_block_remaining_space) {
				new_block = next_block; // Next block has enough space, let's use it!
			}
		}

		// Otherwise, insert a new block.
		if (new_block == NULL) {
			new_block = (DS_ArenaBlockHeader*)DS_MemAllocAligned(arena->allocator, new_block_size, DS_ARENA_BLOCK_ALIGNMENT);
			DS_ASSERT(((uintptr_t)new_block & (DS_ARENA_BLOCK_ALIGNMENT - 1)) == 0); // make sure that the alignment is correct
	
			new_block->size_including_header = new_block_size;
			new_block->next = next_block;
			arena->total_mem_reserved += new_block_size;

			if (curr_block) curr_block->next = new_block;
			else arena->first_block = new_block;
		}

		arena->mark.block = new_block;
		result_address = (char*)new_block + result_offset;
	}

	arena->mark.ptr = result_address + size;
	return result_address;
	DS_ProfExit();
}

DS_API void DS_ArenaReset(DS_Arena* arena) {
	DS_ProfEnter();
	if (arena->first_block) {
		// Free all blocks after the first block
		for (DS_ArenaBlockHeader* block = arena->first_block->next; block;) {
			DS_ArenaBlockHeader* next = block->next;
			arena->total_mem_reserved -= block->size_including_header;
			DS_MemFree(arena->allocator, block);
			block = next;
		}
		arena->first_block->next = NULL;

		// Free the first block too if it's larger than the regular block size
		if (arena->first_block->size_including_header > arena->block_size) {
			arena->total_mem_reserved -= arena->first_block->size_including_header;
			DS_MemFree(arena->allocator, arena->first_block);
			arena->first_block = NULL;
		}
	}
	arena->mark.block = arena->first_block;
	arena->mark.ptr = (char*)arena->first_block + sizeof(DS_ArenaBlockHeader);
	DS_ProfExit();
}

DS_API DS_ArenaMark DS_ArenaGetMark(DS_Arena* arena) {
	return arena->mark;
}

DS_API void DS_ArenaSetMark(DS_Arena* arena, DS_ArenaMark mark) {
	DS_ProfEnter();
	if (mark.block == NULL) {
		arena->mark.block = arena->first_block;
		arena->mark.ptr = (char*)arena->first_block + sizeof(DS_ArenaBlockHeader);
	}
	else {
		arena->mark = mark;
	}
	DS_ProfExit();
}

//#ifndef DS_REALLOC_OVERRIDE
#if 0

// #define UNICODE // TODO: remove
// #include <Windows.h> // TODO: remove
#include <stdlib.h>

void* DS_Realloc_Impl(void* old_ptr, int new_size, int new_alignment) {
	//static bool inside_allocation_validation = false;
	//static DS_Set(void*) DS_alive_allocations = {.arena = DS_HEAP};
	//
	//if (old_ptr != NULL && !inside_allocation_validation) {
	//	inside_allocation_validation = true;
	//
	//	bool was_alive = DS_SetRemove(&DS_alive_allocations, &old_ptr);
	//	DS_ASSERT(was_alive);
	//	
	//	inside_allocation_validation = false;
	//}

	// NEXT STEP TODO: allocation map / validation!

	// static char buffer[64*64*64];
	// static size_t buffer_offset = 0;
	// if (buffer_offset > sizeof(buffer)) __debugbreak();
	// void *result = buffer + buffer_offset;
	// buffer_offset += new_size;
	// buffer_offset = DS_AlignUpPow2_(buffer_offset, 8);

#if 0 // debug
#define DS_AlignUpPow2_(x, p) (((x) + (p) - 1) & ~((p) - 1)) // e.g. (x=30, p=16) -> 32
	void* result = NULL;
	if (new_size > 0) {
		new_size = DS_AlignUpPow2_(new_size, 8); // lets do this just to make sure the resulting pointer will be aligned well
#if 1
		// protect after
		char* pages = VirtualAlloc(NULL, new_size + 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		uint32_t old_protect;
		BOOL ok = VirtualProtect(pages + DS_AlignUpPow2_(new_size, 4096), 4096, PAGE_NOACCESS, &old_protect);
		DS_ASSERT(ok == 1 && old_protect == PAGE_READWRITE);
		result = pages + (DS_AlignUpPow2_(new_size, 4096) - new_size);
#else
		// protect before
		char* pages = VirtualAlloc(NULL, new_size + 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		uint32_t old_protect;
		BOOL ok = VirtualProtect(pages, 4096, PAGE_NOACCESS, &old_protect);
		DS_ASSERT(ok == 1 && old_protect == PAGE_READWRITE);
		result = pages + 4096;
#endif
	}
#endif

	void* result = _aligned_realloc(old_ptr, new_size, new_alignment);

	//if (result != NULL && !inside_allocation_validation) {
	//	DS_ASSERT(result != NULL);
	//	inside_allocation_validation = true;
	//
	//	bool newly_added = DS_SetAdd(&DS_alive_allocations, &result);
	//	DS_ASSERT(newly_added);
	//
	//	inside_allocation_validation = false;
	//}

	return result;
}
#endif

#endif // FIRE_DS_INCLUDED
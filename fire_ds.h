// fire_ds.h - basic data structures
//
// Author: Eero Mutka
// Version: 0
// Date: 25 March, 2024
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//

#ifndef DS_INCLUDED
#define DS_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>  // memcpy, memmove, memset, memcmp, strlen

#ifndef DS_PROFILER_MACROS_OVERRIDE
// Function-level profiler scope. A single function may only have one of these, and it should span the entire function.
#define DS_ProfEnter()
#define DS_ProfExit()
#endif

#ifndef DS_CHECK_OVERRIDE
#include <assert.h>
#define DS_CHECK(x) assert(x)
#endif

#ifndef DS_API
#define DS_API static
#endif

// DS_OUT is to mark output parameters; you may pass NULL to these to ignore them.
#ifndef DS_OUT
#define DS_OUT
#endif

#ifndef DS_MAX_MAP_SLOT_SIZE
#define DS_MAX_MAP_SLOT_SIZE 4096
#endif

#define DS_DEFAULT_ALIGNMENT sizeof(void*)
#define DS_MAX_ALIGNMENT     sizeof(void*[2]) // this should be the alignment of the largest SIMD type.

#ifdef DS_USE_CUSTOM_REALLOC
char* DS_Realloc_Impl(char* old_ptr, int new_size, int new_alignment);
#else
#include <stdlib.h>
static char* DS_Realloc_Impl(char* old_ptr, int new_size, int new_alignment) {
	void* result = _aligned_realloc(old_ptr, new_size, new_alignment);
	return (char*)result;
}
#endif

// ----------------------------------------------------------

typedef struct DS_DefaultArenaBlockHeader {
	int size_including_header;
	struct DS_DefaultArenaBlockHeader* next; // may be NULL
} DS_DefaultArenaBlockHeader;

typedef struct DS_DefaultArenaMark {
	DS_DefaultArenaBlockHeader* block; // If the arena has no blocks allocated yet, then we mark the beginning of the arena by setting this member to NULL.
	char* ptr;
} DS_DefaultArenaMark;

typedef struct DS_DefaultArena {
	int block_size;
	DS_DefaultArenaBlockHeader* first_block; // may be NULL
	DS_DefaultArenaMark mark;
} DS_DefaultArena;

#ifdef DS_CUSTOM_ARENA
// If you want to implement your own custom arena, you must provide definitions for the following:
// - DS_Arena
// - DS_ArenaMark
// - void DS_ArenaInit_Custom(DS_Arena *arena, int block_size)
// - void DS_ArenaDeinit_Custom(DS_Arena *arena)
// - char *DS_ArenaPush_Custom(DS_Arena *arena, int size, int alignment)
// - DS_ArenaMark DS_ArenaGetMark_Custom(DS_Arena *arena)
// - void DS_ArenaSetMark_Custom(DS_Arena *arena, DS_ArenaMark mark)
// - void DS_ArenaReset_Custom(DS_Arena *arena)
#else
typedef DS_DefaultArena DS_Arena;
typedef DS_DefaultArenaMark DS_ArenaMark;
#endif

// -------------------------------------------------------------------

// -- internal helpers -----------

#define DS_Concat_(a, b) a ## b
#define DS_Concat(a, b) DS_Concat_(a, b)

#ifdef __cplusplus
#define DS_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#else
#define DS_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#endif

#define DS_USE_HEAP (DS_Arena*)(-1)

// DS_ArenaOrHeap is used to mark arena parameters where you may pass DS_USE_HEAP.
// When using DS_USE_HEAP, you must also remember call the appropriate deinitialization function that frees the memory.
typedef DS_Arena DS_ArenaOrHeap;

// Results in a compile-error if `elem` does not match the array's element type
#define DS_VecTypecheck(array, elem) (void)((array)->data == elem)

#define DS_MapTypecheckK(MAP, PTR) ((PTR) == &(MAP)->data->key)
#define DS_MapTypecheckV(MAP, PTR) ((PTR) == &(MAP)->data->value)
#define DS_MapKSize(MAP) sizeof((MAP)->data->key)
#define DS_MapVSize(MAP) sizeof((MAP)->data->value)
#define DS_MapElemSize(MAP) sizeof(*(MAP)->data)
#define DS_MapKOffset(MAP) (int)((uintptr_t)&(MAP)->data->key - (uintptr_t)(MAP)->data)
#define DS_MapVOffset(MAP) (int)((uintptr_t)&(MAP)->data->value - (uintptr_t)(MAP)->data)

#define DS_SlotN(ALLOCATOR) sizeof((ALLOCATOR)->buckets[0]->slots) / sizeof((ALLOCATOR)->buckets[0]->slots[0])
#define DS_SlotBucketSize(ALLOCATOR) sizeof(*(ALLOCATOR)->buckets[0])
#define DS_SlotBucketNextPtrOffset(ALLOCATOR) (uint32_t)((uintptr_t)&(ALLOCATOR)->buckets[0]->next_bucket - (uintptr_t)(ALLOCATOR)->buckets[0])
#define DS_SlotSize(ALLOCATOR) sizeof((ALLOCATOR)->buckets[0]->slots[0])

#define DS_BucketElemSize(LIST) (sizeof((LIST)->buckets[0]->dummy_T))
#define DS_BucketNextPtrPadding(LIST) ((uintptr_t)&(LIST)->buckets[0]->dummy_ptr - (uintptr_t)(LIST)->buckets[0] - DS_BucketElemSize(LIST))
#define DS_BucketNextPtrOffset(LIST) ((LIST)->elems_per_bucket * DS_BucketElemSize(LIST) + DS_BucketNextPtrPadding(LIST))

#ifdef __cplusplus
template<typename T> static inline T* DS_Clone__(DS_Arena* a, const T& v) { T* x = (T*)DS_ArenaPush(a, sizeof(T)); *x = v; return x; }
#define DS_Clone_(T, ARENA, VALUE) DS_Clone__<T>(ARENA, VALUE)
#else
#define DS_Clone_(T, ARENA, VALUE) ((T*)0 == &(VALUE), (T*)DS_CloneSize(ARENA, &(VALUE), sizeof(VALUE)))
#endif

// -------------------------------------------------------------------

// `p` must be a power of 2.
// `x` is allowed to be negative as well.
#define DS_AlignUpPow2(x, p) (((x) + (p) - 1) & ~((p) - 1)) // e.g. (x=30, p=16) -> 32
#define DS_AlignDownPow2(x, p) ((x) & ~((p) - 1)) // e.g. (x=30, p=16) -> 16

#ifdef DS_MODE_DEBUG
static inline void DS_VecBoundsCheck_(bool x) { DS_CHECK(x); }
#define DS_VecBoundsCheck(VEC, INDEX) DS_VecBoundsCheck_((size_t)(INDEX) < (size_t)(VEC).length)
#else
#define DS_VecBoundsCheck(VEC, INDEX) (void)0
#endif

#define DS_DoublyListRemove_(list, elem, next_name) { \
	if (elem->next_name[0]) elem->next_name[0]->next_name[1] = elem->next_name[1]; \
	else list[0] = elem->next_name[1]; \
	if (elem->next_name[1]) elem->next_name[1]->next_name[0] = elem->next_name[0]; \
	else list[1] = elem->next_name[0]; \
	}

#define DS_DoublyListPushBack_(list, elem, next_name) { \
	elem->next_name[0] = list[1]; \
	elem->next_name[1] = NULL; \
	if (list[1]) list[1]->next_name[1] = elem; \
	else list[0] = elem; \
	list[1] = elem; \
	}

static inline void* DS_ListPopFront_(void** p_list, void* next) {
	void* first = *p_list;
	*p_list = next;
	return first;
}

#define DS_KIB(x) ((uint64_t)(x) << 10)
#define DS_MIB(x) ((uint64_t)(x) << 20)
#define DS_GIB(x) ((uint64_t)(x) << 30)
#define DS_TIB(x) ((uint64_t)(x) << 40)

/*
Remove a node from doubly-linked list, e.g.
	struct Node { Node *next[2]; };
	Node *list[2] = ...;
	Node *node = ...;
	DS_DoublyListRemove(node, list, next);
*/
#define DS_DoublyListRemove(list, elem, next_name)   DS_DoublyListRemove_(list, elem, next_name)

/*
Push a node to the end of doubly-linked list, e.g.
	struct Node { Node *next[2]; };
	Node *list[2] = ...;
	Node *node = ...;
	DS_DoublyListPushBack(node, list, next);
*/
#define DS_DoublyListPushBack(list, elem, next_name)  DS_DoublyListPushBack_(list, elem, next_name)

/*
Remove the first node of a singly linked list, e.g.
	struct Node { Node *next; };
	Node *list = ...;
	Node *first = DS_ListPopFront(&list, next);
*/
#define DS_ListPopFront(list, next_name)  DS_ListPopFront_(list, (*list)->next_name)

#define DS_ListPushFront(list, elem, next_name) {(elem)->next_name = *(list); *(list) = (elem);}

#define DS_Clone(T, ARENA, VALUE) DS_Clone_(T, ARENA, VALUE)
#define DS_New(T, ARENA) (T*)DS_Clone(T, (ARENA), DS_LangAgnosticLiteral(T){0})

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
DS_API uint32_t DS_MurmurHash3(const void* key, int len, uint32_t seed);
DS_API uint64_t DS_MurmurHash64A(const void* key, int len, uint64_t seed);

#define DS_Map(K, V) \
	struct { DS_ArenaOrHeap *arena; struct{ uint32_t hash; K key; V value; } *data; int length; int capacity; }
typedef DS_Map(char, char) DS_MapRaw;

#define DS_MapInit(MAP)                DS_MapInitRaw((DS_MapRaw*)(MAP), DS_USE_HEAP)
#define DS_MapInitA(MAP, ARENA)        DS_MapInitRaw((DS_MapRaw*)(MAP), (ARENA))

// * Returns true if the key was found.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapFind(MAP, KEY, OUT_VALUE) /* (DS_Map(K, V) *MAP, K KEY, (optional null) V *OUT_VALUE) */ \
	(DS_MapTypecheckK(MAP, &(KEY)) && DS_MapTypecheckV(MAP, OUT_VALUE), \
	DS_MapFindRaw((DS_MapRaw*)MAP, &(KEY), OUT_VALUE, DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Returns the address of the value if the key was found, otherwise NULL.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapFindPtr(MAP, KEY) /* (DS_Map(K, V) *MAP, K KEY) */ \
	(DS_MapTypecheckK(MAP, &(KEY)), \
	DS_MapFindPtrRaw((DS_MapRaw*)MAP, &(KEY), DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Returns true if the key was newly added.
// * Existing keys get overwritten with the new value.
// * KEY and VALUE must be l-values, otherwise this macro won't compile.
#define DS_MapInsert(MAP, KEY, VALUE) /* (DS_Map(K, V) *MAP, K KEY, V VALUE) */ \
	(DS_MapTypecheckK(MAP, &(KEY)) && DS_MapTypecheckV(MAP, &(VALUE)), \
	DS_MapInsertRaw((DS_MapRaw*)(MAP), &(KEY), &(VALUE), DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Returns true if the key was found and removed.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapRemove(MAP, KEY) /* (DS_Map(K, V) *MAP, K KEY) */ \
	(DS_MapTypecheckK(MAP, &(KEY)), \
	DS_MapRemoveRaw((DS_MapRaw*)(MAP), &(KEY), DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

// * Return true if the key was newly added.
// * KEY must be an l-value, otherwise this macro won't compile.
#define DS_MapGetOrAddPtr(MAP, KEY, OUT_VALUE) /* (DS_Map(K, V) *MAP, K KEY, V **OUT_VALUE) */ \
	(DS_MapTypecheckK(MAP, &(KEY)) && DS_MapTypecheckV(MAP, *(OUT_VALUE)), \
	DS_MapGetOrAddRaw((DS_MapRaw*)(MAP), &(KEY), (void**)OUT_VALUE, DS_MapKSize(MAP), DS_MapVSize(MAP), DS_MapElemSize(MAP), DS_MapKOffset(MAP), DS_MapVOffset(MAP)))

#define DS_MapClear(MAP) \
	DS_MapClearRaw((DS_MapRaw*)(MAP), DS_MapElemSize(MAP))

// * Reset the map to a default state and free its memory if using the heap allocator.
#define DS_MapDeinit(MAP) /* (DS_Map(K, V) *MAP) */ \
	DS_MapDeinitRaw((DS_MapRaw*)(MAP), DS_MapElemSize(MAP))

#define DS_ForMapEach(K, V, MAP, IT) /* (type K, type V, DS_Map(K, V) *MAP, name IT) */ \
	struct DS_Concat(_dummy_, __LINE__) { int i_next; K *key; V *value; }; \
	if ((MAP)->length > 0) for (struct DS_Concat(_dummy_, __LINE__) IT = {0}; \
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
	struct { struct{ uint32_t hash; K key; } *data; int length; int capacity; DS_Arena *arena; }
typedef DS_Set(char) DS_SetRaw;

#define DS_SetInit(SET)                DS_MapInitRaw((DS_MapRaw*)(SET), DS_USE_HEAP)
#define DS_SetInitA(SET, ARENA)        DS_MapInitRaw((DS_MapRaw*)(SET), (ARENA))

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
	if ((SET)->length > 0) for (struct DS_Concat(_dummy_, __LINE__) IT = {0}; \
		DS_MapIter((DS_MapRaw*)(SET), &IT.i_next, (void**)&IT.elem, NULL, DS_MapKOffset(SET), 0, DS_MapElemSize(SET)); )

#define DS_SetClear(SET) \
	DS_MapClearRaw((DS_MapRaw*)(SET), DS_MapElemSize(SET))

// * Reset the set to a default state and free its memory if using the heap allocator.
#define DS_SetDeinit(SET) /* (DS_Set(K) *SET) */ \
	DS_MapDeinitRaw((DS_MapRaw*)(SET), DS_MapElemSize(SET))

// * Return true if the key was newly added.
static inline bool DS_MapGetOrAddRaw(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
static inline bool DS_MapGetOrAddRawEx(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset, uint32_t hash);

// * Returns true if the key was found and removed.
static inline bool DS_MapRemoveRaw(DS_MapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset);

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

DS_API void DS_ArenaInit(DS_Arena* arena, int block_size);
DS_API void DS_ArenaDeinit(DS_Arena* arena);

DS_API char* DS_ArenaPush(DS_Arena* arena, int size);
DS_API char* DS_ArenaPushZero(DS_Arena* arena, int size);
DS_API char* DS_ArenaPushEx(DS_Arena* arena, int size, int alignment);

DS_API DS_ArenaMark DS_ArenaGetMark(DS_Arena* arena);
DS_API void DS_ArenaSetMark(DS_Arena* arena, DS_ArenaMark mark);
DS_API void DS_ArenaReset(DS_Arena* arena);

// -- Memory allocation --------------------------------

static char* DS_Realloc(const void* old_ptr, int new_size, int new_alignment) {
	char* result = DS_Realloc_Impl((char*)old_ptr, new_size, new_alignment);
	DS_CHECK(((uintptr_t)result & (new_alignment - 1)) == 0); // check that the alignment is correct
	return result;
}

#define DS_MemAllocEx(SIZE, ALIGNMENT) DS_Realloc(NULL, SIZE, ALIGNMENT)
#define DS_MemAlloc(SIZE)              DS_Realloc(NULL, SIZE, DS_DEFAULT_ALIGNMENT)
#define DS_MemFree(PTR)                DS_Realloc(PTR, 0, 1)

static inline void* DS_CloneSize(DS_Arena* arena, const void* value, int size) { void* p = DS_ArenaPush(arena, size); return memcpy(p, value, size); }
static inline void* DS_CloneSizeA(DS_Arena* arena, const void* value, int size, int align) { void* p = DS_ArenaPushEx(arena, size, align); return memcpy(p, value, size); }

// -- DS_Vec --------------------------------

#define DS_Vec(T) struct { DS_ArenaOrHeap *arena; T *data; int length; int capacity; }
typedef DS_Vec(void) DS_VecRaw;

#define DS_VecGet(VEC, INDEX)          (DS_VecBoundsCheck(VEC, INDEX), ((VEC).data)[INDEX])
#define DS_VecGetPtr(VEC, INDEX)       (DS_VecBoundsCheck(VEC, INDEX), &((VEC).data)[INDEX])
#define DS_VecSet(VEC, INDEX, VALUE)   (DS_VecBoundsCheck(VEC, INDEX), ((VEC).data)[INDEX] = VALUE)
#define DS_VecElemSize(VEC)            sizeof(*(VEC).data)
#define DS_VecPeek(VEC)                (DS_VecBoundsCheck(VEC, (VEC).length - 1), (VEC).data[(VEC).length - 1])
#define DS_VecPeekPtr(VEC)             (DS_VecBoundsCheck(VEC, (VEC).length - 1), &(VEC).data[(VEC).length - 1])

// NOTE: this doesn't reserve any extra capacity at the end... maybe I should change that.
#define DS_VecClone(T, VEC)            DS_LangAgnosticLiteral(T){DS_CloneSize(VEC.data, VEC.length * DS_VecElemSize(VEC)), VEC.length, VEC.length}

#define DS_SliceClone(T, VEC)          DS_LangAgnosticLiteral(T){DS_CloneSize(VEC.data, VEC.length * DS_VecElemSize(VEC)), VEC.length}

#define DS_VecReserve(VEC, CAPACITY)   DS_VecReserveRaw((DS_VecRaw*)(VEC), CAPACITY, DS_VecElemSize(*VEC))

#define DS_VecInit(VEC)                DS_VecInitRaw((DS_VecRaw*)(VEC), DS_USE_HEAP)

// You may pass `DS_USE_HEAP` into `ARENA`, in which case you must also call DS_VecDeinit on this vec
#define DS_VecInitA(VEC, ARENA)        DS_VecInitRaw((DS_VecRaw*)(VEC), (ARENA))

#define DS_VecPush(VEC, ...) do { \
	DS_VecReserveRaw((DS_VecRaw*)(VEC), (VEC)->length + 1, DS_VecElemSize(*(VEC))); \
	(VEC)->data[(VEC)->length++] = __VA_ARGS__; } while (0)

#define DS_VecPushN(VEC, ELEMS_DATA, ELEMS_LENGTH) (DS_VecTypecheck(VEC, ELEMS_DATA), DS_VecPushNRaw((DS_VecRaw*)(VEC), ELEMS_DATA, ELEMS_LENGTH, DS_VecElemSize(*VEC)))

#define DS_VecPushVec(VEC, ELEMS)       (DS_VecTypecheck(VEC, ELEMS.data), DS_VecPushNRaw((DS_VecRaw*)(VEC), ELEMS.data, ELEMS.length, DS_VecElemSize(*VEC)))

#define DS_VecInsert(VEC, AT, ELEM)     (DS_VecTypecheck(VEC, &(ELEM)), DS_VecInsertRaw((DS_VecRaw*)(VEC), AT, &(ELEM), DS_VecElemSize(*VEC)))

#define DS_VecInsertN(VEC, AT, ELEMS_DATA, ELEMS_LENGTH) (DS_VecTypecheck(VEC, ELEMS_DATA), DS_VecInsertNRaw((DS_VecRaw*)(VEC), AT, ELEMS_DATA, ELEMS_LENGTH, DS_VecElemSize(*VEC)))

#define DS_VecRemove(VEC, INDEX)       DS_VecRemoveRaw((DS_VecRaw*)(VEC), INDEX, DS_VecElemSize(*VEC))

#define DS_VecRemoveN(VEC, INDEX, N)   DS_VecRemoveNRaw((DS_VecRaw*)(VEC), INDEX, N, DS_VecElemSize(*VEC))

#define DS_VecPop(VEC)                 (VEC)->data[--(VEC)->length]

#define DS_VecResize(VEC, ELEM, LEN)   (DS_VecTypecheck(VEC, &(ELEM)), DS_VecResizeRaw((DS_VecRaw*)(VEC), LEN, &(ELEM), DS_VecElemSize(*VEC)))

#define DS_VecResizeUndef(VEC, LEN)    DS_VecResizeRaw((DS_VecRaw*)(VEC), LEN, NULL, DS_VecElemSize(*VEC))

#define DS_VecClear(VEC)               (VEC)->length = 0

// Reset the array to a default state and free its memory if using the heap allocator.
#define DS_VecDeinit(VEC)             DS_VecDeinitRaw((DS_VecRaw*)(VEC), DS_VecElemSize(*VEC))

// Magical array iteration macro.
// e.g.
// DS_Vec(float) foo;
// DS_ForVecEach(float, &foo, it) {
//     printf("foo at index %d has the value of %f\n", it.i, it.elem);
// }
#define DS_ForVecEach(T, VEC, IT) \
	(void)((T*)0 == (VEC)->data); /* Trick the compiler into checking that T is the same as the elem type of VEC */ \
	struct DS_Concat(_dummy_, __LINE__) {int i; T *ptr;}; /* Declaring new struct types in for-loop initializers is not standard C */ \
	for (struct DS_Concat(_dummy_, __LINE__) IT = {0, (VEC)->data}; IT.i < (VEC)->length; IT.i++, IT.ptr++)


DS_API int DS_VecPushRaw(DS_VecRaw* array, const void* elem, int elem_size);
DS_API int DS_VecPushNRaw(DS_VecRaw* array, const void* elems, int n, int elem_size);
DS_API void DS_VecInsertRaw(DS_VecRaw* array, int at, const void* elem, int elem_size);
DS_API void DS_VecInsertNRaw(DS_VecRaw* array, int at, const void* elems, int n, int elem_size);
DS_API void DS_VecRemoveRaw(DS_VecRaw* array, int i, int elem_size);
DS_API void DS_VecRemoveNRaw(DS_VecRaw* array, int i, int n, int elem_size);
DS_API void DS_VecPopRaw(DS_VecRaw* array, DS_OUT void* out_elem, int elem_size);
DS_API void DS_VecReserveRaw(DS_VecRaw* array, int capacity, int elem_size);
DS_API void DS_VecResizeRaw(DS_VecRaw* array, int length, const void* value, int elem_size); // set value to NULL to not initialize the memory

// -- Slot Allocator --------------------------------------------------------------------

#define DS_SlotAllocator(T, N) struct { \
	DS_ArenaOrHeap *arena; \
	struct { union {T value; T *next_free_slot;} slots[N]; void *next_bucket; } *buckets[2]; /* first and last bucket */ \
	T *first_free_elem; \
	uint32_t last_bucket_end; \
	uint32_t bucket_next_ptr_offset; \
	uint32_t slot_size; \
	uint32_t bucket_size; \
	uint32_t num_slots_per_bucket; }

typedef DS_SlotAllocator(char, 1) SlotAllocatorRaw;

#define DS_ForSlotAllocatorEachSlot(T, ALLOCATOR, IT) \
	struct DS_Concat(_dummy_, __LINE__) {void *bucket; uint32_t slot_index; T *elem;}; \
	for (struct DS_Concat(_dummy_, __LINE__) IT = {(ALLOCATOR)->buckets[0]}; DS_SlotAllocatorIter((SlotAllocatorRaw*)(ALLOCATOR), &IT.bucket, &IT.slot_index, (void**)&IT.elem);)

#define DS_SlotAllocatorInit(ALLOCATOR) \
	DS_SlotAllocatorInitRaw((SlotAllocatorRaw*)(ALLOCATOR), DS_SlotBucketNextPtrOffset(ALLOCATOR), DS_SlotSize(ALLOCATOR), DS_SlotBucketSize(ALLOCATOR), DS_SlotN(ALLOCATOR), DS_USE_HEAP)

#define DS_SlotAllocatorInitA(ALLOCATOR, ARENA) \
	DS_SlotAllocatorInitRaw((SlotAllocatorRaw*)(ALLOCATOR), DS_SlotBucketNextPtrOffset(ALLOCATOR), DS_SlotSize(ALLOCATOR), DS_SlotBucketSize(ALLOCATOR), DS_SlotN(ALLOCATOR), ARENA)


// * The slot's memory won't be initialized or overridden if it existed before.
#define DS_TakeSlot(ALLOCATOR) \
	DS_TakeSlotRaw((SlotAllocatorRaw*)(ALLOCATOR))

#define DS_FreeSlot(ALLOCATOR, SLOT) \
	DS_FreeSlotRaw((SlotAllocatorRaw*)(ALLOCATOR), SLOT)

// -- Bucket List --------------------------------------------------------------------

typedef struct DS_BucketListIndex {
	void* bucket;
	uint32_t slot_index;
} DS_BucketListIndex;

#define DS_BucketList(T) struct { \
	DS_Arena *arena; \
	SlotAllocatorRaw *slot_allocator; \
	struct {T dummy_T; void *dummy_ptr;} *buckets[2]; /* first and last bucket */ \
	uint32_t last_bucket_end; \
	uint32_t elems_per_bucket; }

typedef DS_BucketList(char) BucketListRaw;

//#define BucketListInitUsingSlotAllocator(LIST, SLOT_ALLOCATOR) BucketListInitUsingSlotAllocatorRaw((BucketListRaw*)(LIST), (SlotAllocatorRaw*)(SLOT_ALLOCATOR), DS_BucketElemSize(LIST))

#define DS_BucketListPush(LIST, ...) \
	DS_BucketListPushRaw((BucketListRaw*)(LIST), (__VA_ARGS__), DS_BucketElemSize(LIST), DS_BucketNextPtrOffset(LIST))

#define DS_ForBucketListEach(T, LIST, IT) \
	struct DS_Concat(_dummy_, __LINE__) {void *bucket; uint32_t slot_index; T *elem;}; \
	for (struct DS_Concat(_dummy_, __LINE__) IT = {(LIST)->buckets[0]}; DS_BucketListIter((BucketListRaw*)LIST, &IT.bucket, &IT.slot_index, (void**)&IT.elem, DS_BucketElemSize(LIST), DS_BucketNextPtrOffset(LIST));)

#define DS_BucketListGetPtr(LIST, INDEX) \
	(void*)((char*)(INDEX).bucket + DS_BucketElemSize(LIST)*(INDEX).slot_index)

// * Returns DS_BucketListIndex
#define DS_BucketListGetEnd(LIST)        DS_BucketListGetEndRaw((BucketListRaw*)(LIST))

#define DS_BucketListSetEnd(LIST, END)   DS_BucketListSetEndRaw((BucketListRaw*)(LIST), (END))

#define DS_BucketListDeinit(LIST)       DS_BucketListDeinitRaw((BucketListRaw*)(LIST), DS_BucketNextPtrOffset(LIST))

// -- IMPLEMENTATION ------------------------------------------------------------------

static inline bool DS_SlotAllocatorIter(SlotAllocatorRaw* allocator, void** bucket, uint32_t* slot_index, void** elem) {
	if (*bucket == allocator->buckets[1] && *slot_index == allocator->last_bucket_end) {
		return false; // we're finished
	}

	if (*slot_index == allocator->num_slots_per_bucket) {
		// go to the next bucket
		*bucket = *(void**)((char*)*bucket + allocator->bucket_next_ptr_offset);
		*slot_index = 0;
	}

	*elem = (void*)((char*)*bucket + allocator->slot_size * (*slot_index));
	*slot_index += 1;
	return true;
}

static inline bool DS_BucketListIter(BucketListRaw* list, void** bucket, uint32_t* slot_index, void** elem, int elem_size, int next_bucket_ptr_offset) {
	if (*bucket == list->buckets[1] && *slot_index == list->last_bucket_end) {
		return false; // we're finished
	}

	if (*slot_index == list->elems_per_bucket) {
		// go to the next bucket
		*bucket = *(void**)((char*)*bucket + next_bucket_ptr_offset);
		*slot_index = 0;
	}

	*elem = (void*)((char*)*bucket + elem_size * (*slot_index));
	*slot_index += 1;
	return true;
}

static inline void DS_SlotAllocatorInitRaw(SlotAllocatorRaw* allocator,
	uint32_t bucket_next_ptr_offset, uint32_t slot_size, uint32_t bucket_size,
	uint32_t num_slots_per_bucket, DS_Arena* arena)
{
	DS_ProfEnter();
	SlotAllocatorRaw result = {0};
	result.bucket_next_ptr_offset = bucket_next_ptr_offset;
	result.slot_size = slot_size;
	result.bucket_size = bucket_size;
	result.num_slots_per_bucket = num_slots_per_bucket;
	result.arena = arena;
	*allocator = result;
	DS_ProfExit();
}

static inline void* DS_TakeSlotRaw(SlotAllocatorRaw* allocator) {
	DS_ProfEnter();
	void* result = NULL;
	DS_CHECK(allocator->arena != NULL); // Did you forget to call DS_SlotAllocatorInit?

	if (allocator->first_free_elem) {
		// Take an existing slot
		result = allocator->first_free_elem;
		allocator->first_free_elem = *(char**)((char*)result);
	}
	else {
		// Allocate from the end

		void* bucket = allocator->buckets[1];

		if (bucket == NULL || allocator->last_bucket_end == allocator->num_slots_per_bucket) {
			// We need to allocate a new bucket
			if (allocator->arena == DS_USE_HEAP) __debugbreak(); // TODO

			void* new_bucket = DS_ArenaPush(allocator->arena, allocator->bucket_size);
			if (bucket) {
				// set the `next` pointer of the previous bucket to point to the new bucket
				*(void**)((char*)bucket + allocator->bucket_next_ptr_offset) = new_bucket;
			}
			else {
				// set the `first bucket` pointer to point to the new bucket
				memcpy(&allocator->buckets[0], &new_bucket, sizeof(void*));
				*(void**)((char*)new_bucket + allocator->bucket_next_ptr_offset) = NULL;
			}

			allocator->last_bucket_end = 0;
			memcpy(&allocator->buckets[1], &new_bucket, sizeof(void*));
			bucket = new_bucket;
		}

		uint32_t slot_idx = allocator->last_bucket_end++;
		result = (char*)bucket + allocator->slot_size * slot_idx;
	}

	DS_ProfExit();
	return result;
}

static inline void DS_FreeSlotRaw(SlotAllocatorRaw* allocator, void* slot) {
	// It'd be nice to provide some debug checking thing to detect double frees
	DS_CHECK(allocator->first_free_elem != (char*)slot);

	*(char**)((char*)slot) = allocator->first_free_elem;
	allocator->first_free_elem = (char*)slot;
}

static inline DS_BucketListIndex DS_BucketListGetEndRaw(BucketListRaw* list) {
	DS_BucketListIndex index = { list->buckets[1], list->last_bucket_end };
	return index;
}

static inline void DS_BucketListSetEndRaw(BucketListRaw* list, DS_BucketListIndex end) {
	memcpy(&list->buckets[1], &end.bucket, sizeof(void*));
	list->last_bucket_end = end.slot_index;
}

//static inline void BucketListInitUsingSlotAllocatorRaw(BucketListRaw *list, SlotAllocatorRaw *slot_allocator, uint32_t elem_size) {
//	DS_CHECK(slot_allocator->slot_next_ptr_offset > elem_size); // The slot allocator slot size must be >= bucket list element size
//
//	BucketListRaw result = {0};
//	result.slot_allocator = slot_allocator;
//	result.elems_per_bucket = slot_allocator->slot_next_ptr_offset / elem_size; // round down
//	*list = result;
//}

static inline void DS_BucketListDeinitRaw(BucketListRaw* list, int next_bucket_ptr_offset) {
	DS_ProfEnter();
	void* bucket = list->buckets[0];
	for (; bucket;) {
		void* next_bucket = *(void**)((char*)bucket + next_bucket_ptr_offset);

		if (list->slot_allocator) {
			DS_FreeSlotRaw(list->slot_allocator, bucket);
		}
		else {
			DS_DebugFillGarbage(bucket, next_bucket_ptr_offset);
			if (list->arena == NULL) __debugbreak(); // TODO
		}

		bucket = next_bucket;
	}

	BucketListRaw empty = {0};
	*list = empty;
	DS_ProfExit();
}

static inline void* DS_BucketListPushRaw(BucketListRaw* list, void* elem, int elem_size, int next_bucket_ptr_offset) {
	DS_ProfEnter();
	void* bucket = list->buckets[1];

	if (bucket == NULL || list->last_bucket_end == list->elems_per_bucket) {
		// We need to allocate a new bucket. Or take an existing one from the end (only possible if DS_BucketListSetEnd has been used).
		void* new_bucket = NULL;

		// There may be an unused bucket at the end that we can use instead of allocating a new one.
		// First try to use that.
		if (bucket) {
			new_bucket = *(void**)((char*)bucket + next_bucket_ptr_offset);
		}

		if (new_bucket == NULL) {
			if (list->slot_allocator) {
				int bucket_size = next_bucket_ptr_offset + sizeof(void*);
				DS_CHECK(list->slot_allocator->slot_size >= (uint32_t)bucket_size);
				new_bucket = DS_TakeSlotRaw(list->slot_allocator);
			}
			else {
				if (list->arena == NULL) __debugbreak(); // TODO
				int bucket_size = next_bucket_ptr_offset + sizeof(void*);
				new_bucket = DS_ArenaPush(list->arena, bucket_size);
			}
		}

		if (bucket) {
			// set the `next` pointer of the previous bucket to point to the new bucket
			*(void**)((char*)bucket + next_bucket_ptr_offset) = new_bucket;
		}
		else {
			// set the `first bucket` pointer to point to the new bucket
			memcpy(&list->buckets[0], &new_bucket, sizeof(void*));
			*(void**)((char*)new_bucket + next_bucket_ptr_offset) = NULL;
		}

		list->last_bucket_end = 0;
		memcpy(&list->buckets[1], &new_bucket, sizeof(void*));
		bucket = new_bucket;
	}

	uint32_t slot_idx = list->last_bucket_end++;
	void* result = (char*)bucket + elem_size * slot_idx;
	memcpy(result, elem, elem_size);
	DS_ProfExit();
	return result;
}

DS_API void DS_VecReserveRaw(DS_VecRaw* array, int capacity, int elem_size) {
	DS_ProfEnter();
	DS_CHECK(array->arena != NULL); // Have you called DS_VecInit?

	int new_capacity = array->capacity;
	while (capacity > new_capacity) {
		new_capacity = new_capacity == 0 ? 8 : new_capacity * 2;
	}

	if (new_capacity != array->capacity) {
		if (array->arena == DS_USE_HEAP) {
			array->data = DS_Realloc(array->data, new_capacity * elem_size, DS_DEFAULT_ALIGNMENT);
		}
		else {
			void* new_data = DS_ArenaPushEx(array->arena, new_capacity * elem_size, DS_DEFAULT_ALIGNMENT);
			memcpy(new_data, array->data, array->length * elem_size);
			DS_DebugFillGarbage(array->data, array->length * elem_size); // fill the old data with garbage to potentially catch errors
			array->data = new_data;
		}

		array->capacity = new_capacity;
	}

	DS_ProfExit();
}

DS_API void DS_VecRemoveRaw(DS_VecRaw* array, int i, int elem_size) { DS_VecRemoveNRaw(array, i, 1, elem_size); }

DS_API void DS_VecRemoveNRaw(DS_VecRaw* array, int i, int n, int elem_size) {
	DS_ProfEnter();
	DS_CHECK(i + n <= array->length);

	char* dst = (char*)array->data + i * elem_size;
	char* src = dst + elem_size * n;
	memmove(dst, src, ((char*)array->data + array->length * elem_size) - src);

	array->length -= n;
	DS_ProfExit();
}

DS_API void DS_VecInsertRaw(DS_VecRaw* array, int at, const void* elem, int elem_size) {
	DS_ProfEnter();
	DS_VecInsertNRaw(array, at, elem, 1, elem_size);
	DS_ProfExit();
}

DS_API void DS_VecInsertNRaw(DS_VecRaw* array, int at, const void* elems, int n, int elem_size) {
	DS_ProfEnter();
	DS_CHECK(at <= array->length);
	DS_VecReserveRaw(array, array->length + n, elem_size);

	// Move existing elements forward
	char* offset = (char*)array->data + at * elem_size;
	memmove(offset + n * elem_size, offset, (array->length - at) * elem_size);

	memcpy(offset, elems, n * elem_size);
	array->length += n;
	DS_ProfExit();
}

DS_API int DS_VecPushNRaw(DS_VecRaw* array, const void* elems, int n, int elem_size) {
	DS_ProfEnter();
	DS_VecReserveRaw(array, array->length + n, elem_size);

	memcpy((char*)array->data + elem_size * array->length, elems, elem_size * n);

	int result = array->length;
	array->length += n;
	DS_ProfExit();
	return result;
}

DS_API void DS_VecResizeRaw(DS_VecRaw* array, int length, DS_OUT const void* value, int elem_size) {
	DS_ProfEnter();
	DS_VecReserveRaw(array, length, elem_size);

	if (value) {
		for (int i = array->length; i < length; i++) {
			memcpy((char*)array->data + i * elem_size, value, elem_size);
		}
	}

	array->length = length;
	DS_ProfExit();
}

DS_API void DS_VecInitRaw(DS_VecRaw* array, DS_Arena* arena) {
	DS_VecRaw empty = {0};
	*array = empty;
	array->arena = arena;
}

DS_API void DS_VecDeinitRaw(DS_VecRaw* array, int elem_size) {
	DS_ProfEnter();
	DS_DebugFillGarbage(array->data, array->capacity * elem_size);
	if (array->arena == DS_USE_HEAP) {
		DS_Realloc(array->data, 0, DS_DEFAULT_ALIGNMENT);
	}
	DS_VecRaw empty = {0};
	*array = empty;
	DS_ProfExit();
}

DS_API int DS_VecPushRaw(DS_VecRaw* array, const void* elem, int elem_size) {
	DS_ProfEnter();
	DS_VecReserveRaw(array, array->length + 1, elem_size);

	memcpy((char*)array->data + array->length * elem_size, elem, elem_size);
	DS_ProfExit();
	return array->length++;
}

DS_API void DS_VecPopRaw(DS_VecRaw* array, DS_OUT void* out_elem, int elem_size) {
	DS_ProfEnter();
	DS_CHECK(array->length >= 1);
	array->length--;
	if (out_elem) {
		memcpy(out_elem, (char*)array->data + array->length * elem_size, elem_size);
	}
	DS_ProfExit();
}

static inline void DS_MapInitRaw(DS_MapRaw* map, DS_Arena* arena) {
	DS_MapRaw empty = {0};
	*map = empty;
	map->arena = arena;
}

static inline void DS_MapClearRaw(DS_MapRaw* map, int elem_size) {
	memset(map->data, 0, map->capacity * elem_size);
	map->length = 0;
}

static inline void DS_MapDeinitRaw(DS_MapRaw* map, int elem_size) {
	DS_ProfEnter();
	DS_DebugFillGarbage(map->data, map->capacity * elem_size);
	if (map->arena == DS_USE_HEAP) {
		DS_Realloc(map->data, 0, DS_DEFAULT_ALIGNMENT);
	}
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
static uint64_t DS_MurmurHash64A(const void* key, int len, uint64_t seed) {
	DS_ProfEnter();
	const uint64_t m = 0xc6a4a7935bd1e995LLU;
	const int r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t* data = (const uint64_t*)key;
	const uint64_t* end = data + (len / 8);

	while (data != end) {
		uint64_t k = *data++;
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;
	}

	const unsigned char* data2 = (const unsigned char*)data;

	switch (len & 7) {
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
DS_API uint32_t DS_MurmurHash3(const void* key, int len, uint32_t seed) {
	DS_ProfEnter();
	const uint8_t* data = (const uint8_t*)key;
	const int nblocks = len / 4;

	uint32_t h1 = seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	// body

	const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
	for (int i = -nblocks; i; i++) {
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
	switch (len & 3) {
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 *= c1; k1 = DS_ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
	};

	// finalization

	h1 ^= len;
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

static inline bool DS_MapGetOrAddRawEx(DS_MapRaw* map, const void* key, DS_OUT void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset, uint32_t hash) {
	DS_ProfEnter();
	DS_CHECK(map->arena != NULL); // Have you called DS_MapInit?

	if (100 * (map->length + 1) > 70 * map->capacity) {
		// Grow the map

		char* old_data = (char*)map->data;
		int old_capacity = map->capacity;

		map->capacity = old_capacity == 0 ? 8 : old_capacity * 2;
		map->length = 0;

		void* new_data = map->arena == DS_USE_HEAP ?
			DS_Realloc(NULL, map->capacity * elem_size, DS_DEFAULT_ALIGNMENT) :
			DS_ArenaPush(map->arena, map->capacity * elem_size);

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
		if (map->arena == DS_USE_HEAP) {
			DS_Realloc(old_data, 0, DS_DEFAULT_ALIGNMENT); // free the old data if using the heap allocator
		}
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
			map->length++;
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

	char temp[DS_MAX_MAP_SLOT_SIZE];
	DS_CHECK(DS_MAX_MAP_SLOT_SIZE >= elem_size);

	bool ok = true;
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
			map->length--;

			// backwards-shift deletion.
			// First remove all elements directly after this element from the map, then add them back to the map, starting from the first one to the right.
			for (;;) {
				index = (index + 1) & mask;

				char* shifting_elem_base = (char*)map->data + index * elem_size;
				uint32_t shifting_elem_hash = *(uint32_t*)shifting_elem_base;
				if (shifting_elem_hash == 0) break;

				memcpy(temp, shifting_elem_base, elem_size);
				memset(shifting_elem_base, 0, elem_size);
				map->length--;

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

DS_API void DS_ArenaInit(DS_Arena* arena, int block_size) {
#ifdef DS_CUSTOM_ARENA
	DS_ArenaInit_Custom(arena, block_size);
#else
	DS_Arena _arena = {0};
	_arena.block_size = block_size;
	*arena = _arena;
#endif
}

DS_API void DS_ArenaDeinit(DS_Arena* arena) {
#ifdef DS_CUSTOM_ARENA
	DS_ArenaDeinit_Custom(arena);
#else
	for (DS_DefaultArenaBlockHeader* block = arena->first_block; block;) {
		DS_DefaultArenaBlockHeader* next = block->next;
		DS_MemFree(block);
		block = next;
	}
	DS_DebugFillGarbage(arena, sizeof(DS_Arena));
#endif
}

DS_API char* DS_ArenaPush(DS_Arena* arena, int size) {
	return DS_ArenaPushEx(arena, size, DS_DEFAULT_ALIGNMENT);
}

DS_API char* DS_ArenaPushZero(DS_Arena* arena, int size) {
	char* ptr = DS_ArenaPushEx(arena, size, DS_DEFAULT_ALIGNMENT);
	memset(ptr, 0, size);
	return ptr;
}

DS_API char* DS_ArenaPushEx(DS_Arena* arena, int size, int alignment) {
#ifdef DS_CUSTOM_ARENA
	char* result = DS_ArenaPush_Custom(arena, size, alignment)
		return result;
#else
	DS_ProfEnter();

	bool alignment_is_power_of_2 = ((alignment) & ((alignment)-1)) == 0;
	DS_CHECK(alignment != 0 && alignment_is_power_of_2);
	DS_CHECK(alignment <= DS_MAX_ALIGNMENT); // DS_Arena blocks get allocated using DS_MAX_ALIGNMENT, and so all allocations within an arena must conform to that.

	DS_DefaultArenaBlockHeader* curr_block = arena->mark.block; // may be NULL
	void* curr_ptr = arena->mark.ptr;

	char* result_address = (char*)DS_AlignUpPow2((uintptr_t)curr_ptr, alignment);
	int remaining_space = curr_block ? curr_block->size_including_header - (int)((uintptr_t)result_address - (uintptr_t)curr_block) : 0;

	if (size > remaining_space) { // We need a new block!
		int result_offset = DS_AlignUpPow2(sizeof(DS_DefaultArenaBlockHeader), alignment);
		int new_block_size = result_offset + size;
		if (arena->block_size > new_block_size) new_block_size = arena->block_size;

		DS_DefaultArenaBlockHeader* new_block = NULL;
		DS_DefaultArenaBlockHeader* next_block = NULL;

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
			new_block = (DS_DefaultArenaBlockHeader*)DS_Realloc(NULL, new_block_size, DS_MAX_ALIGNMENT);
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
	DS_ProfExit();
#endif
}

DS_API void DS_ArenaReset(DS_Arena* arena) {
#ifdef DS_CUSTOM_ARENA
	DS_ArenaReset_Custom(arena);
#else
	DS_ProfEnter();
	if (arena->first_block) {
		// Free all blocks after the first block
		for (DS_DefaultArenaBlockHeader* block = arena->first_block->next; block;) {
			DS_DefaultArenaBlockHeader* next = block->next;
			DS_MemFree(block);
			block = next;
		}
		arena->first_block->next = NULL;

		// Free the first block too if it's larger than the regular block size
		if (arena->first_block->size_including_header > arena->block_size) {
			DS_MemFree(arena->first_block);
			arena->first_block = NULL;
		}
	}
	arena->mark.block = arena->first_block;
	arena->mark.ptr = (char*)arena->first_block + sizeof(DS_DefaultArenaBlockHeader);
	DS_ProfExit();
#endif
}

DS_API DS_ArenaMark DS_ArenaGetMark(DS_Arena* arena) {
#ifdef DS_CUSTOM_ARENA
	DS_ArenaMark result = DS_ArenaGetMark_Custom(arena);
	return result;
#else
	return arena->mark;
#endif
}

DS_API void DS_ArenaSetMark(DS_Arena* arena, DS_ArenaMark mark) {
#ifdef DS_CUSTOM_ARENA
	DS_ArenaSetMark_Custom(arena, mark);
#else
	DS_ProfEnter();
	if (mark.block == NULL) {
		arena->mark.block = arena->first_block;
		arena->mark.ptr = (char*)arena->first_block + sizeof(DS_DefaultArenaBlockHeader);
	}
	else {
		arena->mark = mark;
	}
	DS_ProfExit();
#endif
}

//#ifndef DS_REALLOC_OVERRIDE
#if 0

// #define UNICODE // TODO: remove
// #include <Windows.h> // TODO: remove
#include <stdlib.h>

void* DS_Realloc_Impl(void* old_ptr, int new_size, int new_alignment) {
	//static bool inside_allocation_validation = false;
	//static DS_Set(void*) DS_alive_allocations = {.arena = DS_USE_HEAP};
	//
	//if (old_ptr != NULL && !inside_allocation_validation) {
	//	inside_allocation_validation = true;
	//
	//	bool was_alive = DS_SetRemove(&DS_alive_allocations, &old_ptr);
	//	DS_CHECK(was_alive);
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
		assert(ok == 1 && old_protect == PAGE_READWRITE);
		result = pages + (DS_AlignUpPow2_(new_size, 4096) - new_size);
#else
		// protect before
		char* pages = VirtualAlloc(NULL, new_size + 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		uint32_t old_protect;
		BOOL ok = VirtualProtect(pages, 4096, PAGE_NOACCESS, &old_protect);
		assert(ok == 1 && old_protect == PAGE_READWRITE);
		result = pages + 4096;
#endif
	}
#endif

	void* result = _aligned_realloc(old_ptr, new_size, new_alignment);

	//if (result != NULL && !inside_allocation_validation) {
	//	DS_CHECK(result != NULL);
	//	inside_allocation_validation = true;
	//
	//	bool newly_added = DS_SetAdd(&DS_alive_allocations, &result);
	//	DS_CHECK(newly_added);
	//
	//	inside_allocation_validation = false;
	//}

	return result;
}
#endif

#endif // DS_INCLUDED
// fire_string.h - string utilities
//
// Author: Eero Mutka
// Version: 0
// Date: 25 March, 2024
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//

#ifndef FIRE_STRING_INCLUDED
#define FIRE_STRING_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>  // memcpy, memmove, memset, memcmp, strlen
#include <stdarg.h>
#include <math.h> // isnan, isinf

#ifdef STR_USE_FIRE_DS_ARENA
#define STR_ARENA DS_Arena
#define STR_ARENA_ALLOCATE(ARENA, SIZE) (void*)DS_ArenaPush(ARENA, SIZE)
#else
// Provide a custom arena implementation using the following definitions:
// - STR_ARENA: the arena type
// - STR_ARENA_ALLOCATE: allocate memory from the arena, i.e. `void *STR_ARENA_ALLOCATE(STR_ARENA *arena, int size)`
#if !defined(STR_ARENA) || !defined(STR_ARENA_ALLOCATE)
#error STR_ARENA implementation is missing.
#endif
#endif

typedef struct STR {
	const char* data;
	int size;
} STR;

#ifndef STR_API
#define STR_API static
#endif

#ifndef STR_CHECK
#include <assert.h>
#define STR_CHECK(x) assert(x)
#endif

#ifndef STR_PROFILER_MACROS
// Function-level profiler scope. A single function may only have one of these, and it should span the entire function.
#define STR_ProfEnter()
#define STR_ProfExit()
#endif

// String Literal macros
#ifdef __cplusplus
#define STR_(x) STR{(char*)x, sizeof(x)-1}
#define STR__(x) STR_(x)
#else
#define STR_(x) (STR){(char*)x, sizeof(x)-1}
#define STR__(x) {(char*)x, sizeof(x)-1} // STR literal that works in C initializer lists
#endif

typedef struct STR_Builder {
	STR_ARENA* arena;
	STR str;
	int capacity;
} STR_Builder;

// typedef struct { int min, max; } STR_Range;
// typedef struct { STR_Range *data; int size; } STR_RangeArray;
typedef struct { STR* data; int size; } STR_Array;

#define STR_IsUtf8FirstByte(c) (((c) & 0xC0) != 0x80) /* is c the start of a utf8 sequence? */
#define STR_Each(str, r, i) (int i=0, r = 0, i##_next=0; (r=STR_NextCodepoint(str, &i##_next)); i=i##_next)
#define STR_EachReverse(str, r, i) (int i=str.size, r = 0; r=STR_PrevCodepoint(str, &i);)

// OLD formatting rules:
// ~s                       - STR
// ~c                       - c-style string
// ~~                       - escape for ~
// TODO: ~r                 - codepoint
// ~u8, ~u16, ~u32, ~u64    - unsigned integers
// ~i8, ~i16, ~i32, ~i64    - signed integers
// ~x8, ~x16, ~x32, ~x64    - hexadecimal integers
// ~f                       - double
STR_API void OLD_Print(STR_Builder* s, const char* fmt, ...);
STR_API void OLD_PrintVA(STR_Builder* s, const char* fmt, va_list args);
STR_API void OLD_PrintS(STR_Builder* s, STR str); // write string
STR_API void OLD_PrintSRepeat(STR_Builder* s, STR str, int count);
STR_API void OLD_PrintC(STR_Builder* s, const char* c_str);
STR_API void OLD_PrintB(STR_Builder* s, char b); // print ASCII-byte
STR_API void OLD_PrintR(STR_Builder* s, uint32_t r); // print unicode codepoint
STR_API STR OLD_APrint(STR_ARENA* arena, const char* fmt, ...); // arena-print

typedef struct STR_Formatter {
	void (*print)(STR_Builder* s, struct STR_Formatter* self);
} STR_Formatter;

/*
-- New printer API --
%s   -  c-string
%v   -  view-string (STR)
%hhd -  int8
%hd  -  int16
%d   -  int32
%lld -  int64
%hhu -  uint8
%hu  -  uint16
%u   -  uint32
%llu -  uint64
%f   -  float/double
%c   -  UTF8 codepoint (codepoint)
%%   -  %
%hhx -  hexadecimal uint8
%hx  -  hexadecimal uint16
%x   -  hexadecimal uint32
%llx -  hexadecimal uint64
%?   -  STR_Formatter pointer
*/

STR_API void STR_SetTempArena(STR_ARENA* arena);
STR_API STR_ARENA* STR_GetTempArena();

STR_API char* STR_Form(STR_ARENA* arena, const char* fmt, ...);
STR_API STR STR_FormV(STR_ARENA* arena, const char* fmt, ...);

STR_API char* STR_FormTemp(const char* fmt, ...);
STR_API STR STR_FormTempV(const char* fmt, ...);

STR_API void STR_Print(STR_Builder* s, const char* string);
STR_API void STR_PrintV(STR_Builder* s, STR string);
STR_API void STR_PrintF(STR_Builder* s, const char* fmt, ...);
STR_API void STR_PrintVA(STR_Builder* s, const char* fmt, va_list args);
STR_API void STR_PrintC(STR_Builder* s, uint32_t codepoint); // Print unicode codepoint

STR_API char* STR_Init(STR_ARENA* arena, STR s);
STR_API char* STR_InitTemp(STR s);
STR_API STR STR_InitV(const char* s);

// STR_API STR_RangeArray STR_Split(STR_ARENA *arena, STR str, char character);
//#define STR_Join(arena, ...) STR_JoinN(arena, (STR_Array){(STR[]){__VA_ARGS__}, sizeof((STR[]){__VA_ARGS__}) / sizeof(STR)})
//STR_API STR STR_JoinN(STR_ARENA *arena, STR_Array args);

// * Underscores are allowed and skipped
// * Works with any base up to 16 (i.e. binary, base-10, hex)
STR_API bool STR_ParseU64Ex(STR s, int base, uint64_t* out_value);
// #define STR_ToU64(s, out_value) STR_ParseU64Ex(s, 10, out_value)

// * A single minus or plus is accepted preceding the digits
// * Works with any base up to 16 (i.e. binary, base-10, hex)
// * Underscores are allowed and skipped
STR_API bool STR_ParseI64Ex(STR s, int base, int64_t* out_value);
STR_API bool STR_ParseI64(STR s, int64_t* out_value);

STR_API bool STR_ParseFloat(STR s, double* out);

STR_API STR STR_IntToStr(STR_ARENA* arena, int value);
STR_API STR STR_IntToStrEx(STR_ARENA* arena, uint64_t data, bool is_signed, int radix);
STR_API STR STR_FloatToStr(STR_ARENA* arena, double value, int min_decimals);

STR_API int STR_IntToStr_(char* buffer, uint64_t data, bool is_signed, int radix); // NOT null-terminated, the size of the string is returned.
STR_API int STR_FloatToStr_(char* buffer, double value, int min_decimals); // NOT null-terminated, the size of the string is returned.

STR_API int STR_CodepointToUTF8(char* output, uint32_t codepoint); // returns the number of bytes written
STR_API uint32_t STR_UTF8ToCodepoint(STR str);
STR_API int STR_CodepointSizeAsUTF8(uint32_t codepoint);

// Returns the character on `byteoffset`, then increments it.
// Will returns 0 if byteoffset >= str.size
STR_API uint32_t STR_NextCodepoint(STR str, int* byteoffset);

// Decrements `bytecounter`, then returns the character on it.
// Will returns 0 if byteoffset < 0
STR_API uint32_t STR_PrevCodepoint(STR str, int* byteoffset);

STR_API int STR_CodepointCount(STR str);

// -- String view utilities -------------

STR_API STR STR_Advance(STR* str, int size);
STR_API STR STR_Clone(STR_ARENA* arena, STR str);

STR_API STR STR_BeforeFirst(STR str, uint32_t codepoint); // returns `str` if no codepoint is found
STR_API STR STR_BeforeLast(STR str, uint32_t codepoint);  // returns `str` if no codepoint is found
STR_API STR STR_AfterFirst(STR str, uint32_t codepoint);  // returns `str` if no codepoint is found
STR_API STR STR_AfterLast(STR str, uint32_t codepoint);   // returns `str` if no codepoint is found

STR_API bool STR_Find(STR str, const char* substr, int* out_offset);
STR_API bool STR_FindV(STR str, STR substr, int* out_offset);
STR_API STR STR_ParseUntilAndSkip(STR* remaining, uint32_t codepoint); // cuts forward until `codepoint` or end of the string is reached
STR_API bool STR_FindFirst(STR str, uint32_t codepoint, int* out_offset);
STR_API bool STR_FindLast(STR str, uint32_t codepoint, int* out_offset);
STR_API bool STR_LastIdxOfAnyChar(STR str, STR chars, int* out_index);
STR_API bool STR_Contains(STR str, const char* substr);
STR_API bool STR_ContainsV(STR str, STR substr);
STR_API bool STR_ContainsCodepoint(STR str, uint32_t codepoint);

STR_API STR STR_Replace(STR_ARENA* arena, STR str, STR search_for, STR replace_with);
STR_API STR STR_ReplaceMulti(STR_ARENA* arena, STR str, STR_Array search_for, STR_Array replace_with);
STR_API STR STR_ToLower(STR_ARENA* arena, STR str);
STR_API uint32_t STR_CodepointToLower(uint32_t codepoint);

STR_API bool STR_EndsWith(STR str, const char* end);
STR_API bool STR_EndsWithV(STR str, STR end);
STR_API bool STR_StartsWith(STR str, const char* start);
STR_API bool STR_StartsWithV(STR str, STR start);
STR_API bool STR_CutEnd(STR* str, const char* end);
STR_API bool STR_CutEndV(STR* str, STR end);
STR_API bool STR_CutStart(STR* str, const char* start);
STR_API bool STR_CutStartV(STR* str, STR start);

STR_API bool STR_Equals(STR a, const char* b);
STR_API bool STR_EqualsV(STR a, STR b);
STR_API bool STR_EqualsCaseInsensitive(STR a, const char* b);
STR_API bool STR_EqualsCaseInsensitiveV(STR a, STR b);

STR_API STR STR_Slice(STR str, int lo, int hi);
STR_API STR STR_SliceBefore(STR str, int mid);
STR_API STR STR_SliceAfter(STR str, int mid);

// -- IMPLEMENTATION ------------------------------------------------------------

///////////// Global state //////////////
static STR_ARENA* STR_ACTIVE_TEMP_ARENA;
/////////////////////////////////////////

typedef struct STR_ASCIISet {
	char bytes[32];
} STR_ASCIISet;

static STR_ASCIISet STR_ASCIISetMake(STR chars) {
	STR_ProfEnter();
	STR_ASCIISet set = {0};
	for (int i = 0; i < chars.size; i++) {
		char c = chars.data[i];
		set.bytes[c / 8] |= 1 << (c % 8);
	}
	STR_ProfExit();
	return set;
}

static bool STR_ASCIISetContains(STR_ASCIISet set, char c) {
	return (set.bytes[c / 8] & 1 << (c % 8)) != 0;
}

STR_API STR STR_ParseUntilAndSkip(STR* remaining, uint32_t codepoint) {
	STR line = { remaining->data, 0 };

	for (;;) {
		int cp_size = 0;
		uint32_t cp = STR_NextCodepoint(*remaining, &cp_size);
		if (cp == 0) break;

		remaining->data += cp_size;
		remaining->size -= cp_size;

		if (cp == codepoint) break;
		line.size += cp_size;
	}

	return line;
}

STR_API bool STR_FindFirst(STR str, uint32_t codepoint, int* out_offset) {
	for STR_Each(str, r, offset) {
		if (r == codepoint) {
			*out_offset = offset;
			return true;
		}
	}
	return false;
}

STR_API bool STR_FindLast(STR str, uint32_t codepoint, int* out_offset) {
	for STR_EachReverse(str, r, offset) {
		if (r == codepoint) {
			*out_offset = offset;
			return true;
		}
	}
	return false;
}

STR_API bool STR_LastIdxOfAnyChar(STR str, STR chars, int* out_index) {
	STR_ProfEnter();
	STR_ASCIISet char_set = STR_ASCIISetMake(chars);

	bool found = false;
	for (int i = str.size - 1; i >= 0; i--) {
		if (STR_ASCIISetContains(char_set, str.data[i])) {
			*out_index = i;
			found = true;
			break;
		}
	}

	STR_ProfExit();
	return found;
}

STR_API STR STR_BeforeFirst(STR str, uint32_t codepoint) {
	int offset = str.size;
	STR_FindFirst(str, codepoint, &offset);
	STR result = { str.data, offset };
	return result;
}

STR_API STR STR_BeforeLast(STR str, uint32_t codepoint) {
	int offset = str.size;
	STR_FindLast(str, codepoint, &offset);
	STR result = { str.data, offset };
	return result;
}

STR_API STR STR_AfterFirst(STR str, uint32_t codepoint) {
	int offset = str.size;
	STR result = str;
	if (STR_FindFirst(str, codepoint, &offset)) {
		result = STR_SliceAfter(str, offset + STR_CodepointSizeAsUTF8(codepoint));
	}
	return result;
}

STR_API STR STR_AfterLast(STR str, uint32_t codepoint) {
	int offset = str.size;
	STR result = str;
	if (STR_FindLast(str, codepoint, &offset)) {
		result = STR_SliceAfter(str, offset + STR_CodepointSizeAsUTF8(codepoint));
	}
	return result;
}

STR_API bool STR_EqualsV(STR a, STR b) {
	return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

STR_API bool STR_EqualsCaseInsensitiveV(STR a, STR b) {
	STR_ProfEnter();
	bool equals = a.size == b.size;
	if (equals) {
		for (int i = 0; i < a.size; i++) {
			if (STR_CodepointToLower(a.data[i]) != STR_CodepointToLower(b.data[i])) {
				equals = false;
				break;
			}
		}
	}
	STR_ProfExit();
	return equals;
}

STR_API bool STR_Equals(STR a, const char* b) {
	return a.size == strlen(b) && memcmp(a.data, b, a.size) == 0;
}

STR_API bool STR_EqualsCaseInsensitive(STR a, const char* b) {
	return STR_EqualsCaseInsensitiveV(a, STR_InitV(b));
}

STR_API STR STR_Slice(STR str, int lo, int hi) {
	STR_CHECK(hi >= lo && hi <= str.size);
	STR result = { str.data + lo, hi - lo };
	return result;
}

STR_API STR STR_SliceAfter(STR str, int mid) {
	STR_CHECK(mid <= str.size);
	STR result = { str.data + mid, str.size - mid };
	return result;
}

STR_API STR STR_SliceBefore(STR str, int mid) {
	STR_CHECK(mid <= str.size);
	STR result = { str.data, mid };
	return result;
}

STR_API bool STR_Contains(STR str, const char* substr) {
	return STR_Find(str, substr, NULL);
}

STR_API bool STR_ContainsV(STR str, STR substr) {
	return STR_FindV(str, substr, NULL);
}

STR_API bool STR_FindV(STR str, STR substr, int* out_offset) {
	STR_ProfEnter();
	bool found = false;
	int i_last = str.size - substr.size;
	for (int i = 0; i <= i_last; i++) {
		if (STR_EqualsV(STR_Slice(str, i, i + substr.size), substr)) {
			if (out_offset) *out_offset = i;
			found = true;
			break;
		}
	}
	STR_ProfExit();
	return found;
};

STR_API bool STR_Find(STR str, const char* substr, int* out_offset) {
	STR_ProfEnter();
	bool found = false;
	int substr_length = (int)strlen(substr);
	int i_last = str.size - substr_length;
	for (int i = 0; i <= i_last; i++) {
		if (STR_Equals(STR_Slice(str, i, i + substr_length), substr)) {
			if (out_offset) *out_offset = i;
			found = true;
			break;
		}
	}
	STR_ProfExit();
	return found;
};

STR_API bool STR_ContainsCodepoint(STR str, uint32_t codepoint) {
	for STR_Each(str, r, i) {
		if (r == codepoint) return true;
	}
	return false;
}

STR_API STR STR_Clone(STR_ARENA* arena, STR str) {
	char* data = (char*)STR_ARENA_ALLOCATE(arena, str.size);
	memcpy(data, str.data, str.size);
	STR result = { data, str.size };
	return result;
}

STR_API bool STR_EndsWith(STR str, const char* end) { return STR_EndsWithV(str, STR_InitV(end)); }
STR_API bool STR_StartsWith(STR str, const char* start) { return STR_StartsWithV(str, STR_InitV(start)); }
STR_API bool STR_CutEnd(STR* str, const char* end) { return STR_CutEndV(str, STR_InitV(end)); }
STR_API bool STR_CutStart(STR* str, const char* start) { return STR_CutStartV(str, STR_InitV(start)); }

STR_API bool STR_EndsWithV(STR str, STR end) {
	return str.size >= end.size && STR_EqualsV(end, STR_SliceAfter(str, str.size - end.size));
}

STR_API bool STR_StartsWithV(STR str, STR start) {
	return str.size >= start.size && STR_EqualsV(start, STR_SliceBefore(str, start.size));
}

STR_API bool STR_CutEndV(STR* str, STR end) {
	STR_ProfEnter();
	bool ends_with = STR_EndsWithV(*str, end);
	if (ends_with) {
		str->size -= end.size;
	}
	STR_ProfExit();
	return ends_with;
}

STR_API bool STR_CutStartV(STR* str, STR start) {
	STR_ProfEnter();
	bool starts_with = STR_StartsWithV(*str, start);
	if (starts_with) {
		*str = STR_SliceAfter(*str, start.size);
	}
	STR_ProfExit();
	return starts_with;
}

/*STR_API STR_RangeArray STR_Split(STR_ARENA *arena, STR str, char character) {
	STR_ProfEnter();
	int required_len = 1;
	for (int i = 0; i < str.size; i++) {
		if (str.data[i] == character) required_len++;
	}

	STR_RangeArray splits = {0};
	splits.data = (StrRange*)STR_ARENA_ALLOCATE(arena, required_len * sizeof(StrRange));

	int prev = 0;
	for (int i = 0; i < str.size; i++) {
		if (str.data[i] == character) {
			STR_Range range = {prev, i};
			splits.data[splits.size++] = range;
			prev = i + 1;
		}
	}

	STR_Range range = {prev, str.size};
	splits.data[splits.size++] = range;
	STR_ProfExit();
	return splits;
}*/

STR_API STR STR_FloatToStr(STR_ARENA* arena, double value, int min_decimals) {
	char buffer[100];
	int size = STR_FloatToStr_(buffer, value, min_decimals);
	STR value_str = { buffer, size };
	return STR_Clone(arena, value_str);
}

STR_API STR STR_IntToStr(STR_ARENA* arena, int value) { return STR_IntToStrEx(arena, value, true, 10); }

STR_API STR STR_IntToStrEx(STR_ARENA* arena, uint64_t data, bool is_signed, int radix) {
	char buffer[100];
	int size = STR_IntToStr_(buffer, data, is_signed, radix);
	STR value_str = { buffer, size };
	return STR_Clone(arena, value_str);
}

STR_API int STR_IntToStr_(char* buffer, uint64_t data, bool is_signed, int radix) {
	STR_CHECK(radix >= 2 && radix <= 16);
	int offset = 0;

	uint64_t remaining = data;

	bool is_negative = is_signed && (int64_t)remaining < 0;
	if (is_negative) {
		remaining = -(int64_t)remaining; // Negate the value, then print it as it's now positive
	}

	for (;;) {
		uint64_t temp = remaining;
		remaining /= radix;

		uint64_t digit = temp - remaining * radix;
		buffer[offset++] = "0123456789abcdef"[digit];

		if (remaining == 0) break;
	}

	if (is_negative) buffer[offset++] = '-'; // Print minus sign

	// It's now printed in reverse, so let's reverse the digits
	int i = 0;
	int j = offset - 1;
	while (i < j) {
		char temp = buffer[j];
		buffer[j] = buffer[i];
		buffer[i] = temp;
		i++;
		j--;
	}
	return offset;
}

// Technique from: https://blog.benoitblanchon.fr/lightweight-float-to-string/
// - Supports printing up to nine digits after the decimal point
STR_API int STR_FloatToStr_(char* buffer, double value, int min_decimals) {
	int offset = 0;
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

	offset += STR_IntToStr_(buffer + offset, integralPart, false, 10);

	if (decimalPart || min_decimals > 0) { // Write decimals
		int width = 9;

		// remove trailing zeros
		while ((decimalPart % 10 == 0 && width > min_decimals) && width > 0) {
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

		int temp_str_len = (int)(temp_buffer + sizeof(temp_buffer) - temp_str);
		memcpy(buffer + offset, temp_str, temp_str_len), offset += temp_str_len;
	}

	if (exponent < 0) {
		memcpy(buffer + offset, "e-", 2), offset += 2;
		offset += STR_IntToStr_(buffer + offset, -exponent, false, 10);
	}

	if (exponent > 0) {
		memcpy(buffer + offset, "e", 1), offset += 1;
		offset += STR_IntToStr_(buffer + offset, exponent, false, 10);
	}
	return offset;
}

/*STR_API STR STR_JoinN(STR_ARENA *arena, STR_Array args) {
	STR_ProfEnter();
	int offset = 0;

	for (int i = 0; i < args.size; i++) {
		offset += args.data[i].size;
	}

	char *data = (char*)STR_ARENA_ALLOCATE(arena, offset + 1);

	offset = 0;
	for (int i = 0; i < args.size; i++) {
		memcpy(data + offset, args.data[i].data, args.data[i].size);
		offset += args.data[i].size;
	}

	*((char*)data + offset) = 0; // Add a null termination for better visualization in a debugger
	STR result = {data, offset};
	STR_ProfExit();
	return result;
}*/

static const uint32_t STR_UTF8_OFFSETS[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

STR_API int STR_CodepointSizeAsUTF8(uint32_t codepoint) {
	char _[4]; int size = STR_CodepointToUTF8(_, codepoint);
	return size;
}

STR_API int STR_CodepointToUTF8(char* output, uint32_t codepoint) {
	// Original implementation by Jeff Bezanson from https://www.cprogramming.com/tutorial/utf8.c (u8_wc_toutf8, public domain)
	STR_ProfEnter();
	uint32_t ch = codepoint;
	int result = 0;
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
	STR_ProfExit();
	return result;
}

STR_API uint32_t STR_UTF8ToCodepoint(STR str) {
	int offset = 0;
	return STR_NextCodepoint(str, &offset);
}

STR_API uint32_t STR_NextCodepoint(STR str, int* byteoffset) {
	// Original implementation by Jeff Bezanson from https://www.cprogramming.com/tutorial/unicode.html (u8_nextchar, public domain)
	if (*byteoffset >= str.size) return 0;

	uint32_t ch = 0;
	int sz = 0;
	do {
		ch <<= 6;
		ch += (unsigned char)str.data[(*byteoffset)++];
		sz++;
	} while (*byteoffset < str.size && !STR_IsUtf8FirstByte(str.data[*byteoffset]));
	ch -= STR_UTF8_OFFSETS[sz - 1];
	
	return (uint32_t)ch;
}

STR_API uint32_t STR_PrevCodepoint(STR str, int* byteoffset) {
	// See https://www.cprogramming.com/tutorial/unicode.html
	if (*byteoffset <= 0) return 0;

	(void)(STR_IsUtf8FirstByte(str.data[--(*byteoffset)]) ||
		STR_IsUtf8FirstByte(str.data[--(*byteoffset)]) ||
		STR_IsUtf8FirstByte(str.data[--(*byteoffset)]) || --(*byteoffset));

	int b = *byteoffset;
	return STR_NextCodepoint(str, &b);
}

STR_API int STR_CodepointCount(STR str) {
	STR_ProfEnter();
	int i = 0;
	for STR_Each(str, r, offset) {
		(void)offset;
		i++;
	}
	STR_ProfExit();
	return i;
}

// https://graphitemaster.github.io/aau/#unsigned-multiplication-can-overflow
inline bool DoesMulOverflow(uint64_t x, uint64_t y) { return y && x > ((uint64_t)-1) / y; }
inline bool DoesAddOverflow(uint64_t x, uint64_t y) { return x + y < x; }
//inline bool DoesSubUnderflow(uint64_t x, uint64_t y) { return x - y > x; }

STR_API bool STR_ParseU64Ex(STR s, int base, uint64_t* out_value) {
	STR_ProfEnter();
	STR_CHECK(2 <= base && base <= 16);

	uint64_t value = 0;
	int i = 0;
	for (; i < s.size; i++) {
		int c = s.data[i];
		if (c == '_') continue;

		if (c >= 'A' && c <= 'Z') c += 32; // ASCII convert to lowercase

		int digit;
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
	STR_ProfExit();
	return i == s.size;
}

STR_API bool STR_ParseI64(STR s, int64_t* out_value) {
	return STR_ParseI64Ex(s, 10, out_value);
}

STR_API bool STR_ParseI64Ex(STR s, int base, int64_t* out_value) {
	STR_ProfEnter();
	STR_CHECK(2 <= base && base <= 16);

	int64_t sign = 1;
	if (s.size > 0) {
		if (s.data[0] == '+') STR_Advance(&s, 1);
		else if (s.data[0] == '-') {
			STR_Advance(&s, 1);
			sign = -1;
		}
	}

	int64_t val;
	bool ok = STR_ParseU64Ex(s, base, (uint64_t*)&val) && val >= 0;
	if (ok) *out_value = val * sign;
	STR_ProfExit();
	return ok;
}

STR_API char* STR_Init(STR_ARENA* arena, STR s) {
	STR_ProfEnter();
	char* data = (char*)STR_ARENA_ALLOCATE(arena, s.size + 1);
	memcpy(data, s.data, s.size);
	data[s.size] = 0;
	STR_ProfExit();
	return data;
}

STR_API char* STR_InitTemp(STR s) {
	return STR_Init(STR_ACTIVE_TEMP_ARENA, s);
}

static double STR_ToFloat_(const char* str, int* success) {
	// Original implementation by Michael Hartmann https://github.com/michael-hartmann/parsefloat/blob/master/strtodouble.c (public domain)

	double intpart = 0, fracpart = 0;
	int sign = +1, size = 0, conversion = 0, exponent = 0;

	// skip whitespace
	//while(isspace(*str)) str++;

	// check for sign (optional; either + or -)
	if (*str == '-') {
		sign = -1;
		str++;
	}
	else if (*str == '+') str++;

	// check for nan and inf
	if (STR_CodepointToLower(str[0]) == 'n' && STR_CodepointToLower(str[1]) == 'a' && STR_CodepointToLower(str[2]) == 'n') {
		if (success != NULL) *success = 1;
		return NAN;
	}
	if (STR_CodepointToLower(str[0]) == 'i' && STR_CodepointToLower(str[1]) == 'n' && STR_CodepointToLower(str[2]) == 'f') {
		if (success != NULL) *success = 1;
		return sign * INFINITY;
	}

	// find number of digits before decimal point
	{
		const char* p = str;
		size = 0;
		while (*p >= '0' && *p <= '9') {
			p++;
			size++;
		}
	}

	if (size) conversion = 1;

	// convert intpart part of decimal point to a float
	{
		double f = 1;
		for (int i = 0; i < size; i++) {
			int v = str[size - 1 - i] - '0';
			intpart += v * f;
			f *= 10;
		}
		str += size;
	}

	// check for decimal point (optional)
	if (*str == '.') {
		const char* p = ++str;

		// find number of digits after decimal point
		size = 0;
		while (*p >= '0' && *p <= '9') {
			p++;
			size++;
		}

		if (size) conversion = 1;

		// convert fracpart part of decimal point to a float
		double f = 0.1;
		for (int i = 0; i < size; i++) {
			int v = str[i] - '0';
			fracpart += v * f;
			f *= 0.1;
		}

		str = p;
	}

	if (conversion && (*str == 'e' || *str == 'E')) {
		int expsign = +1;
		const char* p = ++str;

		if (*p == '+') p++;
		else if (*p == '-') {
			expsign = -1;
			p++;
		}

		str = p;
		size = 0;
		while (*p >= '0' && *p <= '9') {
			size++;
			p++;
		}

		int f = 1;
		for (int i = 0; i < size; i++) {
			int v = str[size - 1 - i] - '0';
			exponent += v * f;
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

STR_API bool STR_ParseFloat(STR s, double* out) {
	STR_ProfEnter();

	STR_CHECK(s.size < 63);
	char cstr[64] = {0};
	memcpy(cstr, s.data, s.size);

	// https://github.com/michael-hartmann/parsefloat/blob/master/strtodouble.c
	char* end;
	int success;
	*out = STR_ToFloat_(cstr, &success);

	STR_ProfExit();
	return success == 1;
}

STR_API STR STR_Replace(STR_ARENA* arena, STR str, STR search_for, STR replace_with) {
	if (search_for.size > str.size) return str;
	STR_ProfEnter();

	STR_Builder builder = { arena };

	int last = str.size - search_for.size;

	for (int i = 0; i <= last;) {
		if (memcmp(str.data + i, search_for.data, search_for.size) == 0) {
			STR_PrintV(&builder, replace_with);
			i += search_for.size;
		}
		else {
			STR char_str = { &str.data[i], 1 };
			STR_PrintV(&builder, char_str);
			i++;
		}
	}
	STR_ProfExit();
	return builder.str;
}

STR_API STR STR_ReplaceMulti(STR_ARENA* arena, STR str, STR_Array search_for, STR_Array replace_with) {
	STR_ProfEnter();
	STR_CHECK(search_for.size == replace_with.size);
	int n = search_for.size;

	STR_Builder builder = { arena };

	for (int i = 0; i < str.size;) {

		bool replaced = false;
		for (int j = 0; j < n; j++) {
			STR search_for_j = ((STR*)search_for.data)[j];
			if (i + search_for_j.size > str.size) continue;

			if (memcmp(str.data + i, search_for_j.data, search_for_j.size) == 0) {
				STR replace_with_j = ((STR*)replace_with.data)[j];
				STR_PrintV(&builder, replace_with_j);
				i += search_for_j.size;
				replaced = true;
				break;
			}
		}

		if (!replaced) {
			STR char_str = { &str.data[i], 1 };
			STR_PrintV(&builder, char_str);
			i++;
		}
	}
	STR_ProfExit();
	return builder.str;
}

STR_API STR STR_InitV(const char* s) {
	STR result = { (char*)s, (int)strlen(s) };
	return result;
}

STR_API STR STR_Advance(STR* str, int size) {
	STR_ProfEnter();
	STR result = STR_SliceBefore(*str, size);
	*str = STR_SliceAfter(*str, size);
	STR_ProfExit();
	return result;
}

STR_API uint32_t STR_CodepointToLower(uint32_t codepoint) { // TODO: handle unicode
	return codepoint >= 'A' && codepoint <= 'Z' ? codepoint + 32 : codepoint;
}

STR_API STR STR_ToLower(STR_ARENA* arena, STR str) {
	STR_ProfEnter();

	STR result = STR_Clone(arena, str);
	char* data = (char*)result.data; // normally, strings are const char*, but let's just ignore that here

	for (int i = 0; i < result.size; i++) {
		data[i] = STR_CodepointToLower(result.data[i]);
	}
	STR_ProfExit();
	return result;
}

STR_API void STR_SetTempArena(STR_ARENA* arena) {
	STR_ACTIVE_TEMP_ARENA = arena;
}

STR_API STR_ARENA* STR_GetTempArena() {
	return STR_ACTIVE_TEMP_ARENA;
}

STR_API void STR_PrintVA(STR_Builder* s, const char* fmt, va_list args) {
	for (const char* c = fmt; *c; c++) {
		if (*c == '%') {
			c++;

			// Parse size sub-specifiers
			int h = 0, l = 0;
			for (; *c == 'h'; ) { c++; h++; }
			for (; *c == 'l'; ) { c++; l++; }

			bool is_signed = false;
			int radix = 10;

			// Parse specifier
			switch (*c) {
			default: STR_CHECK(0);
			case 's': {
				STR_Print(s, va_arg(args, char*));
			} break;
			case 'v': {
				STR_PrintV(s, va_arg(args, STR));
			} break;
			case '?': {
				STR_Formatter* printer = va_arg(args, STR_Formatter*);
				printer->print(s, printer);
			} break;
			case '%': {
				STR_Print(s, "%");
			} break;
			case 'f': {
				char buffer[64];
				int size = STR_FloatToStr_(buffer, va_arg(args, double), 0);
				STR value_str = { buffer, size };
				STR_PrintV(s, value_str);
			} break;
			case 'x': { radix = 16;       goto print_int; }
			case 'd': { is_signed = true; goto print_int; }
			case 'i': { is_signed = true; goto print_int; }
			case 'u': {
			print_int:;
				uint64_t value;
				if (is_signed) {
					if (l == 1) { value = (uint64_t)va_arg(args, long); }
					else if (l == 2) { value = (uint64_t)va_arg(args, long long); }
					else if (h == 1) { value = (uint64_t)va_arg(args, short); }
					else if (h == 2) { value = (uint64_t)va_arg(args, char); }
					else { value = (uint64_t)va_arg(args, int); }
				}
				else {
					if (l == 1) { value = (uint64_t)va_arg(args, unsigned long); }
					else if (l == 2) { value = (uint64_t)va_arg(args, unsigned long long); }
					else if (h == 1) { value = (uint64_t)va_arg(args, unsigned short); }
					else if (h == 2) { value = (uint64_t)va_arg(args, unsigned char); }
					else { value = (uint64_t)va_arg(args, unsigned int); }
				}

				char buffer[100];
				int size = STR_IntToStr_(buffer, value, is_signed, radix);
				STR value_str = { buffer, size };
				STR_PrintV(s, value_str);
			} break;
			}
			continue;
		}

		STR character_str = { c, 1 };
		STR_PrintV(s, character_str);
	}
}

STR_API char* STR_Form(STR_ARENA* arena, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_Builder builder = { arena };
	STR_PrintVA(&builder, fmt, args);
	STR_PrintC(&builder, '\0');
	va_end(args);
	return (char*)builder.str.data;
}

STR_API char* STR_FormTemp(const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_Builder builder = { STR_ACTIVE_TEMP_ARENA };
	STR_PrintVA(&builder, fmt, args);
	STR_PrintC(&builder, '\0');
	va_end(args);
	return (char*)builder.str.data;
}

STR_API STR STR_FormV(STR_ARENA* arena, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_Builder builder = { arena };
	STR_PrintVA(&builder, fmt, args);
	va_end(args);
	return builder.str;
}

STR_API STR STR_FormTempV(const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_Builder builder = { STR_ACTIVE_TEMP_ARENA };
	STR_PrintVA(&builder, fmt, args);
	va_end(args);
	return builder.str;
}

STR_API void STR_Print(STR_Builder* s, const char* string) {
	STR str = { string, (int)strlen(string) };
	STR_PrintV(s, str);
}

STR_API void STR_PrintV(STR_Builder* s, STR string) {
	int capacity = s->capacity;
	int new_size = s->str.size + string.size;

	while (new_size > capacity) {
		capacity = capacity == 0 ? 8 : capacity * 2;
	}

	// Resize string if necessary
	if (capacity != s->capacity) {
		char* new_memory = (char*)STR_ARENA_ALLOCATE(s->arena, capacity);
		memcpy(new_memory, s->str.data, s->str.size);

		s->str.data = new_memory;
		s->capacity = capacity;
	}

	memcpy((char*)s->str.data + s->str.size, string.data, string.size);
	s->str.size = new_size;
}

STR_API void STR_PrintF(STR_Builder* s, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_PrintVA(s, fmt, args);
	va_end(args);
}

STR_API void STR_PrintC(STR_Builder* s, uint32_t codepoint) {
	char buffer[4];
	STR str = { buffer, STR_CodepointToUTF8(buffer, codepoint) };
	STR_PrintV(s, str);
}

#if 1 // OLD API

STR_API void OLD_PrintS(STR_Builder* s, STR str) {
	int capacity = s->capacity;
	int new_size = s->str.size + str.size;

	while (new_size > capacity) {
		capacity = capacity == 0 ? 8 : capacity * 2;
	}

	// Resize string if necessary
	if (capacity != s->capacity) {
		char* new_memory = (char*)STR_ARENA_ALLOCATE(s->arena, capacity);
		memcpy(new_memory, s->str.data, s->str.size);

		s->str.data = new_memory;
		s->capacity = capacity;
	}

	memcpy((char*)s->str.data + s->str.size, str.data, str.size);
	s->str.size = new_size;
}

STR_API void OLD_PrintB(STR_Builder* s, char b) {
	STR str = { &b, 1 };
	OLD_PrintS(s, str);
}

STR_API void OLD_PrintR(STR_Builder* s, uint32_t r) {
	char buffer[4];
	STR str = { buffer, STR_CodepointToUTF8(buffer, r) };
	OLD_PrintS(s, str);
}

STR_API void OLD_PrintSRepeat(STR_Builder* s, STR str, int count) {
	for (int i = 0; i < count; i++) OLD_PrintS(s, str);
}

STR_API void OLD_PrintC(STR_Builder* s, const char* c_str) {
	STR str = { (char*)c_str, (int)strlen(c_str) };
	OLD_PrintS(s, str);
}

STR_API STR OLD_APrint(STR_ARENA* arena, const char* fmt, ...) {
	STR_ProfEnter();

	va_list args;
	va_start(args, fmt);

	STR_Builder builder = { arena };
	OLD_PrintVA(&builder, fmt, args);

	va_end(args);
	STR_ProfExit();
	return builder.str;
}

// The format string is assumed to be valid, as currently no error handling is done.
STR_API void OLD_PrintVA(STR_Builder* s, const char* fmt, va_list args) {
	STR_ProfEnter();
	STR_CHECK(s->arena != NULL);
	for (const char* c = fmt; *c; c++) {
		if (*c == '~') {
			c++;
			if (*c == 0) break;

			switch (*c) {
			case 's': {
				OLD_PrintS(s, va_arg(args, STR));
			} break;

			case 'c': {
				OLD_PrintC(s, va_arg(args, char*));
			} break;

			case '~': {
				OLD_PrintC(s, "`~");
			} break;

			case 'f': {
				char buffer[64];
				int size = STR_FloatToStr_(buffer, va_arg(args, double), 0);
				STR value_str = { buffer, size };
				OLD_PrintS(s, value_str);
			} break;

			case 'x': // fallthrough
			case 'i': // fallthrough
			case 'u': {
				char format_c = *c;
				c++;
				if (*c == 0) break;

				uint64_t value = 0;
				switch (*c) {
				case '8': { value = va_arg(args, char); } break; // char
				case '1': { value = va_arg(args, uint16_t); } break; // U16
				case '3': { value = va_arg(args, uint32_t); } break; // uint32_t
				case '6': { value = va_arg(args, uint64_t); } break; // U16
				default: STR_CHECK(false);
				}

				if (*c != '8') { // char is the only one with only 1 character
					c++;
					if (*c == 0) break;
				}

				char buffer[100];
				int size = STR_IntToStr_(buffer, value, format_c == 'i', format_c == 'x' ? 16 : 10);
				STR value_str = { buffer, size };
				OLD_PrintS(s, value_str);
			} break;

			default: STR_CHECK(false);
			}
		}
		else {
			OLD_PrintB(s, *c);
		}
	}
	STR_ProfExit();
}

STR_API void OLD_Print(STR_Builder* s, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	OLD_PrintVA(s, fmt, args);
	va_end(args);
}
#endif // OLD API

#endif // FIRE_STRING_INCLUDED
// fire_string.h - by Eero Mutka (https://eeromutka.github.io/)
//
// Length-based string type and string utilities.
// 
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than STR_, simply do a find and replace in this file.
//

#ifndef FIRE_STRING_INCLUDED
#define FIRE_STRING_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>  // memcpy, memmove, memset, memcmp, strlen
#include <stdarg.h>
#include <math.h> // isnan, isinf

#ifdef STR_CUSTOM_MALLOC
// (Provide your own implementation before including this file)
#elif defined(FIRE_DS_INCLUDED) /* Use memory functions from fire_ds.h */
#define STR_MemAlloc(ALLOCATOR, SIZE) (void*)DS_MemAlloc((DS_Allocator*)ALLOCATOR, SIZE)
#define STR_MemFree(ALLOCATOR, PTR) DS_MemFree((DS_Allocator*)ALLOCATOR, (void*)(PTR))
#else
#include <stdlib.h>
#define STR_MemAlloc(ALLOCATOR, SIZE) (void*)malloc(SIZE)
#define STR_MemFree(ALLOCATOR, PTR) free((void*)(PTR))
#endif

typedef struct STR_View {
	const char* data;
	size_t size;
	
#ifdef __cplusplus
	STR_View() : data(0), size(0) {}
	STR_View(const char* _data, size_t _size) : data(_data), size(_size) {}
	STR_View(const char* c_string) : data(c_string), size(c_string ? strlen(c_string) : 0) {}
#endif
} STR_View;

#ifndef STR_API
#define STR_API static
#endif

#ifndef STR_ASSERT
#include <assert.h>
#define STR_ASSERT(x) assert(x)
#endif

#ifndef STR_PROFILER_MACROS
// Function-level profiler scope. A single function may only have one of these, and it should span the entire function.
#define STR_ProfEnter()
#define STR_ProfExit()
#endif

// String view literal macros
#ifdef __cplusplus
#define STR_V(x)    STR_View{(char*)x, sizeof(x)-1}
#define STR_VI(x)   STR_View{(char*)x, sizeof(x)-1}
#else
#define STR_V(x)  (STR_View){(char*)x, sizeof(x)-1}
#define STR_VI(x)           {(char*)x, sizeof(x)-1} // STR_View literal that works in C initializer lists
#endif

typedef struct STR_Builder {
	void* allocator;
	STR_View str;
	size_t capacity;
} STR_Builder;

typedef struct { STR_View* data; size_t size; } STR_Array;

#define STR_IsUtf8FirstByte(c) (((c) & 0xC0) != 0x80) /* is c the start of a utf8 sequence? */

//#define STR_Each(str, r, i) (size_t i=0, r = 0, i##_next=0; (r=STR_NextCodepoint(str, &i##_next)); i=i##_next)
//#define STR_EachReverse(str, r, i) (size_t i=str.size, r = 0; r=STR_PrevCodepoint(str, &i);)

typedef struct STR_Formatter {
	void (*print)(STR_Builder* s, struct STR_Formatter* self);
} STR_Formatter;

/*
-- printer API --
%s   -  c-string
%v   -  view-string (STR_View)
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

STR_API char* STR_ToC(void* allocator, STR_View str); // Allocates a new string
STR_API STR_View STR_ToV(const char* str); // Makes a string view into `str`

STR_API const char* STR_CloneC(void* allocator, const char* str); // Allocates a new string
STR_API STR_View STR_Clone(void* allocator, STR_View str); // Allocates a new string
STR_API void STR_Free(void* allocator, STR_View str);

STR_API char* STR_FormC(void* allocator, const char* fmt, ...); // Allocates a new string
STR_API STR_View STR_Form(void* allocator, const char* fmt, ...); // Allocates a new string

STR_API void STR_BuilderInit(STR_Builder* s, void* allocator); // Alternatively, init by {}-initializer and passing in the allocator field
STR_API void STR_BuilderDeinit(STR_Builder* s);

STR_API void STR_PrintC(STR_Builder* s, const char* string);
STR_API void STR_PrintV(STR_Builder* s, STR_View string);
STR_API void STR_PrintF(STR_Builder* s, const char* fmt, ...);
STR_API void STR_PrintVA(STR_Builder* s, const char* fmt, va_list args);
STR_API void STR_PrintU(STR_Builder* s, uint32_t codepoint); // Print unicode codepoint

// * Underscores are allowed and skipped
// * Works with any base up to 16 (i.e. binary, base-10, hex)
STR_API bool STR_ParseU64Ex(STR_View s, int base, uint64_t* out_value);
// #define STR_ToU64(s, out_value) STR_ParseU64Ex(s, 10, out_value)

// * A single minus or plus is accepted preceding the digits
// * Works with any base up to 16 (i.e. binary, base-10, hex)
// * Underscores are allowed and skipped
STR_API bool STR_ParseI64Ex(STR_View s, int base, int64_t* out_value);
STR_API bool STR_ParseI64(STR_View s, int64_t* out_value);

STR_API bool STR_ParseFloat(STR_View s, double* out);

STR_API STR_View STR_IntToStr(void* allocator, int value);
STR_API STR_View STR_IntToStrEx(void* allocator, uint64_t data, bool is_signed, int radix);
STR_API STR_View STR_FloatToStr(void* allocator, double value, int min_decimals);

STR_API size_t STR_CodepointToUTF8(char* output, uint32_t codepoint); // returns the number of bytes written
STR_API uint32_t STR_UTF8ToCodepoint(STR_View str);
STR_API size_t STR_CodepointSizeAsUTF8(uint32_t codepoint);

// Returns the character on `byteoffset`, then increments it.
// Will returns 0 if byteoffset >= str.size
STR_API uint32_t STR_NextCodepoint(STR_View str, size_t* byteoffset);

// Decrements `bytecounter`, then returns the character on it.
// Will returns 0 if byteoffset < 0
STR_API uint32_t STR_PrevCodepoint(STR_View str, size_t* byteoffset);

STR_API size_t STR_CodepointCount(STR_View str);

// -- String view utilities -------------

STR_API STR_View STR_Advance(STR_View* str, size_t size);

STR_API STR_View STR_BeforeFirst(STR_View str, uint32_t codepoint); // returns `str` if no codepoint is found
STR_API STR_View STR_BeforeLast(STR_View str, uint32_t codepoint);  // returns `str` if no codepoint is found
STR_API STR_View STR_AfterFirst(STR_View str, uint32_t codepoint);  // returns `str` if no codepoint is found
STR_API STR_View STR_AfterLast(STR_View str, uint32_t codepoint);   // returns `str` if no codepoint is found

STR_API bool STR_Find(STR_View str, STR_View substr, size_t* out_offset);
STR_API bool STR_FindC(STR_View str, const char* substr, size_t* out_offset);
STR_API STR_View STR_ParseUntilAndSkip(STR_View* remaining, uint32_t codepoint); // cuts forward until `codepoint` or end of the string is reached and an empty string is returned
STR_API bool STR_FindFirst(STR_View str, uint32_t codepoint, size_t* out_offset);
STR_API bool STR_FindLast(STR_View str, uint32_t codepoint, size_t* out_offset);
STR_API bool STR_LastIdxOfAnyChar(STR_View str, STR_View chars, size_t* out_index);
STR_API bool STR_Contains(STR_View str, STR_View substr);
STR_API bool STR_ContainsC(STR_View str, const char* substr);
STR_API bool STR_ContainsU(STR_View str, uint32_t codepoint);

STR_API STR_View STR_Replace(void* allocator, STR_View str, STR_View search_for, STR_View replace_with);
STR_API STR_View STR_ReplaceMulti(void* allocator, STR_View str, STR_Array search_for, STR_Array replace_with);
STR_API STR_View STR_ToLower(void* allocator, STR_View str);
STR_API uint32_t STR_CodepointToLower(uint32_t codepoint);

STR_API bool STR_EndsWith(STR_View str, STR_View end);
STR_API bool STR_EndsWithC(STR_View str, const char* end);
STR_API bool STR_StartsWith(STR_View str, STR_View start);
STR_API bool STR_StartsWithC(STR_View str, const char* start);
STR_API bool STR_CutEnd(STR_View* str, STR_View end);
STR_API bool STR_CutEndC(STR_View* str, const char* end);
STR_API bool STR_CutStart(STR_View* str, STR_View start);
STR_API bool STR_CutStartC(STR_View* str, const char* start);

STR_API bool STR_Match(STR_View a, STR_View b);
STR_API bool STR_MatchC(const char* a, const char* b);
STR_API bool STR_MatchCaseInsensitive(STR_View a, STR_View b);
STR_API bool STR_MatchCaseInsensitiveC(const char* a, const char* b);

STR_API STR_View STR_Slice(STR_View str, size_t lo, size_t hi);
STR_API STR_View STR_SliceBefore(STR_View str, size_t mid);
STR_API STR_View STR_SliceAfter(STR_View str, size_t mid);

// -- IMPLEMENTATION ------------------------------------------------------------

typedef struct STR_ASCIISet {
	char bytes[32];
} STR_ASCIISet;

static STR_ASCIISet STR_ASCIISetMake(STR_View chars) {
	STR_ProfEnter();
	STR_ASCIISet set = {0};
	for (size_t i = 0; i < chars.size; i++) {
		char c = chars.data[i];
		set.bytes[c / 8] |= 1 << (c % 8);
	}
	STR_ProfExit();
	return set;
}

static bool STR_ASCIISetContains(STR_ASCIISet set, char c) {
	return (set.bytes[c / 8] & 1 << (c % 8)) != 0;
}

STR_API STR_View STR_ParseUntilAndSkip(STR_View* remaining, uint32_t codepoint) {
	STR_View line = { remaining->data, 0 };

	for (;;) {
		size_t cp_size = 0;
		uint32_t cp = STR_NextCodepoint(*remaining, &cp_size);
		if (cp == 0) break;

		remaining->data += cp_size;
		remaining->size -= cp_size;

		if (cp == codepoint) break;
		line.size += cp_size;
	}

	return line;
}

STR_API bool STR_FindFirst(STR_View str, uint32_t codepoint, size_t* out_offset) {
	size_t i = 0, i_next = 0;
	for (uint32_t r; r = STR_NextCodepoint(str, &i_next); i = i_next) {
		if (r == codepoint) {
			*out_offset = i;
			return true;
		}
	}
	return false;
}

STR_API bool STR_FindLast(STR_View str, uint32_t codepoint, size_t* out_offset) {
	size_t i = str.size;
	for (uint32_t r; r = STR_PrevCodepoint(str, &i);) {
		if (r == codepoint) {
			*out_offset = i;
			return true;
		}
	}
	return false;
}

STR_API bool STR_LastIdxOfAnyChar(STR_View str, STR_View chars, size_t* out_index) {
	STR_ProfEnter();
	STR_ASCIISet char_set = STR_ASCIISetMake(chars);

	bool found = false;
	for (intptr_t i = str.size - 1; i >= 0; i--) {
		if (STR_ASCIISetContains(char_set, str.data[i])) {
			*out_index = i;
			found = true;
			break;
		}
	}

	STR_ProfExit();
	return found;
}

STR_API STR_View STR_BeforeFirst(STR_View str, uint32_t codepoint) {
	size_t offset = str.size;
	STR_FindFirst(str, codepoint, &offset);
	STR_View result = { str.data, offset };
	return result;
}

STR_API STR_View STR_BeforeLast(STR_View str, uint32_t codepoint) {
	size_t offset = str.size;
	STR_FindLast(str, codepoint, &offset);
	STR_View result = { str.data, offset };
	return result;
}

STR_API STR_View STR_AfterFirst(STR_View str, uint32_t codepoint) {
	size_t offset = str.size;
	STR_View result = str;
	if (STR_FindFirst(str, codepoint, &offset)) {
		result = STR_SliceAfter(str, offset + STR_CodepointSizeAsUTF8(codepoint));
	}
	return result;
}

STR_API STR_View STR_AfterLast(STR_View str, uint32_t codepoint) {
	size_t offset = str.size;
	STR_View result = str;
	if (STR_FindLast(str, codepoint, &offset)) {
		result = STR_SliceAfter(str, offset + STR_CodepointSizeAsUTF8(codepoint));
	}
	return result;
}

STR_API bool STR_Match(STR_View a, STR_View b) {
	return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

STR_API bool STR_MatchCaseInsensitive(STR_View a, STR_View b) {
	STR_ProfEnter();
	bool equals = a.size == b.size;
	if (equals) {
		for (size_t i = 0; i < a.size; i++) {
			if (STR_CodepointToLower(a.data[i]) != STR_CodepointToLower(b.data[i])) {
				equals = false;
				break;
			}
		}
	}
	STR_ProfExit();
	return equals;
}

STR_API bool STR_MatchC(const char* a, const char* b) {
	return strcmp(a, b) == 0;
}

STR_API bool STR_MatchCaseInsensitiveC(const char* a, const char* b) {
	return STR_MatchCaseInsensitive(STR_ToV(a), STR_ToV(b));
}

STR_API STR_View STR_Slice(STR_View str, size_t lo, size_t hi) {
	STR_ASSERT(hi >= lo && hi <= str.size);
	STR_View result = { str.data + lo, hi - lo };
	return result;
}

STR_API STR_View STR_SliceAfter(STR_View str, size_t mid) {
	STR_ASSERT(mid <= str.size);
	STR_View result = { str.data + mid, str.size - mid };
	return result;
}

STR_API STR_View STR_SliceBefore(STR_View str, size_t mid) {
	STR_ASSERT(mid <= str.size);
	STR_View result = { str.data, mid };
	return result;
}

STR_API bool STR_ContainsC(STR_View str, const char* substr) {
	return STR_FindC(str, substr, NULL);
}

STR_API bool STR_Contains(STR_View str, STR_View substr) {
	return STR_Find(str, substr, NULL);
}

STR_API bool STR_Find(STR_View str, STR_View substr, size_t* out_offset) {
	STR_ProfEnter();
	bool found = false;
	size_t i_last = str.size - substr.size;
	for (size_t i = 0; i <= i_last; i++) {
		if (STR_Match(STR_Slice(str, i, i + substr.size), substr)) {
			if (out_offset) *out_offset = i;
			found = true;
			break;
		}
	}
	STR_ProfExit();
	return found;
};

STR_API bool STR_FindC(STR_View str, const char* substr, size_t* out_offset) {
	STR_View substr_v = STR_ToV(substr);
	return STR_Find(str, substr_v, out_offset);
};

STR_API bool STR_ContainsU(STR_View str, uint32_t codepoint) {
	size_t i = 0;
	for (uint32_t r; r = STR_NextCodepoint(str, &i);) {
		if (r == codepoint) return true;
	}
	return false;
}

STR_API bool STR_EndsWithC(STR_View str, const char* end) { return STR_EndsWith(str, STR_ToV(end)); }
STR_API bool STR_StartsWithC(STR_View str, const char* start) { return STR_StartsWith(str, STR_ToV(start)); }
STR_API bool STR_CutEndC(STR_View* str, const char* end) { return STR_CutEnd(str, STR_ToV(end)); }
STR_API bool STR_CutStartC(STR_View* str, const char* start) { return STR_CutStart(str, STR_ToV(start)); }

STR_API bool STR_EndsWith(STR_View str, STR_View end) {
	return str.size >= end.size && STR_Match(end, STR_SliceAfter(str, str.size - end.size));
}

STR_API bool STR_StartsWith(STR_View str, STR_View start) {
	return str.size >= start.size && STR_Match(start, STR_SliceBefore(str, start.size));
}

STR_API bool STR_CutEnd(STR_View* str, STR_View end) {
	STR_ProfEnter();
	bool ends_with = STR_EndsWith(*str, end);
	if (ends_with) {
		str->size -= end.size;
	}
	STR_ProfExit();
	return ends_with;
}

STR_API bool STR_CutStart(STR_View* str, STR_View start) {
	STR_ProfEnter();
	bool starts_with = STR_StartsWith(*str, start);
	if (starts_with) {
		*str = STR_SliceAfter(*str, start.size);
	}
	STR_ProfExit();
	return starts_with;
}

STR_API size_t STR_IntToStrBuf(char* buffer, uint64_t data, bool is_signed, int radix) {
	STR_ASSERT(radix >= 2 && radix <= 16);
	size_t offset = 0;

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
	intptr_t i = 0;
	intptr_t j = offset - 1;
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
STR_API size_t STR_FloatToStrBuf(char* buffer, double value, int min_decimals) {
	size_t offset = 0;
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

	offset += STR_IntToStrBuf(buffer + offset, integralPart, false, 10);

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

		size_t temp_str_len = (size_t)(temp_buffer + sizeof(temp_buffer) - temp_str);
		memcpy(buffer + offset, temp_str, temp_str_len), offset += temp_str_len;
	}

	if (exponent < 0) {
		memcpy(buffer + offset, "e-", 2), offset += 2;
		offset += STR_IntToStrBuf(buffer + offset, -exponent, false, 10);
	}

	if (exponent > 0) {
		memcpy(buffer + offset, "e", 1), offset += 1;
		offset += STR_IntToStrBuf(buffer + offset, exponent, false, 10);
	}
	return offset;
}

STR_API STR_View STR_FloatToStr(void* allocator, double value, int min_decimals) {
	char buffer[100];
	size_t size = STR_FloatToStrBuf(buffer, value, min_decimals);
	STR_View value_str = { buffer, size };
	return STR_Clone(allocator, value_str);
}

STR_API STR_View STR_IntToStr(void* allocator, int value) { return STR_IntToStrEx(allocator, value, true, 10); }

STR_API STR_View STR_IntToStrEx(void* allocator, uint64_t data, bool is_signed, int radix) {
	char buffer[100];
	size_t size = STR_IntToStrBuf(buffer, data, is_signed, radix);
	STR_View value_str = { buffer, size };
	return STR_Clone(allocator, value_str);
}

static const uint32_t STR_UTF8_OFFSETS[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

STR_API size_t STR_CodepointSizeAsUTF8(uint32_t codepoint) {
	char _[4];
	return STR_CodepointToUTF8(_, codepoint);
}

STR_API size_t STR_CodepointToUTF8(char* output, uint32_t codepoint) {
	// CREDIT: Jeff Bezanson (https://www.cprogramming.com/tutorial/utf8.c, u8_wc_toutf8; public domain)
	
	STR_ProfEnter();
	uint32_t ch = codepoint;
	size_t result = 0;
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

STR_API uint32_t STR_UTF8ToCodepoint(STR_View str) {
	size_t offset = 0;
	return STR_NextCodepoint(str, &offset);
}

STR_API uint32_t STR_NextCodepoint(STR_View str, size_t* byteoffset) {
	// CREDIT: Jeff Bezanson (https://www.cprogramming.com/tutorial/unicode.html, u8_nextchar; public domain)
	if (*byteoffset >= str.size) return 0;

	uint32_t ch = 0;
	size_t sz = 0;
	do {
		ch <<= 6;
		ch += (unsigned char)str.data[(*byteoffset)++];
		sz++;
	} while (*byteoffset < str.size && !STR_IsUtf8FirstByte(str.data[*byteoffset]));
	ch -= STR_UTF8_OFFSETS[sz - 1];

	return (uint32_t)ch;
}

STR_API uint32_t STR_PrevCodepoint(STR_View str, size_t* byteoffset) {
	// See https://www.cprogramming.com/tutorial/unicode.html
	if (*byteoffset <= 0) return 0;

	(void)(STR_IsUtf8FirstByte(str.data[--(*byteoffset)]) ||
		STR_IsUtf8FirstByte(str.data[--(*byteoffset)]) ||
		STR_IsUtf8FirstByte(str.data[--(*byteoffset)]) || --(*byteoffset));

	size_t b = *byteoffset;
	return STR_NextCodepoint(str, &b);
}

STR_API size_t STR_CodepointCount(STR_View str) {
	STR_ProfEnter();
	size_t i = 0;
	for (uint32_t r; r = STR_NextCodepoint(str, &i);) {
		i++;
	}
	STR_ProfExit();
	return i;
}

// https://graphitemaster.github.io/aau/#unsigned-multiplication-can-overflow
inline bool DoesMulOverflow(uint64_t x, uint64_t y) { return y && x > ((uint64_t)-1) / y; }
inline bool DoesAddOverflow(uint64_t x, uint64_t y) { return x + y < x; }

STR_API bool STR_ParseU64Ex(STR_View s, int base, uint64_t* out_value) {
	STR_ProfEnter();
	STR_ASSERT(2 <= base && base <= 16);

	uint64_t value = 0;
	size_t i = 0;
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

STR_API bool STR_ParseI64(STR_View s, int64_t* out_value) {
	return STR_ParseI64Ex(s, 10, out_value);
}

STR_API bool STR_ParseI64Ex(STR_View s, int base, int64_t* out_value) {
	STR_ProfEnter();
	STR_ASSERT(2 <= base && base <= 16);

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

STR_API const char* STR_CloneC(void* allocator, const char* str) {
	size_t size = strlen(str) + 1;
	char* data = (char*)STR_MemAlloc(allocator, size);
	strcpy_s(data, size, str);
	return data;
}

STR_API char* STR_ToC(void* allocator, STR_View str) {
	STR_ProfEnter();
	char* data = (char*)STR_MemAlloc(allocator, str.size + 1);
	memcpy(data, str.data, str.size);
	data[str.size] = 0;
	STR_ProfExit();
	return data;
}

STR_API STR_View STR_ToV(const char* s) {
	STR_View result = { (char*)s, strlen(s) };
	return result;
}

STR_API STR_View STR_Clone(void* allocator, STR_View str) {
	char* data = (char*)STR_MemAlloc(allocator, str.size);
	memcpy(data, str.data, str.size);
	STR_View result = { data, str.size };
	return result;
}

STR_API void STR_Free(void* allocator, STR_View str) {
	STR_MemFree(allocator, str.data);
}

static double STR_ToFloat_(const char* str, int* success) {
	// CREDIT: Michael Hartmann (https://github.com/michael-hartmann/parsefloat/blob/master/strtodouble.c; public domain)

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
		for (size_t i = 0; i < size; i++) {
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
		for (size_t i = 0; i < size; i++) {
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
		for (size_t i = 0; i < size; i++) {
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

STR_API bool STR_ParseFloat(STR_View s, double* out) {
	STR_ProfEnter();

	STR_ASSERT(s.size < 63);
	char cstr[64] = {0};
	memcpy(cstr, s.data, s.size);

	int success;
	*out = STR_ToFloat_(cstr, &success);

	STR_ProfExit();
	return success == 1;
}

STR_API STR_View STR_Replace(void* allocator, STR_View str, STR_View search_for, STR_View replace_with) {
	if (search_for.size > str.size) return str;
	STR_ProfEnter();

	STR_Builder builder = { allocator };

	size_t last = str.size - search_for.size;
	for (size_t i = 0; i <= last;) {
		if (memcmp(str.data + i, search_for.data, search_for.size) == 0) {
			STR_PrintV(&builder, replace_with);
			i += search_for.size;
		}
		else {
			STR_View char_str = { &str.data[i], 1 };
			STR_PrintV(&builder, char_str);
			i++;
		}
	}
	STR_ProfExit();
	return builder.str;
}

STR_API STR_View STR_ReplaceMulti(void* allocator, STR_View str, STR_Array search_for, STR_Array replace_with) {
	STR_ProfEnter();
	STR_ASSERT(search_for.size == replace_with.size);
	size_t n = search_for.size;

	STR_Builder builder = { allocator };

	for (size_t i = 0; i < str.size;) {

		bool replaced = false;
		for (size_t j = 0; j < n; j++) {
			STR_View search_for_j = ((STR_View*)search_for.data)[j];
			if (i + search_for_j.size > str.size) continue;

			if (memcmp(str.data + i, search_for_j.data, search_for_j.size) == 0) {
				STR_View replace_with_j = ((STR_View*)replace_with.data)[j];
				STR_PrintV(&builder, replace_with_j);
				i += search_for_j.size;
				replaced = true;
				break;
			}
		}

		if (!replaced) {
			STR_View char_str = { &str.data[i], 1 };
			STR_PrintV(&builder, char_str);
			i++;
		}
	}
	STR_ProfExit();
	return builder.str;
}

STR_API STR_View STR_Advance(STR_View* str, size_t size) {
	STR_ProfEnter();
	STR_View result = STR_SliceBefore(*str, size);
	*str = STR_SliceAfter(*str, size);
	STR_ProfExit();
	return result;
}

STR_API uint32_t STR_CodepointToLower(uint32_t codepoint) { // TODO: handle unicode
	return codepoint >= 'A' && codepoint <= 'Z' ? codepoint + 32 : codepoint;
}

STR_API STR_View STR_ToLower(void* allocator, STR_View str) {
	STR_ProfEnter();

	STR_View result = STR_Clone(allocator, str);
	char* data = (char*)result.data; // normally, strings are const char*, but let's just ignore that here

	for (size_t i = 0; i < result.size; i++) {
		data[i] = STR_CodepointToLower(result.data[i]);
	}
	STR_ProfExit();
	return result;
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
			default: STR_ASSERT(0);
			case 's': {
				STR_PrintC(s, va_arg(args, char*));
			} break;
			case 'v': {
				STR_PrintV(s, va_arg(args, STR_View));
			} break;
			case '?': {
				STR_Formatter* printer = va_arg(args, STR_Formatter*);
				printer->print(s, printer);
			} break;
			case '%': {
				STR_PrintC(s, "%");
			} break;
			case 'f': {
				char buffer[64];
				size_t size = STR_FloatToStrBuf(buffer, va_arg(args, double), 0);
				STR_View value_str = { buffer, size };
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
				size_t size = STR_IntToStrBuf(buffer, value, is_signed, radix);
				STR_View value_str = { buffer, size };
				STR_PrintV(s, value_str);
			} break;
			}
			continue;
		}

		STR_View character_str = { c, 1 };
		STR_PrintV(s, character_str);
	}
}

STR_API char* STR_FormC(void* allocator, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_Builder builder = { allocator };
	STR_PrintVA(&builder, fmt, args);
	STR_PrintU(&builder, '\0');
	va_end(args);
	return (char*)builder.str.data;
}

STR_API STR_View STR_Form(void* allocator, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	STR_Builder builder = { allocator };
	STR_PrintVA(&builder, fmt, args);
	va_end(args);
	return builder.str;
}

STR_API void STR_BuilderInit(STR_Builder* s, void* allocator) {
	STR_Builder result = {allocator};
	*s = result;
}

STR_API void STR_BuilderDeinit(STR_Builder* s) {
	STR_Free(s->allocator, s->str);
}

STR_API void STR_PrintC(STR_Builder* s, const char* string) {
	STR_View str = { string, strlen(string) };
	STR_PrintV(s, str);
}

STR_API void STR_PrintV(STR_Builder* s, STR_View string) {
	size_t capacity = s->capacity;
	size_t new_size = s->str.size + string.size;

	while (new_size > capacity) {
		capacity = capacity == 0 ? 8 : capacity * 2;
	}

	// Resize string if necessary
	if (capacity != s->capacity) {
		char* new_memory = (char*)STR_MemAlloc(s->allocator, capacity);
		memcpy(new_memory, s->str.data, s->str.size);
		STR_MemFree(s->allocator, s->str.data);
		
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

STR_API void STR_PrintU(STR_Builder* s, uint32_t codepoint) {
	char buffer[4];
	STR_View str = { buffer, STR_CodepointToUTF8(buffer, codepoint) };
	STR_PrintV(s, str);
}

#endif // FIRE_STRING_INCLUDED
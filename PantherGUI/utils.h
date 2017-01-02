#pragma once

#include <cstdio>
#include <string>

#ifdef WIN32
// on Windows, the standard assert does not immediately break into code
// instead it shows an annoying dialog box and does NOT pause exceution
// this can result in many difficulties when dealing with multiple threads
// we REALLY want assert to stop execution, so we overwrite the assert behavior
#ifdef assert
#undef assert
#endif
#define assert(expression) if (!(expression)) { __debugbreak(); }
#else
#include <cassert>
#endif

typedef enum {
	PGDirectionLeft,
	PGDirectionRight
} PGDirection;

typedef long long lng;
typedef size_t ulng;

struct PGUTF8Character {
	char length;
	char character[4];
};

#define NEWLINE_CHARACTER "\r\n"

#define DOUBLE_CLICK_TIME 400

namespace panther {
	template<class T>
	T abs(T t1) {
		return t1 < 0 ? t1 * -1 : t1;
	}

	char* strdup(const char* source);
}

template< typename... Args >
std::string string_sprintf(const char* format, Args... args) {
	int length = std::snprintf( nullptr, 0, format, args... );
	assert( length >= 0 );

	char* buf = new char[length + 1];
	std::snprintf( buf, length + 1, format, args... );

	std::string str( buf );
	delete[] buf;
	return std::move(str);
}


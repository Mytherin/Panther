#pragma once

#include <cstdio>
#include <string>
#include "assert.h"

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

	std::string tolower(std::string str);
	std::string toupper(std::string str);

	void replace(std::string& source, std::string from, std::string to);

	bool epsilon_equals(double a, double b);
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

std::string StripQuotes(std::string str);

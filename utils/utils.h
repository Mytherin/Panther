#pragma once

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <cstdio>
#include <string>
#include "assert.h"
#include <vector>
#include <float.h>

typedef unsigned char byte;

typedef enum {
	PGDirectionLeft,
	PGDirectionRight
} PGDirection;

typedef long long lng;
typedef size_t ulng;

typedef float PGScalar;

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
	template<class T>
	T clamp(T t1, T min, T max) {
		if (min > max) return clamp(t1, max, min);
		return t1 < min ? min : (t1 > max ? max : t1);
	}
	template<class T>
	T min(T a, T b) {
		return a < b ? a : b;
	}
	template<class T>
	T max(T a, T b) {
		return a > b ? a : b;
	}

	char* strdup(const char* source);

	std::vector<std::string> split(std::string, char delimiter);
	std::string ltrim(std::string);
	std::string rtrim(std::string);
	std::string trim(std::string);
	std::string tolower(std::string str);
	std::string toupper(std::string str);
	unsigned char chartolower(unsigned char character);
	unsigned char chartoupper(unsigned char character);
	unsigned char is_digit(unsigned char character);
	void* memrchr(const void* s, int c, size_t n);

	void replace(std::string& source, std::string from, std::string to);

	bool epsilon_equals(double a, double b);

	lng strtolng(const char* text, char** end, size_t length);
	inline lng strtolng(const char* text, size_t length) {
		return strtolng(text, nullptr, length);
	}
	inline lng strtolng(const char* text, char** end) {
		return strtolng(text, end, strlen(text));
	}
	inline lng strtolng(std::string text) {
		return strtolng(text.c_str(), text.size());
	}
	inline lng strtolng(const char* text) {
		return strtolng(text, strlen(text));
	}
	double strtodbl(const char* text, char** end, size_t length);
	inline double strtodbl(const char* text, size_t length) {
		return strtodbl(text, nullptr, length);
	}
	inline double strtodbl(const char* text, char** end) {
		return strtodbl(text, end, strlen(text));
	}
	inline double strtodbl(std::string text) {
		return strtodbl(text.c_str(), text.size());
	}
	inline double strtodbl(const char* text) {
		return strtodbl(text, strlen(text));
	}


	template<class T>
	T clamped_access(const std::vector<T>& vector, lng index) {
		return vector[std::min((lng)vector.size() - 1, std::max((lng)0, index))];
	}
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

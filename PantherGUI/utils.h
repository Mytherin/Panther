#pragma once

#include <assert.h>

typedef enum {
	PGDirectionLeft,
	PGDirectionRight
} PGDirection;

typedef long long lng;

struct PGUTF8Character {
	char length;
	char character[4];
};

#define NEWLINE_CHARACTER "\r\n"

#define DOUBLE_CLICK_TIME 200

namespace PG {
	template<class T>
	T abs(T t1) {
	    return t1 < 0 ? t1 * -1 : t1;
	}
}
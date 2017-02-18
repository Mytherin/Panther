
#include "windowfunctions.h"
#include <algorithm>


bool PGRectangleContains(PGRect rect, PGPoint point) {
	return point.x >= rect.x &&
		point.y >= rect.y &&
		point.x <= (rect.x + rect.width) &&
		point.y <= (rect.y + rect.height);
}

bool PGRectangleContains(PGIRect rect, PGPoint point) {
	return point.x >= rect.x &&
		point.y >= rect.y &&
		point.x <= (rect.x + rect.width) &&
		point.y <= (rect.y + rect.height);
}

bool PGIRectanglesOverlap(PGIRect a, PGIRect b) {
	return (a.x <= b.x + b.width && a.x + a.width >= b.x && a.y <= b.y + b.height && a.y + a.height >= b.y);
}

PGRect::PGRect(PGIRect rect) : x(rect.x), y(rect.y), width(rect.width), height(rect.height) {
}

std::string StripQuotes(std::string str) {
	if (str[0] == '"')
		str = str.substr(1);
	if (str[str.size() - 1] == '"')
		str = str.substr(0, str.size() - 1);
	return str;
}

namespace panther {
	void strcpy(char* destination, char* source) {
		while (*source) {
			*destination = *source;
			destination++;
			source++;
		}
		*destination = '\0';
	}

	char* strdup(const char* source) {
		char* result = (char*)malloc(strlen(source));
		strcpy(result, (char*)source);
		return result;
	}


	unsigned char chartolower(unsigned char character) {
		if ('A' <= character && character <= 'Z')
			character += 'a' - 'A';
		return character;
	}

	unsigned char chartoupper(unsigned char character) {
		if ('a' <= character && character <= 'z')
			character += 'A' - 'a';
		return character;
	}

	unsigned char is_digit(unsigned char character) {
		return character >= '0' && character <= '9';
	}

	std::string tolower(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}
	std::string toupper(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	void replace(std::string& str, std::string from, std::string to) {
		if (from.empty())
			return;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}

	bool epsilon_equals(double a, double b) {
		return abs(a - b) < 0.00001;
	}

	lng strtolng(const char* text, char**end, size_t length) {
		long converted = strtol(text, end, 10);
		return (lng)converted;
	}

	double strtodbl(const char* text, char**end, size_t length) {
		double d = strtod(text, end);
		return d;
	}

	void* memrchr(const void* s, int c, size_t n) {
		const unsigned char* p = (const unsigned char*)s;
		for (p += n; n > 0; n--)
			if (*--p == c)
				return (void*)p;

		return NULL;
	}
}


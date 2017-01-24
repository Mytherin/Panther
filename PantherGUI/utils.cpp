
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
			source++;
		}
	}

	char* strdup(const char* source) {
		char* result = (char*) malloc(strlen(source));
		strcpy(result, (char*) source);
		return result;
	}

	std::string tolower(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}
	std::string toupper(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	bool epsilon_equals(double a, double b) {
		return abs(a - b) < 0.00001;
	}
}


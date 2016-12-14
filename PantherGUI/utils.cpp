
#include "windowfunctions.h"


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
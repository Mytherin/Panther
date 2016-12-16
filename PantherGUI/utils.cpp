
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

bool PGIRectanglesOverlap(PGIRect a, PGIRect b) {
	return (a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y);
}
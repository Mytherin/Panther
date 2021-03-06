#pragma once

#include "container.h"

#define TOOLBAR_HEIGHT 24

class PGToolbar : public PGContainer {
public:
	PGToolbar(PGWindowHandle window);
	~PGToolbar();

	void Initialize();

	PGCursorType GetCursor(PGPoint mouse);

	void Draw(PGRendererHandle);
private:
	PGFontHandle font;
};
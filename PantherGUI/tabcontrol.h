
#pragma once

#include "control.h"

class TabControl : public Control {
public:
	TabControl(PGWindowHandle window);

	void Draw(PGRendererHandle, PGIRect*);
protected:
	PGScalar tab_offset;
};

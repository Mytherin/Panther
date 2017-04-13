#pragma once

#include "container.h"

class PGToolbar : public PGContainer {
public:
	PGToolbar(PGWindowHandle window);
	~PGToolbar();

	void Draw(PGRendererHandle);
};
#pragma once

#include "windowfunctions.h"
#include "utils.h"

class Control {
public:
	int x, y;
	int width, height;

	Control(PGWindowHandle window);

	virtual void MouseWheel(int x, int y, int distance, PGModifier modifier);
	virtual void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void KeyboardButton(PGButton button, PGModifier modifier);
	virtual void KeyboardCharacter(char character, PGModifier modifier);
	virtual void KeyboardUnicode(char* character, PGModifier modifier);
	virtual void Draw(PGRendererHandle, PGRect*);

	virtual void Invalidate();
	virtual void Invalidate(PGRect);
protected:
	PGWindowHandle window;
	bool HasFocus();
};
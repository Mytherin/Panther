#pragma once

#include "windowfunctions.h"
#include "utils.h"

class Control {
public:
	PGScalar x, y;
	PGScalar width, height;

	Control(PGWindowHandle window);

	virtual void MouseWheel(int x, int y, int distance, PGModifier modifier);
	virtual void KeyboardButton(PGButton button, PGModifier modifier);
	virtual void KeyboardCharacter(char character, PGModifier modifier);
	virtual void KeyboardUnicode(char* character, PGModifier modifier);
	virtual void PeriodicRender(void);
	virtual void Draw(PGRendererHandle, PGIRect*);

	virtual void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	virtual void Invalidate();
	virtual void Invalidate(PGIRect);
	virtual void Invalidate(PGRect);
protected:
	PGWindowHandle window;
	bool HasFocus();
};
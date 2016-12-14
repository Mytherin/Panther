
#pragma once

#include "control.h"
#include "textfield.h"

class TabControl : public Control {
public:
	TabControl(PGWindowHandle window, TextField* textfield);

	bool KeyboardButton(PGButton button, PGModifier modifier);
	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	void Draw(PGRendererHandle, PGIRect*);

	void PrevTab();
	void NextTab();
	void CloseTab();
protected:
	TextField* textfield;

	void SwitchToFile(TextFile* file);

	PGScalar tab_offset;
	int active_tab;
};


#pragma once

#include "control.h"
#include "textfield.h"

struct Tab {
	TextFile* file;
	PGScalar width;
	PGScalar x;
	PGScalar target_x;

	Tab(TextFile* file) : file(file), x(-1), target_x(-1) { }
};

class TabControl : public Control {
public:
	TabControl(PGWindowHandle window, TextField* textfield);

	void PeriodicRender(void);

	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	void Draw(PGRendererHandle, PGIRect*);

	void PrevTab();
	void NextTab();
	void CloseTab(int tab);

	bool IsDragging() {
		return drag_tab;
	}
protected:
	PGFontHandle font;

	int GetSelectedTab(int x);

	std::vector<Tab> tabs;

	TextField* textfield;

	void SwitchToFile(TextFile* file);

	PGScalar tab_offset;
	int active_tab;

	bool drag_tab = false;
	PGScalar drag_offset;
};

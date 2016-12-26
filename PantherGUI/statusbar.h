#pragma once

#include "button.h"
#include "control.h"
#include "textfield.h"

#define STATUSBAR_HEIGHT 20

class StatusBar : public Control {
public:
	StatusBar(PGWindowHandle window, TextField* textfield);
	~StatusBar();

	void UpdateParentSize(PGSize old_size, PGSize new_size);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	bool IsDragging();
	void SelectionChanged();

	void Draw(PGRendererHandle, PGIRect*);

	void Invalidate() {
		PGIRect rect = PGIRect((int)X(), (int)Y(), (int)this->width, (int)this->height);
		RefreshWindow(this->window, rect);
	}
	TextField* active_textfield;
private:
	Button buttons[4];
	PGFontHandle font;
};
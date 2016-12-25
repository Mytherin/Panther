#pragma once

#include "control.h"
#include "textfield.h"

#define STATUSBAR_HEIGHT 20

class StatusBar : public Control {
public:
	StatusBar(PGWindowHandle window, TextField* textfield);

	void UpdateParentSize(PGSize old_size, PGSize new_size);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	void SelectionChanged();

	void Draw(PGRendererHandle, PGIRect*);

	void Invalidate() {
		PGIRect rect = PGIRect((int)X(), (int)Y(), (int)this->width, (int)this->height);
		RefreshWindow(this->window, rect);
	}
private:
	PGIRect buttons[5];
	PGFontHandle font;
	TextField* active_textfield;




};
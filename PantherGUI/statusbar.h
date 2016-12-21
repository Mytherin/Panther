#pragma once

#include "control.h"
#include "textfield.h"

#define STATUSBAR_HEIGHT 20

class StatusBar : public Control {
public:
	StatusBar(PGWindowHandle window, TextField* textfield);

	void UpdateParentSize(PGSize old_size, PGSize new_size);

	void SelectionChanged();

	void Draw(PGRendererHandle, PGIRect*);
private:
	PGFontHandle font;
	TextField* active_textfield;




};
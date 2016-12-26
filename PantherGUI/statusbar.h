#pragma once

#include "button.h"
#include "control.h"
#include "container.h"
#include "textfield.h"

#define STATUSBAR_HEIGHT 20

class StatusBar : public PGContainer {
public:
	StatusBar(PGWindowHandle window, TextField* textfield);
	~StatusBar();

	void UpdateParentSize(PGSize old_size, PGSize new_size);

	void SelectionChanged();

	void Draw(PGRendererHandle, PGIRect*);

	void Invalidate() {
		PGIRect rect = PGIRect((int)X(), (int)Y(), (int)this->width, (int)this->height);
		RefreshWindow(this->window, rect);
	}
	TextField* active_textfield;
private:
	Button* unicode_button;
	Button* language_button;
	Button* lineending_button;
	Button* tabwidth_button;

	PGFontHandle font;
};
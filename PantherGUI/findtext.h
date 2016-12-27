#pragma once

#include "button.h"
#include "container.h"
#include "simpletextfield.h"
#include "togglebutton.h"

class FindText : public PGContainer {
public:
	FindText(PGWindowHandle window);
	~FindText();

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnResize(PGSize old_size, PGSize new_size);

	bool HighlightMatches() { return toggle_highlight->IsToggled(); }

	void Find(PGDirection direction);
	void FindAll(PGDirection direction);

	lng selected_match = -1;
private:
	PGScalar hoffset = 0;

	PGFontHandle font;
	SimpleTextField* field;
	Button* find_button;
	Button* find_prev;
	Button* find_all;
	Button* find_expand;
	ToggleButton* toggle_regex;
	ToggleButton* toggle_matchcase;
	ToggleButton* toggle_wholeword;
	ToggleButton* toggle_wrap;
	ToggleButton* toggle_highlight;
};
#pragma once

#include "button.h"
#include "container.h"
#include "simpletextfield.h"
#include "togglebutton.h"

class FindText : public PGContainer {
public:
	FindText(PGWindowHandle window, bool replace = false);
	~FindText();

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnResize(PGSize old_size, PGSize new_size);

	bool HighlightMatches() { return toggle_highlight->IsToggled(); }

	bool Find(PGDirection direction, bool include_selection = false);
	void FindAll(PGDirection direction);
	void Replace();
	void ReplaceAll();

	lng selected_match = -1;

	void ToggleReplace();
private:
	bool replace = false;

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

	SimpleTextField* replace_field = nullptr;
	Button* replace_button = nullptr;
	Button* replace_all_button = nullptr;
};
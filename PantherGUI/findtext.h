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

	bool HighlightMatches() { return toggle_highlight && toggle_highlight->IsToggled(); }
	
	void ShiftTextfieldFocus(PGDirection direction);

	void SelectAllMatches();
	bool Find(PGDirection direction, bool include_selection = false);
	void FindAll(bool select_first_match = true);
	void Replace();
	void ReplaceAll();

	void ToggleReplace();

	void Close();

	PG_CONTROL_KEYBINDINGS;
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

	nlohmann::json& GetFindHistory();
	void SetTextfile(TextFile* textfile);

	PGCursorPosition begin_pos;
	PGCursorPosition end_pos;
	TextFile* current_textfile;

	SimpleTextField* replace_field = nullptr;
	Button* replace_button = nullptr;
	Button* replace_all_button = nullptr;

	lng history_entry = 0;
};
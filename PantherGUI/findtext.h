#pragma once

#include "button.h"
#include "container.h"
#include "simpletextfield.h"
#include "togglebutton.h"

class FindText : public PGContainer {
public:
	FindText(PGWindowHandle window, PGFindTextType type);
	~FindText();

	bool KeyboardButton(PGButton button, PGModifier modifier);
	
	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnResize(PGSize old_size, PGSize new_size);

	bool HighlightMatches() { return type != PGFindReplaceManyFiles && toggle_highlight && toggle_highlight->IsToggled(); }
	
	void ShiftTextfieldFocus(PGDirection direction);

	void SelectAllMatches(bool in_selection = false);
	bool Find(PGDirection direction, bool include_selection = false);
	void FindAll(bool select_first_match = true);
	void FindInFiles();
	void Replace();
	void ReplaceAll(bool in_selection = false);

	void SetType(PGFindTextType type);

	void Close();

	PG_CONTROL_KEYBINDINGS;
private:
	PGFindTextType type;

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

	SimpleTextField* replace_field = nullptr;
	Button* replace_button = nullptr;
	Button* replace_all_button = nullptr;
	Button* replace_in_selection_button = nullptr;
	Button* replace_expand = nullptr;

	SimpleTextField* files_to_include_field = nullptr;
	ToggleButton* source_files_only = nullptr;
	ToggleButton* respect_gitignore = nullptr;
	


	lng history_entry = 0;
};
#pragma once

#include "button.h"
#include "container.h"
#include "label.h"
#include "simpletextfield.h"
#include "togglebutton.h"
#include "findtextmanager.h"

class PGFindText : public PGContainer {
public:
	PGFindText(PGWindowHandle window, PGFindTextType type);
	~PGFindText();

	bool KeyboardButton(PGButton button, PGModifier modifier);
	
	void Draw(PGRendererHandle renderer);

	bool HighlightMatches() { return type != PGFindReplaceManyFiles && toggle_highlight && toggle_highlight->IsToggled(); }
	
	void ShiftTextfieldFocus(PGDirection direction);

	bool SelectAllMatches(bool in_selection = false);
	bool Find(PGDirection direction, bool include_selection = false);
	void FindAll(bool select_first_match = true);
	void FindInFiles();
	void Replace();
	void ReplaceAll(bool in_selection = false);

	void SetType(PGFindTextType type);

	void ResolveSize(PGSize new_size);

	void Close();

	PG_CONTROL_KEYBINDINGS;

	virtual PGControlType GetControlType() { return PGControlTypeFindText; }

private:
	enum PGFindTextToggles {
		PGFindTextToggleRegex,
		PGFindTextToggleMatchcase,
		PGFindTextToggleWholeword,
		PGFindTextToggleWrap,
		PGFindTextToggleHighlight,
		PGFindTextToggleSourceOnly,
		PGFindTextToggleGitIgnore
	};
	void Toggle(PGFindTextToggles type);

	bool regex = false;
	bool matchcase = false;
	bool wholeword = false;
	bool wrap = true;
	bool highlight = true;
	bool source_only = false;
	bool respect_gitignore = false;

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
	Label* find_label = nullptr;
	
	SimpleTextField* replace_field = nullptr;
	Button* replace_button = nullptr;
	Button* replace_all_button = nullptr;
	Button* replace_in_selection_button = nullptr;
	Button* replace_expand = nullptr;
	Label* replace_label = nullptr;

	SimpleTextField* files_to_include_field = nullptr;
	ToggleButton* source_files_only = nullptr;
	ToggleButton* toggle_respect_gitignore = nullptr;
	Label* filter_label = nullptr;

	FindTextManager& GetFindTextManager();

	void UpdateFieldHeight(bool force_update = false);

	lng field_lines = 0;
	lng history_entry = 0;
};
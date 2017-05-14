#pragma once

#include "button.h"
#include "container.h"
#include "scrollbar.h"
#include "togglebutton.h"
#include "searchindex.h"

class SimpleTextField;

#define SEARCHBOX_MAX_ENTRIES 30

class SearchBox;

typedef void(*SearchBoxRenderFunction)(PGRendererHandle renderer, PGFontHandle font, SearchEntry& entry, PGScalar& x, PGScalar& y, PGScalar button_height);
typedef void(*SearchBoxSelectionChangedFunction)(SearchBox* searchbox, SearchEntry& entry, void* data);
typedef void(*SearchBoxCloseFunction)(SearchBox* searchbox, bool success, SearchEntry& entry, void* data);


class SearchBox : public PGContainer {
public:
	SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries, bool render_subtitles = true);
	SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries, SearchIndex* index, bool render_subtitles = true);

	void Initialize();

	bool KeyboardCharacter(char character, PGModifier modifier);
	bool KeyboardButton(PGButton button, PGModifier modifier);

	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void Draw(PGRendererHandle renderer);

	PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }

	void OnResize(PGSize old_size, PGSize new_size);

	bool ControlTakesFocus() { return true; }

	void Close(bool success = false);

	void OnRender(SearchBoxRenderFunction func) { render_function = func; }
	void OnSelectionChanged(SearchBoxSelectionChangedFunction func, void* data) { selection_changed = func; selection_changed_data = data; }
	void OnClose(SearchBoxCloseFunction func, void* data) { close_function = func; close_data = data;}

	PG_CONTROL_KEYBINDINGS;

	virtual PGControlType GetControlType() { return PGControlTypeSearchBox; }
private:
	SearchIndex* index = nullptr;
	std::vector<SearchEntry> entries;
	std::vector<SearchEntry*> displayed_entries;

	bool render_subtitles = false;

	PGFontHandle font;
	SimpleTextField* field;
	Scrollbar* scrollbar;

	PGScalar entry_height = 0;
	lng scroll_position = 0;
	lng selected_entry;
	std::string filter;
	SearchBoxRenderFunction render_function = nullptr;
	SearchBoxSelectionChangedFunction selection_changed = nullptr;
	void* selection_changed_data = nullptr;

	SearchBoxCloseFunction close_function = nullptr;
	void* close_data = nullptr;

	PGScalar GetEntryHeight() { return entry_height; }
	lng GetRenderedEntries();
	void SetScrollPosition(lng entry);
	void SetSelectedEntry(lng entry);

	void Filter(std::string filter);
};
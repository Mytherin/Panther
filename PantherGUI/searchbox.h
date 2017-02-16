#pragma once

#include "button.h"
#include "container.h"
#include "simpletextfield.h"
#include "togglebutton.h"

struct SearchEntry {
	std::string display_name;
	std::string display_subtitle;
	std::string text;
	std::shared_ptr<TextFile> data;
	double multiplier;
	double basescore;
};

struct SearchRank {
	lng index;
	double score;
	lng pos;
	lng subpos;
	lng text_pos;

	SearchRank(lng index, double score) : index(index), score(score), pos(-1), text_pos(-1) {}

	friend bool operator<(const SearchRank& l, const SearchRank& r) {
		return l.score < r.score;
	}
	friend bool operator> (const SearchRank& lhs, const SearchRank& rhs){ return rhs < lhs; }
	friend bool operator<=(const SearchRank& lhs, const SearchRank& rhs){ return !(lhs > rhs); }
	friend bool operator>=(const SearchRank& lhs, const SearchRank& rhs){ return !(lhs < rhs); }
	friend bool operator==(const SearchRank& lhs, const SearchRank& rhs) { return lhs.score == rhs.score; }
	friend bool operator!=(const SearchRank& lhs, const SearchRank& rhs){ return !(lhs == rhs); }
};

#define SEARCHBOX_MAX_ENTRIES 30

class SearchBox;

typedef void(*SearchBoxRenderFunction)(PGRendererHandle renderer, PGFontHandle font, SearchRank& rank, SearchEntry& entry, PGScalar& x, PGScalar& y, PGScalar button_height);
typedef void(*SearchBoxSelectionChangedFunction)(SearchBox* searchbox, SearchRank& rank, SearchEntry& entry, void* data);
typedef void(*SearchBoxSelectionConfirmedFunction)(SearchBox* searchbox, SearchRank& rank, SearchEntry& entry, void* data);
typedef void(*SearchBoxSelectionCancelledFunction)(SearchBox* searchbox, void* data);

class SearchBox : public PGContainer {
public:
	SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries);
	~SearchBox();

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnResize(PGSize old_size, PGSize new_size);

	void Close(bool success = false);

	void OnRender(SearchBoxRenderFunction func) { render_function = func; }
	void OnSelectionChanged(SearchBoxSelectionChangedFunction func, void* data) { selection_changed = func; selection_changed_data = data; }
	void OnSelectionConfirmed(SearchBoxSelectionConfirmedFunction func, void* data) { selection_confirmed = func; selection_confirmed_data = data; }
	void OnSelectionCancelled(SearchBoxSelectionCancelledFunction func, void* data) { selection_cancelled = func; selection_cancelled_data = data; }
private:
	std::vector<SearchEntry> entries;
	std::vector<SearchRank> displayed_entries;

	PGFontHandle font;
	SimpleTextField* field;

	lng selected_entry;
	lng filter_size;
	SearchBoxRenderFunction render_function = nullptr;
	SearchBoxSelectionChangedFunction selection_changed = nullptr;
	void* selection_changed_data = nullptr;
	SearchBoxSelectionConfirmedFunction selection_confirmed = nullptr;
	void* selection_confirmed_data = nullptr;
	SearchBoxSelectionCancelledFunction selection_cancelled = nullptr;
	void* selection_cancelled_data = nullptr;

	void Filter(std::string filter);
};
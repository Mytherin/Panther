#pragma once

#include "button.h"
#include "container.h"
#include "simpletextfield.h"
#include "togglebutton.h"

struct SearchEntry {
	std::string display_name;
	std::string text;
	void* data;
};

struct SearchRank {
	lng index;
	double score;
	lng pos;
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

class SearchBox : public PGContainer {
public:
	SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries);
	~SearchBox();

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnResize(PGSize old_size, PGSize new_size);

	void Close();
private:
	std::vector<SearchEntry> entries;
	std::vector<SearchRank> displayed_entries;

	PGFontHandle font;
	SimpleTextField* field;

	lng selected_entry;
	lng filter_size;

	void Filter(std::string filter);
};
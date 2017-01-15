
#include "searchbox.h"
#include "style.h"
#include "unicode.h"


SearchBox::SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries) :
	PGContainer(window), selected_entry(-1), entries(entries), filter_size(0) {
	lng index = 0;
	for (auto it = this->entries.begin(); it != this->entries.end(); it++) {
		if (displayed_entries.size() < SEARCHBOX_MAX_ENTRIES)
			// FIXME: score based on previous selections or some other criteria
			displayed_entries.push_back(SearchRank(index, 0));
		index++;
	}
	font = PGCreateFont("myriad", false, true);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));

	field = new SimpleTextField(window);
	field->width = this->width;
	field->x = 0;
	field->y = 0;
	field->OnUserCancel([](Control* c, void* data, PGModifier modifier) {
		((SearchBox*)data)->Close();
	}, (void*) this);
	field->OnSuccessfulExit([](Control* c, void* data, PGModifier modifier) {
		((SearchBox*)data)->Close();
	}, (void*) this);
	field->OnTextChanged([](Control* c, void* data) {
		((SearchBox*)data)->Filter(((SimpleTextField*)c)->GetText());

	}, (void*) this);
	this->AddControl(field);
}

SearchBox::~SearchBox() {

}

bool SearchBox::KeyboardButton(PGButton button, PGModifier modifier) {
	return PGContainer::KeyboardButton(button, modifier);
}

void RenderTextPartialBold(PGRendererHandle renderer, PGFontHandle font, std::string text, lng start_bold, lng bold_size, PGScalar x, PGScalar y) {

	PGScalar current_position = 0;
	lng current_character = 0;
	assert(start_bold < (lng) text.size());
	if (start_bold > 0) {
		SetTextStyle(font, PGTextStyleNormal);
		RenderText(renderer, font, text.c_str(), start_bold, x + current_position, y);
		current_position += MeasureTextWidth(font, text.c_str(), start_bold);
		current_character += start_bold;
	}
	if (start_bold >= 0) {
		SetTextStyle(font, PGTextStyleBold);
		RenderText(renderer, font, text.c_str() + current_character, bold_size, x + current_position, y);
		current_position += MeasureTextWidth(font, text.c_str() + current_character, bold_size);
		current_character += bold_size;
	}
	SetTextStyle(font, PGTextStyleNormal);
	if (current_character < text.size()) {
		RenderText(renderer, font, text.c_str() + current_character, text.size() - current_character, x + current_position, y);
	}

}

void SearchBox::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGScalar x = X() - rect->x, y = Y() - rect->y;
	PGScalar current_pos = field->height;
	lng initial_selection = 0;
	lng current_selection = initial_selection;


	PGScalar BUTTON_HEIGHT = 0;
	{
		BUTTON_HEIGHT += 6;
		BUTTON_HEIGHT += 8;
		SetTextFontSize(font, 13);
		BUTTON_HEIGHT += GetTextHeight(font) + 1;
		SetTextFontSize(font, 11);
		BUTTON_HEIGHT += GetTextHeight(font) + 1;
	}
	while (current_pos + BUTTON_HEIGHT < this->height) {
		if (current_selection >= displayed_entries.size()) break;

		// render the background
		PGColor background_color = PGColor(34, 34, 34);
		if (selected_entry == current_selection) {
			background_color = PGColor(64, 64, 64);
		}

		if (current_selection != initial_selection) {
			current_pos += 4;
		}
		RenderRectangle(renderer, PGRect(x, y + current_pos, this->width, BUTTON_HEIGHT), background_color, PGDrawStyleFill);

		if (current_selection != initial_selection) {
			RenderLine(renderer, PGLine(
				PGPoint(x, y + current_pos + 4), 
				PGPoint(x + this->width, y + current_pos + 4)),
				PGColor(30, 30, 30), 2);
			current_pos += 4;
		}

		current_pos += 3;
		SearchRank& rank = displayed_entries[current_selection];
		SearchEntry& entry = entries[rank.index];
		// render the text
		SetTextFontSize(font, 13);
		RenderTextPartialBold(renderer, font, entry.display_name, rank.pos, filter_size, x + 5, y + current_pos + 1);
		current_pos += GetTextHeight(font) + 1;
		SetTextFontSize(font, 11);
		RenderTextPartialBold(renderer, font, entry.text, rank.text_pos, filter_size, x + 5, y + current_pos + 2);
		current_pos += GetTextHeight(font) + 1;
		current_pos += 3;

		current_selection++;
	}
	PGContainer::Draw(renderer, rect);
}

void SearchBox::OnResize(PGSize old_size, PGSize new_size) {
	field->width = new_size.width;
}

void SearchBox::Close() {
	dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);
}

void SearchBox::Filter(std::string filter) {
	filter = utf8_tolower(filter);
	displayed_entries.clear();
	lng index = 0;
	filter_size = filter.size();

	for (auto it = entries.begin(); it != entries.end(); it++) {
		size_t pos = utf8_tolower(it->display_name).find(filter);
		size_t textpos = utf8_tolower(it->text).find(filter);
		if (pos != std::string::npos || textpos != std::string::npos) {
			size_t score_pos = std::min(textpos, pos);
			double score = score_pos == 0 ? 1 : 1.0 / score_pos;
			//displayed_entries.push_back(SearchRank(index, score));
			if (displayed_entries.size() >= SEARCHBOX_MAX_ENTRIES) {
				// have to remove an entry
				if (displayed_entries.back().score < score) {
					displayed_entries.pop_back();
				} else {
					continue;
				}
			}
			SearchRank rank = SearchRank(index, score);
			rank.pos = pos;
			rank.text_pos = textpos;
			// insertion sort
			displayed_entries.insert(
				std::upper_bound(displayed_entries.begin(), displayed_entries.end(), rank),
				rank
				);
		}
		index++;
	}
	std::reverse(displayed_entries.begin(), displayed_entries.end());
	this->Invalidate();
}
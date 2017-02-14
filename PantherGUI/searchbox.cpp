
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
	field->OnCancel([](Control* c, void* data) {
		((SearchBox*)data)->Close();
	}, (void*) this);
	field->OnConfirm([](Control* c, void* data) {
		((SearchBox*)data)->Close(true);
	}, (void*) this);
	field->OnTextChanged([](Control* c, void* data) {
		((SearchBox*)data)->Filter(((SimpleTextField*)c)->GetText());
	}, (void*) this);
	this->AddControl(field);
}

SearchBox::~SearchBox() {

}

bool SearchBox::KeyboardButton(PGButton button, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		switch (button) {
		case PGButtonUp:
			if (displayed_entries.size() > 0) {
				selected_entry--;
				if (selected_entry < 0) {
					selected_entry = displayed_entries.size() - 1;
				}
				SearchRank& rank = displayed_entries[selected_entry];
				SearchEntry& entry = entries[rank.index];
				selection_changed(this, rank, entry, selection_changed_data);
				this->Invalidate();
			}
			return true;
		case PGButtonDown:
			if (displayed_entries.size() > 0) {
				selected_entry++;
				if (selected_entry >= displayed_entries.size()) {
					selected_entry = 0;
				}
				SearchRank& rank = displayed_entries[selected_entry];
				SearchEntry& entry = entries[rank.index];
				selection_changed(this, rank, entry, selection_changed_data);
				this->Invalidate();
			}
			return true;
		default:
			break;
		}
	}
	return PGContainer::KeyboardButton(button, modifier);
}

void RenderTextPartialBold(PGRendererHandle renderer, PGFontHandle font, std::string text, lng start_bold, lng bold_size, PGScalar x, PGScalar y) {
	PGScalar current_position = 0;
	lng current_character = 0;
	assert(bold_size == 0 || (start_bold == 0 || start_bold < (lng) text.size()));
	if (start_bold > 0 && text.size() > 0) {
		SetTextStyle(font, PGTextStyleNormal);
		lng length = std::min(start_bold, (lng) text.size() - 1);
		RenderText(renderer, font, text.c_str(), length, x + current_position, y);
		current_position += MeasureTextWidth(font, text.c_str(), length);
		current_character += length;
	}
	if (bold_size != 0) {
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

}

void SearchBox::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGScalar x = X() - rect->x, y = Y() - rect->y;
	PGScalar current_x = x;
	PGScalar current_y = y + field->height;
	lng initial_selection = 0; // FIXME: set to scroll size
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

	while ((current_y - y) + BUTTON_HEIGHT < this->height) {
		if (current_selection >= displayed_entries.size()) break;

		// render the background
		PGColor background_color = PGColor(34, 34, 34);
		if (selected_entry == current_selection) {
			background_color = PGColor(64, 64, 64);
		}

		if (current_selection != initial_selection) {
			current_y += 4;
		}
		RenderRectangle(renderer, PGRect(current_x, current_y, this->width, BUTTON_HEIGHT), background_color, PGDrawStyleFill);

		if (current_selection != initial_selection) {
			RenderLine(renderer, PGLine(
				PGPoint(current_x, current_y), 
				PGPoint(current_x + this->width, current_y)),
				PGColor(30, 30, 30), 2);
		}

		current_y += 3;
		SearchRank& rank = displayed_entries[current_selection];
		SearchEntry& entry = entries[rank.index];
		if (render_function) {
			render_function(renderer, font, rank, entry, current_x, current_y, BUTTON_HEIGHT - 6);
		}
		// render the text
		SetTextFontSize(font, 13);
		RenderTextPartialBold(renderer, font, entry.display_name, rank.pos, filter_size, current_x + 5, current_y + 1);
		current_y += GetTextHeight(font) + 1;
		SetTextFontSize(font, 11);
		RenderTextPartialBold(renderer, font, entry.display_subtitle, rank.subpos, filter_size, current_x + 5, current_y + 2);
		current_y += GetTextHeight(font) + 1;
		current_y += 3;

		current_x = x;
		current_selection++;
	}
	PGContainer::Draw(renderer, rect);
}

void SearchBox::OnResize(PGSize old_size, PGSize new_size) {
	field->width = new_size.width;
}

void SearchBox::Close(bool success) {
	if (success && displayed_entries.size() > 0) {
		if (selection_confirmed) {
			SearchRank& rank = displayed_entries[selected_entry];
			SearchEntry& entry = entries[rank.index];
			selection_confirmed(this, rank, entry, selection_confirmed_data);
		}
	} else {
		if (selection_cancelled) {
			selection_cancelled(this, selection_cancelled_data);
		}
	}

	dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);
}

void SearchBox::Filter(std::string filter) {
	filter = utf8_tolower(filter);
	displayed_entries.clear();
	filter_size = filter.size();

	for (lng index = 0; index < entries.size(); index++) {
		SearchEntry& entry = entries[index];
		size_t pos = utf8_tolower(entry.display_name).find(filter);
		size_t subpos = utf8_tolower(entry.display_subtitle).find(filter);
		size_t textpos = utf8_tolower(entry.text).find(filter);
		if (pos != std::string::npos || subpos != std::string::npos || textpos != std::string::npos) {
			size_t score_pos = std::min(textpos, pos);
			double score = ((score_pos == 0 ? 1.1 : 1.0 / score_pos) * entry.multiplier) + entry.basescore;
			//displayed_entries.push_back(SearchRank(index, score));
			if (displayed_entries.size() >= SEARCHBOX_MAX_ENTRIES) {
				// have to remove an entry
				if (displayed_entries.front().score < score) {
					displayed_entries.erase(displayed_entries.begin());
				} else {
					continue;
				}
			}
			SearchRank rank = SearchRank(index, score);
			rank.pos = pos;
			rank.subpos = subpos;
			rank.text_pos = textpos;
			// insertion sort
			displayed_entries.insert(
				std::upper_bound(displayed_entries.begin(), displayed_entries.end(), rank),
				rank
				);
		}
	}
	std::reverse(displayed_entries.begin(), displayed_entries.end());
	selected_entry = 0;
	if (displayed_entries.size() > 0) {
		SearchRank& rank = displayed_entries[selected_entry];
		SearchEntry& entry = entries[rank.index];
		selection_changed(this, rank, entry, selection_changed_data);
	}
	this->Invalidate();
}
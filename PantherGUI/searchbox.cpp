
#include "searchbox.h"
#include "style.h"
#include "unicode.h"
#include "goto.h"

SearchBox::SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries, bool render_subtitles) :
	PGContainer(window), selected_entry(-1), entries(entries), filter_size(0),
	scrollbar(nullptr), scroll_position(0), render_subtitles(render_subtitles) {
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
	this->Filter("");

	scrollbar = new Scrollbar(this, window, false, false);
	scrollbar->bottom_padding = 0;
	scrollbar->top_padding = SCROLLBAR_SIZE;
	scrollbar->SetPosition(PGPoint(this->width - SCROLLBAR_SIZE, field->height));
	scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((SearchBox*)scroll->parent)->scroll_position = value;
	});
	this->AddControl(scrollbar);
}

void SearchBox::SetSelectedEntry(lng entry) {
	selected_entry = entry;
	if (selected_entry < 0) {
		selected_entry = displayed_entries.size() - 1;
	}
	if (selected_entry >= displayed_entries.size()) {
		selected_entry = 0;
	}
	if (selected_entry - (GetRenderedEntries() - 1) > scroll_position) {
		SetScrollPosition(selected_entry - (GetRenderedEntries() - 1));
	}
	if (selected_entry < scroll_position) {
		SetScrollPosition(selected_entry);
	}
	if (displayed_entries.size() > 0 && selection_changed) {
		SearchRank& rank = displayed_entries[selected_entry];
		SearchEntry& search_entry = entries[rank.index];
		selection_changed(this, rank, search_entry, selection_changed_data);
	}
}

lng SearchBox::GetRenderedEntries() {
	return (lng) std::floor((this->height - field->height) / entry_height);
}

void SearchBox::SetScrollPosition(lng position) {
	scroll_position = std::max(std::min((lng) displayed_entries.size() - GetRenderedEntries(), position), (lng) 0);
}

bool SearchBox::KeyboardButton(PGButton button, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		switch (button) {
		case PGButtonUp:
			if (displayed_entries.size() > 0) {
				SetSelectedEntry(selected_entry - 1);
				this->Invalidate();
			}
			return true;
		case PGButtonDown:
			if (displayed_entries.size() > 0) {
				SetSelectedEntry(selected_entry + 1);
				this->Invalidate();
			}
			return true;
		default:
			break;
		}
	}
	return PGContainer::KeyboardButton(button, modifier);
}

void SearchBox::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	if (button & PGLeftMouseButton && y > field->height) {
		if (!scrollbar->visible || mouse.x < this->width - scrollbar->width) {
			lng selected_entry = (mouse.y - field->height) / entry_height;
			if (selected_entry >= 0 && selected_entry < GetRenderedEntries()) {
				SetSelectedEntry(scroll_position + selected_entry);
			}
			return;
		}
	}
	PGContainer::MouseDown(x, y, button, modifier, click_count);
}

void SearchBox::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (button & PGLeftMouseButton && y > field->height) {
		if (!scrollbar->visible || mouse.x < this->width - scrollbar->width) {
			lng selected_entry = (mouse.y - field->height) / entry_height;
			if (selected_entry >= 0 && selected_entry < GetRenderedEntries()) {
				if (scroll_position + selected_entry == this->selected_entry) {
					this->Close(true);
				}
			}
			return;
		}
	}
	PGContainer::MouseUp(x, y, button, modifier);
}

void SearchBox::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		if (distance != 0) {
			lng offset = (distance < 0 ? -1 : 1) * std::ceil(std::abs(distance / 30.0f));
			SetScrollPosition(scroll_position - offset);
			this->Invalidate();
		}
	}
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
	if (bold_size > 0 && start_bold >= 0) {
		SetTextStyle(font, PGTextStyleBold);
		RenderText(renderer, font, text.c_str() + current_character, bold_size, x + current_position, y);
		current_position += MeasureTextWidth(font, text.c_str() + current_character, bold_size);
		current_character += bold_size;
	}
	if (current_character < text.size()) {
		SetTextStyle(font, PGTextStyleNormal);
		RenderText(renderer, font, text.c_str() + current_character, text.size() - current_character, x + current_position, y);
	}

}

void SearchBox::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGScalar x = X() - rect->x, y = Y() - rect->y;
	PGScalar current_x = x;
	PGScalar initial_y = y + field->height;
	PGScalar current_y = initial_y;
	lng initial_selection = scroll_position;
	lng current_selection = initial_selection;
	entry_height = 0;
	{
		entry_height += 10;
		SetTextFontSize(font, 13);
		entry_height += GetTextHeight(font) + 1;
		if (render_subtitles) {
			SetTextFontSize(font, 11);
			entry_height += GetTextHeight(font) + 1;
		}
	}

	while ((current_y - y) + entry_height < this->height) {
		if (current_selection >= displayed_entries.size()) break;

		// render the background
		PGColor background_color = PGColor(34, 34, 34);
		if (selected_entry == current_selection) {
			background_color = PGColor(64, 64, 64);
		}

		if (current_selection != initial_selection) {
			current_y += 4;
		}
		RenderRectangle(renderer, PGRect(current_x, current_y, this->width, entry_height), background_color, PGDrawStyleFill);

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
			render_function(renderer, font, rank, entry, current_x, current_y, entry_height - 6);
		}
		// render the text
		SetTextFontSize(font, 13);
		RenderTextPartialBold(renderer, font, entry.display_name, rank.pos, filter_size, current_x + 5, current_y + 1);
		current_y += GetTextHeight(font) + 1;
		if (render_subtitles) {
			SetTextFontSize(font, 11);
			RenderTextPartialBold(renderer, font, entry.display_subtitle, rank.subpos, filter_size, current_x + 5, current_y + 2);
			current_y += GetTextHeight(font) + 1;
		}
		current_y += 3;

		current_x = x;
		current_selection++;
	}
	if (GetRenderedEntries() >= displayed_entries.size()) {
		scrollbar->visible = false;
	} else {
		scrollbar->visible = true;
		scrollbar->SetPosition(PGPoint(this->width - scrollbar->width, field->height + SCROLLBAR_PADDING));
		scrollbar->SetSize(PGSize(SCROLLBAR_SIZE, current_y - initial_y - 2 * SCROLLBAR_PADDING - 7));
		scrollbar->UpdateValues(0, displayed_entries.size() - GetRenderedEntries(), GetRenderedEntries(), scroll_position);
	}
	PGContainer::Draw(renderer, rect);
}

void SearchBox::OnResize(PGSize old_size, PGSize new_size) {
	field->width = new_size.width;
	PGContainer::OnResize(old_size, new_size);
}

void SearchBox::Close(bool success) {
	PGGotoAnything* goto_anything = dynamic_cast<PGGotoAnything*>(this->parent);
	if (goto_anything != nullptr) {
		goto_anything->Close(success);
	} else {
		dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);
	}
	/*
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
	dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);*/
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
	SetSelectedEntry(0);
	this->Invalidate();
}

#include "searchbox.h"
#include "simpletextfield.h"
#include "style.h"
#include "unicode.h"
#include "goto.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(SearchBox);

SearchBox::SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries, bool render_subtitles) :
	PGContainer(window), selected_entry(-1), entries(entries),
	scrollbar(nullptr), scroll_position(0), render_subtitles(render_subtitles),
	close_function(nullptr), close_data(nullptr) {
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
	scrollbar->padding.bottom = SCROLLBAR_PADDING;
	scrollbar->padding.top = SCROLLBAR_PADDING;
	scrollbar->SetAnchor(PGAnchorRight | PGAnchorTop);
	scrollbar->margin.top = field->height + 2;
	scrollbar->margin.bottom = 2;
	scrollbar->fixed_width = SCROLLBAR_SIZE;
	scrollbar->percentage_height = 1;	
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

bool SearchBox::KeyboardCharacter(char character, PGModifier modifier) {
	if (this->PressCharacter(SearchBox::keybindings, character, modifier)) {
		return true;
	}
	return PGContainer::KeyboardCharacter(character, modifier);
}

bool SearchBox::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(SearchBox::keybindings, button, modifier)) {
		return true;
	}
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
	if (button & PGLeftMouseButton && mouse.y > field->height) {
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
	if (button & PGLeftMouseButton && mouse.y > field->y + field->height) {
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

void RenderEntry(PGRendererHandle renderer, PGFontHandle font, std::string text, std::string filter, PGScalar x, PGScalar y) {
	std::vector<size_t> bold_positions;
	std::string search_text = panther::tolower(text);

	size_t pos = search_text.find(filter);
	if (pos != std::string::npos) {
		for (size_t i = 0; i < filter.size(); i++) {
			bold_positions.push_back(pos + i);
		}
	} else {
		size_t prev_offset = 0;
		for (size_t i = 0; i < filter.size(); i++) {
			size_t offset = search_text.find(filter[i], prev_offset);
			if (offset == std::string::npos) {
				if (prev_offset == 0) {
					continue;
				} else {
					offset = search_text.find(filter[i], 0);
					if (offset == std::string::npos) {
						continue;
					}
				}
			}
			bold_positions.push_back(offset);
			prev_offset = offset;
		}
		std::sort(bold_positions.begin(), bold_positions.end());
	}
	PGScalar current_position = 0;
	lng current_character = 0;
	for (size_t i = 0; i < bold_positions.size(); i++) {
		size_t bold = bold_positions[i];
		if (bold < current_character) continue;
		if (bold > current_character) {
			SetTextStyle(font, PGTextStyleNormal);
			RenderText(renderer, font, text.c_str() + current_character, bold - current_character, x + current_position, y);
			current_position += MeasureTextWidth(font, text.c_str() + current_character, bold - current_character);
		}
		current_character = bold;
		while (i + 1 < bold_positions.size()) {
			if (bold_positions[i + 1] == bold + 1) {
				i++;
				bold = bold_positions[i];
			} else {
				break;
			}
		}
		bold++;
		SetTextStyle(font, PGTextStyleBold);
		RenderText(renderer, font, text.c_str() + current_character, bold - current_character, x + current_position, y);
		current_position += MeasureTextWidth(font, text.c_str() + current_character, bold - current_character);
		current_character = bold;
	}
	if (text.size() != current_character) {
		SetTextStyle(font, PGTextStyleNormal);
		RenderText(renderer, font, text.c_str() + current_character, text.size() - current_character, x + current_position, y);
	}
}

void SearchBox::Draw(PGRendererHandle renderer) {
	PGScalar x = X(), y = Y();
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
		RenderEntry(renderer, font, entry.display_name, filter, current_x + 5, current_y + 1);
		current_y += GetTextHeight(font) + 1;
		if (render_subtitles) {
			SetTextFontSize(font, 11);
			RenderEntry(renderer, font, entry.display_subtitle, filter, current_x + 5, current_y + 2);
			//RenderTextPartialBold(renderer, font, entry.display_subtitle, rank.subpos, filter_size, current_x + 5, current_y + 2);
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
		scrollbar->UpdateValues(0, displayed_entries.size() - GetRenderedEntries(), GetRenderedEntries(), scroll_position);
	}
	PGContainer::Draw(renderer);
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
		if (close_function) {
			SearchRank& rank = displayed_entries[selected_entry];
			SearchEntry& entry = entries[rank.index];
			close_function(this, success, rank, entry, close_data);
		}
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

size_t StringDistance(std::string s, std::string t) {
	size_t similarity = t.size() + s.size();
	size_t pos = utf8_tolower(t).find(s);
	if (pos != std::string::npos) {
		return pos;
	}
	similarity = t.size();
	size_t prev_offset = 0;
	size_t max_score = t.size() + s.size();
	size_t found_characters = 0;
	for (size_t i = 0; i < s.size(); i++) {
		size_t offset = t.find(s[i]);
		if (offset == std::string::npos) {
			similarity += s.size() / 2;
			continue;
		}
		found_characters++;
		t.erase(t.begin() + offset);
		if (offset != prev_offset) {
			similarity += panther::abs((lng)offset - (lng)prev_offset) / 2;
		}
		prev_offset = offset;
	}
	if (similarity >= max_score || found_characters == 0 || found_characters < s.size() / 2) {
		return std::string::npos;
	}
	return similarity;
}

void SearchBox::Filter(std::string filter) {
	filter = utf8_tolower(filter);
	displayed_entries.clear();
	this->filter = filter;

	for (lng index = 0; index < entries.size(); index++) {
		SearchEntry& entry = entries[index];
		size_t cost1 = StringDistance(filter, utf8_tolower(entry.display_name));
		size_t cost2 = StringDistance(filter, utf8_tolower(entry.display_subtitle));
		size_t cost3 = StringDistance(filter, utf8_tolower(entry.text));
		if (cost1 != std::string::npos || cost2 != std::string::npos || cost3 != std::string::npos) {
			cost2 = cost2 == std::string::npos ? cost2 : cost2 + entry.display_subtitle.size();
			cost3 = cost3 == std::string::npos ? cost3 : cost3 + 2 * entry.text.size();
			size_t score_pos = std::min(std::min(cost1, cost2), cost3);
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

void SearchBox::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = SearchBox::keybindings_noargs;
	noargs["confirm"] = [](Control* c) {
		dynamic_cast<SearchBox*>(c)->Close(true);
	};
	noargs["cancel"] = [](Control* c) {
		dynamic_cast<SearchBox*>(c)->Close(false);
	};
}

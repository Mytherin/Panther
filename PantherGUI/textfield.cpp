
#include "textfield.h"
#include <sstream>
#include <algorithm>
#include "logger.h"
#include "text.h"
#include "controlmanager.h"

void TextField::MinimapMouseEvent(bool mouse_enter) {
	this->InvalidateMinimap();
}

void TextField::ScrollbarMouseEvent(bool mouse_enter) {

}

void MMMouseEvent(TextField* textfield, bool mouse_enter) {
	return textfield->MinimapMouseEvent(mouse_enter);
}

TextField::TextField(PGWindowHandle window, TextFile* file) :
	Control(window, false), display_carets(true), display_carets_count(0), display_scrollbar(true), display_minimap(true), display_linenumbers(true) {
	assert(file);
	textfile = file;
	textfile->SetTextField(this);

	drag_type = PGDragNone;
	line_height = 19;
	character_width = 8;
	tabwidth = 5;

	ControlManager* manager = (ControlManager*)GetControlManager(window);
	manager->RegisterMouseRegion(&minimap_region, this, (PGMouseCallback)MMMouseEvent);

}

TextField::~TextField() {

}

#define FLICKER_CARET_INTERVAL 15

void TextField::RefreshCursors() {
	// FIXME: thread safety on setting display_carets_count?
	display_carets_count = 0;
	display_carets = true;
}

void TextField::PeriodicRender(void) {
	// FIXME: thread safety on incrementing display_carets_count and cursors?
	if (!WindowHasFocus(window)) {
		display_carets = false;
		display_carets_count = 0;
		if (!current_focus) {
			current_focus = true;
			Invalidate();
		}
		return;
	} else if (current_focus) {
		if (current_focus) {
			current_focus = false;
			Invalidate();
		}
	}

	display_carets_count++;
	if (display_carets_count % FLICKER_CARET_INTERVAL == 0) {
		display_carets_count = 0;
		display_carets = !display_carets;
		lng start_line = LLONG_MAX, end_line = -LLONG_MAX;
		std::vector<Cursor*> cursors = textfile->GetCursors();
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			start_line = std::min((*it)->start_line, start_line);
			end_line = std::max((*it)->start_line, end_line);
		}
		if (start_line <= end_line) {
			this->InvalidateBetweenLines(start_line, end_line);
		}
	}

	if (!textfile->IsLoaded()) {
		Invalidate();
	}
}

void TextField::DrawTextField(PGRendererHandle renderer, PGIRect* rectangle, bool minimap, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay) {
	// FIXME: respect Width while rendering
	lng start_line = textfile->GetLineOffset();
	std::vector<Cursor*> cursors = textfile->GetCursors();
	lng linenr = start_line;
	TextLine *current_line;
	PGColor selection_color = PGColor(20, 60, 255, 125);
	PGScalar line_height = GetTextHeight(renderer);
	PGScalar start_position_y = position_y;
	if (minimap) {
		// fill in the background of the minimap
		PGRect rect(position_x_text, position_y, this->width - position_x_text, this->height - position_y);
		RenderRectangle(renderer, rect, PGColor(30, 30, 30), PGStyleFill);
		// start line of the minimap
		start_line = GetMinimapStartLine();;
		linenr = start_line;
		start_position_y = position_y + GetMinimapOffset();
	}
	lng word_start = -1, word_end = -1;
	char* selected_word = nullptr;
	if (!minimap) {
		if (cursors[0]->GetSelectedWord(word_start, word_end) >= 0) {
			// successfully found a word
			selected_word = (char*)calloc(1, word_end - word_start + 1);
			memcpy(selected_word, textfile->GetLine(cursors[0]->BeginLine())->GetLine() + word_start, word_end - word_start);
		}
	}
	lng block = -1;
	bool parsed = false;
	textfile->Lock();
	while ((current_line = textfile->GetLine(linenr)) != nullptr) {
		// only render lines that fall within the render rectangle
		if (position_y > rectangle->height) break;
		if (!(position_y + line_height < 0)) {
			// render the actual text
			lng new_block = textfile->GetBlock(linenr);
			if (new_block != block) {
				block = new_block;
				parsed = textfile->BlockIsParsed(block);
			}
			char* line = current_line->GetLine();
			lng length = current_line->GetLength();
			lng position = 0;
			PGScalar xpos = position_x_text;
			if (parsed) {
				PGSyntax* syntax = &current_line->syntax;
				while (syntax && syntax->end > 0) {
					bool squiggles = false;
					assert(syntax->end > position);
					switch (syntax->type) {
					case -1:
						squiggles = true;
					case 0:
					case 255:
					case 4:
						SetTextColor(renderer, PGColor(191, 191, 191));
						break;
					case 1:
						SetTextColor(renderer, PGColor(255, 255, 0));
						break;
					case 2:
						SetTextColor(renderer, PGColor(166, 226, 46));
						break;
					case 3:
						SetTextColor(renderer, PGColor(230, 219, 116));
						break;
					case 5:
						SetTextColor(renderer, PGColor(128, 128, 128));
						break;
					}
					RenderText(renderer, line + position, syntax->end - position, xpos, position_y);
					PGScalar text_width = MeasureTextWidth(renderer, line + position, syntax->end - position);
					if (!minimap) {
						if (squiggles) {
							RenderSquiggles(renderer, text_width, xpos, position_y + line_height * 1.2f, PGColor(255, 0, 0));
						}
					}
					xpos += text_width;
					position = syntax->end;
					syntax = syntax->next;
				}
			}
			SetTextColor(renderer, PGColor(191, 191, 191));
			RenderText(renderer, line + position, length - position, xpos, position_y);
			if (selected_word) {
				for (lng i = 0; i <= length - (word_end - word_start); i++) {
					if ((i == 0 || GetCharacterClass(line[i - 1]) != PGCharacterTypeText) &&
						GetCharacterClass(line[i]) == PGCharacterTypeText) {
						bool found = true;
						for (lng j = 0; j < (word_end - word_start); j++) {
							if (line[i + j] != selected_word[j]) {
								found = false;
								break;
							}
						}
						if (found) {
							if ((i + (word_end - word_start) == length ||
								GetCharacterClass(line[i + (word_end - word_start)]) != PGCharacterTypeText)) {
								PGScalar char_width = MeasureTextWidth(renderer, "x", 1);
								PGScalar width = MeasureTextWidth(renderer, selected_word, word_end - word_start);
								PGRect rect(position_x_text + i * char_width, position_y, width, line_height);
								RenderRectangle(renderer, rect, PGColor(191, 191, 191), PGStyleStroke);
							}
						}
					}
				}
			}
		}
		linenr++;
		position_y += line_height;
	}
	textfile->Unlock();
	// render the selection and carets
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		lng startline = std::max((*it)->BeginLine(), start_line);
		lng endline = std::min((*it)->EndLine(), linenr);
		position_y = this->y + (startline - start_line) * line_height - rectangle->y;
		for (; startline <= endline; startline++) {
			current_line = textfile->GetLine(startline);
			assert(current_line);
			lng start, end;
			if (startline == (*it)->BeginLine()) {
				if (startline == (*it)->EndLine()) {
					// start and end are on the same line
					start = (*it)->BeginCharacter();
					end = (*it)->EndCharacter();
				} else {
					start = (*it)->BeginCharacter();
					end = current_line->GetLength() + 1;
				}
			} else if (startline == (*it)->EndLine()) {
				start = 0;
				end = (*it)->EndCharacter();
			} else {
				start = 0;
				end = current_line->GetLength() + 1;
			}

			if (!minimap && startline == (*it)->SelectedLine()) {
				if (display_carets) {
					// render the caret on the selected line
					RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x_text, position_y, (*it)->SelectedCharacter(), line_height);
				}
			}
			RenderSelection(renderer,
				current_line->GetLine(),
				current_line->GetLength(),
				position_x_text,
				position_y,
				start,
				end,
				selection_color,
				line_height);
			position_y += line_height;
		}
	}
	if (!minimap) {
		this->line_height = line_height;
	} else {
		this->minimap_line_height = line_height;
		if (render_overlay) {
			// render the overlay for the minimap
			PGRect rect(position_x_text, start_position_y, this->width - position_x_text, line_height * GetLineHeight());
			RenderRectangle(renderer, rect, PGColor(191, 191, 191, 50), PGStyleFill);
		}
	}
	if (selected_word) free(selected_word);
}

void TextField::Draw(PGRendererHandle renderer, PGIRect* rectangle) {
	// FIXME: mouse = caret over textfield
	bool window_has_focus = WindowHasFocus(window);

	SetTextFont(renderer, nullptr, 15);
	SetTextColor(renderer, PGColor(200, 200, 182));
	//SetTextAlign(renderer, PGTextAlignRight);

	// determine the width of the line numbers
	std::vector<Cursor*> cursors = textfile->GetCursors();
	lng line_count = textfile->GetLineCount();
	text_offset = 0;
	if (this->display_linenumbers) {
		auto line_number = std::to_string(std::max((lng)10, textfile->GetLineCount() + 1));
		text_offset = 10 + MeasureTextWidth(renderer, line_number.c_str(), line_number.size());
	}
	// get the mouse position (for rendering hovers)
	PGPoint mouse = GetMousePosition(window);
	mouse.x -= this->x;
	mouse.y -= this->y;
	// determine the character width
	character_width = MeasureTextWidth(renderer, "a", 1);
	// render the line numbers
	PGScalar position_x = this->x - rectangle->x;
	PGScalar position_y = this->y - rectangle->y;
	lng linenr = textfile->GetLineOffset();
	if (this->display_linenumbers) {
		TextLine *current_line;
		while ((current_line = textfile->GetLine(linenr)) != nullptr) {
			// only render lines that fall within the render rectangle
			if (rectangle && position_y > rectangle->height) break;
			if (!rectangle || position_y + line_height >= 0) {
				// render the line number
				auto line_number = std::to_string(linenr + 1);
				RenderText(renderer, line_number.c_str(), line_number.size(), position_x, position_y);

				// if the line is selected by a cursor, render an overlay
				for (auto it = cursors.begin(); it != cursors.end(); it++) {
					if (linenr == (*it)->SelectedLine()) {
						RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, line_height), PGColor(32, 32, 255, 64), PGStyleFill);
						break;
					}
				}
			}
			linenr++;
			position_y += line_height;
		}
	}
	// render the actual text field
	//SetTextAlign(renderer, PGTextAlignLeft);
	PGScalar minimap_width = this->display_minimap ? GetMinimapWidth() : 0;
	PGScalar textfield_width = this->width - minimap_width;

	if (textfile->IsLoaded()) {
		DrawTextField(renderer, rectangle, false, this->x + text_offset - rectangle->x, this->y - rectangle->y, textfield_width, false);
	} else {
		PGScalar offset = this->width / 10;
		PGScalar width = this->width - offset * 2;
		PGScalar height = 5;
		PGScalar padding = 1;

		RenderRectangle(renderer, PGRect(offset - padding, this->height / 2 - height / 2 - padding, width + 2 * padding, height + 2 * padding), PGColor(191, 191, 191), PGStyleFill);
		RenderRectangle(renderer, PGRect(offset, this->height / 2 - height / 2, width * textfile->LoadPercentage(), height), PGColor(20, 60, 255), PGStyleFill);
	}
	// render the minimap
	if (textfile->IsLoaded() && this->display_minimap) {
		bool mouse_in_minimap = window_has_focus && mouse.x >= this->width - SCROLLBAR_WIDTH - minimap_width && mouse.x <= this->width - SCROLLBAR_WIDTH;
		PGIRect minimap_rect = PGIRect(this->x + textfield_width, this->y, minimap_width, this->height);

		SetTextFont(renderer, nullptr, 2.5f);
		DrawTextField(renderer, &minimap_rect, true, this->x + textfield_width - rectangle->x, this->y - rectangle->y, minimap_width, mouse_in_minimap);
	}

	// render the scrollbar
	if (this->display_scrollbar) {
		bool mouse_in_scrollbar = window_has_focus && mouse.x >= this->width - SCROLLBAR_WIDTH && mouse.x <= this->width;
		int y = this->y - rectangle->y;

		// the background of the scrollbar
		RenderRectangle(renderer, PGRect(x + this->width - SCROLLBAR_WIDTH, y, SCROLLBAR_WIDTH, y + this->height), PGColor(62, 62, 62), PGStyleFill);
		// the arrows above/below the scrollbar
		PGColor arrowColor(104, 104, 104);
		if (mouse_in_scrollbar && mouse.y >= 0 && mouse.y <= 16)
			arrowColor = PGColor(28, 151, 234);
		RenderTriangle(renderer, PGPoint(x + this->width - 8, y + 4), PGPoint(x + this->width - 13, y + 12), PGPoint(x + this->width - 3, y + 12), arrowColor, PGStyleFill);
		arrowColor = PGColor(104, 104, 104);
		if (mouse_in_scrollbar && mouse.y >= this->height - 16 && mouse.y <= this->height)
			arrowColor = PGColor(28, 151, 234);
		RenderTriangle(renderer, PGPoint(x + this->width - 8, y + this->height - 4), PGPoint(x + this->width - 13, y + this->height - 12), PGPoint(x + this->width - 3, y + this->height - 12), arrowColor, PGStyleFill);
		// the actual scrollbar
		PGScalar scrollbar_height = GetScrollbarHeight();
		PGScalar scrollbar_offset = GetScrollbarOffset();
		PGColor scrollbar_color = PGColor(104, 104, 104);
		if (this->drag_type == PGDragScrollbar)
			scrollbar_color = PGColor(0, 122, 204);
		else if (mouse_in_scrollbar && mouse.y >= scrollbar_offset && mouse.y <= scrollbar_offset + scrollbar_height)
			scrollbar_color = PGColor(28, 151, 234);
		RenderRectangle(renderer, PGRect(x + this->width - 12, y + scrollbar_offset, 8, scrollbar_height), scrollbar_color, PGStyleFill);
	}
}

void TextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
}

PGScalar TextField::GetScrollbarHeight() {
	return std::max((PGScalar)16, (this->GetLineHeight() * (this->height - SCROLLBAR_BASE_OFFSET * 2)) / (this->textfile->GetLineCount() + this->GetLineHeight()));;
}

PGScalar TextField::GetScrollbarOffset() {
	return SCROLLBAR_BASE_OFFSET + ((textfile->GetLineOffset() * (this->height - GetScrollbarHeight() - 2 * SCROLLBAR_BASE_OFFSET)) / std::max((lng)1, (this->textfile->GetLineCount() - 1)));
}

PGScalar TextField::GetMinimapWidth() {
	return this->width / 7.0f;
}

PGScalar TextField::GetMinimapHeight() {
	return minimap_line_height * GetLineHeight();
}

PGScalar TextField::GetMinimapOffset() {
	lng start_line = GetMinimapStartLine();
	lng lines_rendered = this->height / minimap_line_height;
	double percentage = (textfile->GetLineOffset() - start_line) / (double)lines_rendered;
	return this->height * percentage;;
}

lng TextField::GetMinimapStartLine() {
	lng lines_rendered = this->height / minimap_line_height;
	// percentage of text
	double percentage = (double)textfile->GetLineOffset() / textfile->GetLineCount();
	return std::max((lng)(textfile->GetLineOffset() - (lines_rendered * percentage)), (lng)0);
}

void TextField::SetMinimapOffset(PGScalar offset) {
	// compute lineoffset_y from minimap offset
	double percentage = (double)offset / this->height;
	lng start_line = GetMinimapStartLine();
	lng lines_rendered = this->height / minimap_line_height;
	lng lineoffset_y = start_line + (lng)(lines_rendered * percentage);
	lineoffset_y = std::max((lng)0, std::min(lineoffset_y, this->textfile->GetLineCount() - 1));
	textfile->SetLineOffset(lineoffset_y);
}

void TextField::SetScrollbarOffset(PGScalar offset) {
	// compute lineoffset_y from scrollbar offset
	// offset = SCROLLBAR_BASE_OFFSET + ((this->lineoffset_y * (this->height - GetScrollbarHeight() - 2 * SCROLLBAR_BASE_OFFSET)) / std::max((lng)1, (this->textfile->GetLineCount() - 1)));
	// offset - SCROLLBAR_BASE_OFFSET = (this->lineoffset_y * (this->height - GetScrollbarHeight() - 2 * SCROLLBAR_BASE_OFFSET)) / std::max((lng)1, this->textfile->GetLineCount() - 1);
	// std::max((lng)1, this->textfile->GetLineCount() - 1) * (offset - SCROLLBAR_BASE_OFFSET) = this->lineoffset_y * (this->height - GetScrollbarHeight() - 2 * SCROLLBAR_BASE_OFFSET)
	lng lineoffset_y = (lng)((std::max((lng)1, this->textfile->GetLineCount() - 1) * (offset - SCROLLBAR_BASE_OFFSET)) / (this->height - GetScrollbarHeight() - 2 * SCROLLBAR_BASE_OFFSET));
	lineoffset_y = std::max((lng)0, std::min(lineoffset_y, this->textfile->GetLineCount() - 1));
	textfile->SetLineOffset(lineoffset_y);
}

void TextField::GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character, bool clip_character) {
	// find the line position of the mouse
	// FIXME: account for horizontal scroll
	lng lineoffset_y = textfile->GetLineOffset();
	lng line_offset = std::max(std::min((lng)(y / this->line_height), textfile->GetLineCount() - lineoffset_y - 1), (lng)0);
	line = lineoffset_y + line_offset;
	// find the character position within the line
	x = x - this->text_offset;
	PGScalar width = 0;
	if (clip_character) {
		char* text = textfile->GetLine(line)->GetLine();
		lng length = textfile->GetLine(line)->GetLength();
		character = length;
		for (lng i = 0; i < length; i++) {
			PGScalar char_width = character_width;
			if (text[i] == '\t') {
				char_width = character_width * tabwidth;
			}
			if (width <= x && width + char_width > x) {
				character = i;
				break;
			}
			width += char_width;
		}
	} else {
		character = (int)(x / character_width);
	}
}

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (button == PGLeftMouseButton) {
		if (this->display_scrollbar && mouse.x > this->width - SCROLLBAR_WIDTH) {
			// scrollbar
			if (mouse.y <= SCROLLBAR_BASE_OFFSET) {
				textfile->OffsetLineOffset(-1);
				this->Invalidate();
			} else if (mouse.y >= this->height - SCROLLBAR_BASE_OFFSET) {
				textfile->OffsetLineOffset(1);
				this->Invalidate();
			} else {
				PGScalar scrollbar_offset = GetScrollbarOffset();
				PGScalar scrollbar_height = GetScrollbarHeight();

				if (mouse.y >= scrollbar_offset && mouse.y <= scrollbar_offset + scrollbar_height) {
					// mouse is on the scrollbar; enable dragging of the scrollbar
					drag_type = PGDragScrollbar;
					drag_offset = mouse.y - scrollbar_offset;
					this->Invalidate();
				} else if (mouse.y <= scrollbar_offset) {
					// mouse click above the scrollbar
					textfile->OffsetLineOffset(-GetLineHeight());
					this->Invalidate();
				} else {
					// mouse click below the scrollbar
					textfile->OffsetLineOffset(GetLineHeight());
					this->Invalidate();
				}
			}
			return;
		}
		if (this->display_minimap) {
			lng minimap_width = (lng)GetMinimapWidth();
			lng minimap_position = this->width - (this->display_scrollbar ? SCROLLBAR_WIDTH : 0) - minimap_width;
			if (mouse.x > minimap_position && mouse.x <= minimap_position + minimap_width) {
				PGScalar minimap_offset = GetMinimapOffset();
				PGScalar minimap_height = GetMinimapHeight();
				if (mouse.y < minimap_offset) {
					// mouse click above the minimap
					textfile->OffsetLineOffset(-GetLineHeight());
					this->Invalidate();
				} else if (mouse.y > minimap_offset + minimap_height) {
					// mouse click below the minimap
					textfile->OffsetLineOffset(GetLineHeight());
					this->Invalidate();
				} else {
					// mouse is on the minimap; enable dragging of the scrollbar
					// FIXME: drag minimap
					drag_type = PGDragMinimap;
					drag_offset = mouse.y - minimap_offset;
					this->Invalidate();
				}
				return;
			}
		}
		if (drag_type == PGDragSelectionCursors) return;
		drag_type = PGDragSelection;
		lng line = 0, character = 0;
		GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);

		time_t time = GetTime();
		if (time - last_click.time < DOUBLE_CLICK_TIME &&
			PG::abs(mouse.x - last_click.x) < 2 &&
			PG::abs(mouse.y - last_click.y) < 2) {
			last_click.clicks = last_click.clicks == 2 ? 0 : last_click.clicks + 1;
		} else {
			last_click.clicks = 0;
		}
		last_click.time = time;
		last_click.x = mouse.x;
		last_click.y = mouse.y;

		if (modifier == PGModifierNone && last_click.clicks == 0) {
			textfile->SetCursorLocation(line, character);
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SetCursorLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");"));
		} else if (modifier == PGModifierShift) {
			textfile->GetActiveCursor()->SetCursorStartLocation(line, character);
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SetCursorStartLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");"));
		} else if (modifier == PGModifierCtrl) {
			Logger::GetInstance()->WriteLogMessage(std::string("active_cursor = new Cursor(&textfile, ") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\n") + std::string("cursors.push_back(active_cursor);"));
			textfile->AddNewCursor(line, character);
		} else if (last_click.clicks == 1) {
			textfile->SetCursorLocation(line, character);
			textfile->GetActiveCursor()->SelectWord();
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SetCursorLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\ncursors[0].SelectWord();"));
		} else if (last_click.clicks == 2) {
			textfile->SetCursorLocation(line, character);
			textfile->GetActiveCursor()->SelectLine();
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SelectLine();"));
		}
		this->Invalidate();
	} else if (button == PGMiddleMouseButton) {
		if (drag_type == PGDragSelection) return;
		drag_type = PGDragSelectionCursors;
		lng line, character;
		GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
		textfile->SetCursorLocation(line, character);
		this->Invalidate();
	}
}

void TextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (button & PGLeftMouseButton) {
		if (drag_type != PGDragSelectionCursors) {
			drag_type = PGDragNone;
			this->Invalidate();
		}
	} else if (button & PGMiddleMouseButton) {
		if (drag_type == PGDragSelectionCursors) {
			drag_type = PGDragNone;
		}
	}
}

void TextField::MouseMove(int x, int y, PGMouseButton buttons) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (buttons & PGLeftMouseButton) {
		if (drag_type == PGDragSelection) {
			// FIXME: when having multiple cursors and we are altering the active cursor,
			// the active cursor can never "consume" the other selections (they should always stay)
			lng line, character;
			GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
			Cursor* active_cursor = textfile->GetActiveCursor();
			if (active_cursor->start_line != line || active_cursor->start_character != character) {
				lng old_line = active_cursor->start_line;
				active_cursor->SetCursorStartLocation(line, character);
				Logger::GetInstance()->WriteLogMessage(std::string("if (!active_cursor) active_cursor = cursors.front();\nactive_cursor->SetCursorStartLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\nCursor::NormalizeCursors(textfield, cursors, false);"));
				Cursor::NormalizeCursors(textfile, textfile->GetCursors(), false);
				this->InvalidateBetweenLines(old_line, line);
			}
		} else if (drag_type == PGDragScrollbar) {
			lng current_offset = textfile->GetLineOffset();
			SetScrollbarOffset(mouse.y - drag_offset);
			if (current_offset != textfile->GetLineOffset())
				this->Invalidate();
		} else if (drag_type == PGDragMinimap) {
			lng current_offset = textfile->GetLineOffset();
			SetMinimapOffset(mouse.y - drag_offset);
			if (current_offset != textfile->GetLineOffset())
				this->Invalidate();
		}
	} else if (buttons & PGMiddleMouseButton) {
		if (drag_type == PGDragSelectionCursors) {
			lng line, character;
			GetLineCharacterFromPosition(mouse.x, mouse.y, line, character, false);
			std::vector<Cursor*>& cursors = textfile->GetCursors();
			Cursor* active_cursor = textfile->GetActiveCursor();
			lng start_character = active_cursor->end_character;
			lng start_line = active_cursor->end_line;
			lng increment = line > active_cursor->end_line ? 1 : -1;
			cursors[0] = active_cursor;
			textfile->ClearExtraCursors();
			for (auto it = active_cursor->end_line; ; it += increment) {
				if (it != active_cursor->end_line) {
					lng line_length = textfile->GetLine(it)->GetLength();
					if (start_character == character || line_length >= start_character) {
						Cursor* cursor = new Cursor(textfile, it, start_character);
						cursor->end_character = std::min(start_character, line_length);
						cursor->start_character = std::min(character, line_length);
						cursors.push_back(cursor);
					}
				}
				if (it == line) {
					break;
				}
			}

			cursors[0]->start_character = std::min(character, textfile->GetLine(cursors[0]->start_line)->GetLength());
			this->InvalidateBetweenLines(cursors[0]->start_line, line);
		}
	} else {
		drag_type = PGDragNone;
		if (this->display_scrollbar && mouse.x >= this->width - SCROLLBAR_WIDTH && mouse.x <= this->width) {
			this->InvalidateScrollbar();
		} else if (this->display_minimap && mouse.x >= this->width - SCROLLBAR_WIDTH - GetMinimapWidth() && mouse.x <= this->width - SCROLLBAR_WIDTH) {
			this->InvalidateMinimap();
		}
	}
}

int TextField::GetLineHeight() {
	return (int)(this->height / line_height);
}

bool TextField::KeyboardButton(PGButton button, PGModifier modifier) {
	Logger::GetInstance()->WriteLogMessage(std::string("textField->KeyboardButton(") + GetButtonName(button) + std::string(", ") + GetModifierName(modifier) + std::string(");"));
	switch (button) {
		// FIXME: when moving up/down, count \t as multiple characters (equal to tab size)
		// FIXME: when moving up/down, maintain the current character number (even though a line is shorter than that number)
	case PGButtonDown:
		if (modifier == PGModifierCtrlShift) {
			textfile->MoveLines(1);
		} else if (modifier == PGModifierNone) {
			textfile->OffsetLine(1);
		} else if (modifier == PGModifierShift) {
			textfile->OffsetSelectionLine(1);
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetLineOffset(1);
		} else {
			return false;
		}
		Invalidate();
		return true;
	case PGButtonUp:
		if (modifier == PGModifierCtrlShift) {
			textfile->MoveLines(-1);
		} else if (modifier == PGModifierNone) {
			textfile->OffsetLine(-1);
		} else if (modifier == PGModifierShift) {
			textfile->OffsetSelectionLine(-1);
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetLineOffset(-1);
		} else {
			return false;
		}
		Invalidate();
		return true;
	case PGButtonLeft:
		if (modifier == PGModifierNone) {
			textfile->OffsetCharacter(-1);
		} else if (modifier == PGModifierShift) {
			textfile->OffsetSelectionCharacter(-1);
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetWord(PGDirectionLeft);
		} else if (modifier == PGModifierCtrlShift) {
			textfile->OffsetSelectionWord(PGDirectionLeft);
		} else {
			return false;
		}
		Invalidate();
		return true;
	case PGButtonRight:
		if (modifier == PGModifierNone) {
			textfile->OffsetCharacter(1);
		} else if (modifier == PGModifierShift) {
			textfile->OffsetSelectionCharacter(1);
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetWord(PGDirectionRight);
		} else if (modifier == PGModifierCtrlShift) {
			textfile->OffsetSelectionWord(PGDirectionRight);
		} else {
			return false;
		}
		Invalidate();
		return true;
	case PGButtonEnd:
		if (modifier == PGModifierNone) {
			textfile->OffsetEndOfLine();
		} else if (modifier == PGModifierShift) {
			textfile->SelectEndOfLine();
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetEndOfFile();
		} else if (modifier == PGModifierCtrlShift) {
			textfile->SelectEndOfFile();
		} else {
			return false;
		}
		Invalidate();
		return true;
	case PGButtonHome:
		if (modifier == PGModifierNone) {
			textfile->OffsetStartOfLine();
		} else if (modifier == PGModifierShift) {
			textfile->SelectStartOfLine();
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetStartOfFile();
		} else if (modifier == PGModifierCtrlShift) {
			textfile->SelectStartOfFile();
		}
		Invalidate();
		break;
	case PGButtonPageUp:
		if (modifier == PGModifierNone) {
			textfile->OffsetLine(-GetLineHeight());
			Invalidate();
			return true;
		}
		return false;
	case PGButtonPageDown:
		if (modifier == PGModifierNone) {
			textfile->OffsetLine(GetLineHeight());
			Invalidate();
			return true;
		}
		return false;
	case PGButtonDelete:
		if (modifier == PGModifierNone) {
			this->textfile->DeleteCharacter(PGDirectionRight);
		} else if (modifier == PGModifierCtrl) {
			this->textfile->DeleteWord(PGDirectionRight);
		} else if (modifier == PGModifierShift) {
			textfile->DeleteLines();
		} else if (modifier == PGModifierCtrlShift) {
			this->textfile->DeleteLine(PGDirectionRight);
		} else {
			return false;
		}
		this->Invalidate();
		return true;
	case PGButtonBackspace:
		if (modifier == PGModifierNone || modifier == PGModifierShift) {
			this->textfile->DeleteCharacter(PGDirectionLeft);
		} else if (modifier == PGModifierCtrl) {
			this->textfile->DeleteWord(PGDirectionLeft);
		} else if (modifier == PGModifierCtrlShift) {
			this->textfile->DeleteLine(PGDirectionLeft);
		}  else {
			return false;
		}
		this->Invalidate();
		return true;
	case PGButtonEnter:
		if (modifier == PGModifierNone) {
			this->textfile->AddNewLine();
		} else if (modifier == PGModifierCtrl) {
			this->textfile->AddEmptyLine(PGDirectionRight);
		} else if (modifier == PGModifierCtrlShift) {
			this->textfile->AddEmptyLine(PGDirectionLeft);
		} else {
			return false;
		}
		this->Invalidate();
		return true;
	default:
		return false;
	}
}

void TextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;

	Logger::GetInstance()->WriteLogMessage(std::string("textField->KeyboardCharacter(") + std::to_string(character) + std::string(", ") + GetModifierName(modifier) + std::string(");"));

	if (modifier == PGModifierNone) {
		this->textfile->InsertText(character);
		this->Invalidate();
	} else {
		if (modifier & PGModifierCtrl) {
			switch (character) {
			case 'Z':
				this->textfile->Undo();
				this->Invalidate();
				break;
			case 'Y':
				this->textfile->Redo();
				this->Invalidate();
				break;
			case 'A':
				this->textfile->SelectEverything();
				this->Invalidate();
				break;
			case 'C': {
				std::string text = textfile->CopyText();
				SetClipboardText(window, text);
				break;
			}
			case 'V': {
				if (modifier & PGModifierShift) {
					// FIXME: cycle through previous copy-pastes
				} else {
					textfile->PasteText(GetClipboardText(window));
				}
				this->Invalidate();
				break;
			}
			case 'S': {
				textfile->SaveChanges();
				break;
			}
			}
		}
	}
}

void TextField::KeyboardUnicode(char *character, PGModifier modifier) {
	// FIXME
}

void TextField::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		lng lineoffset_y = textfile->GetLineOffset();
		if (SetScrollOffset(lineoffset_y - (distance / 120) * 2)) {
			this->Invalidate();
		}
	}
}

bool
TextField::SetScrollOffset(lng offset) {
	lng lineoffset_y = textfile->GetLineOffset();
	lng new_y = std::min(std::max(offset, (lng)0), (lng)(textfile->GetLineCount() - 1));
	if (new_y != lineoffset_y) {
		textfile->SetLineOffset(new_y);
		return true;
	}
	return false;
}

void TextField::InvalidateLine(lng line) {
	lng lineoffset_y = textfile->GetLineOffset();
	this->Invalidate(PGRect(0, (line - lineoffset_y) * line_height, this->width, line_height));
}

void TextField::InvalidateBeforeLine(lng line) {
	lng lineoffset_y = textfile->GetLineOffset();
	this->Invalidate(PGRect(0, 0, this->width, (line - lineoffset_y) * line_height));
}

void TextField::InvalidateAfterLine(lng line) {
	lng lineoffset_y = textfile->GetLineOffset();
	this->Invalidate(PGRect(0, (line - lineoffset_y) * line_height, this->width, this->height));
}

void TextField::InvalidateBetweenLines(lng start, lng end) {
	if (start > end) {
		InvalidateBetweenLines(end, start);
		return;
	}
	lng lineoffset_y = textfile->GetLineOffset();
	this->Invalidate(PGRect(0, (start - lineoffset_y) * line_height, this->width,
		(end - lineoffset_y) * line_height - (start - lineoffset_y) * line_height + line_height));
}

void TextField::InvalidateScrollbar() {
	this->Invalidate(PGRect(this->width - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, this->height));
}

void TextField::InvalidateMinimap() {
	PGScalar minimap_width = GetMinimapWidth();
	this->Invalidate(PGRect(this->width - SCROLLBAR_WIDTH - minimap_width, 0, minimap_width, this->height));
}

void TextField::SetTextFile(TextFile* textfile) {
	this->textfile = textfile;
	textfile->SetTextField(this);
	this->Invalidate();
}

void TextField::OnResize(PGSize old_size, PGSize new_size) {
	minimap_region.width = GetMinimapWidth();
	minimap_region.height = new_size.height;
	textfield_region.width = new_size.width - GetMinimapWidth() - SCROLLBAR_WIDTH;
	textfield_region.height = new_size.height;
	minimap_region.x = textfield_region.width;
}

PGCursorType TextField::GetCursor(PGPoint mouse) {
	mouse.x -= x;
	mouse.y -= y;
	if (!textfile->IsLoaded()) {
		return PGCursorWait;
	}
	if (mouse.x <= textfield_region.width) {
		return PGCursorIBeam;
	}
	return PGCursorStandard;
}

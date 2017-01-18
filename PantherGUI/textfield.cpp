
#include "textfield.h"
#include <sstream>
#include <algorithm>
#include "logger.h"
#include "text.h"
#include "controlmanager.h"
#include "style.h"
#include "syntax.h"

#include "container.h"
#include "simpletextfield.h"
#include "statusbar.h"

#include "searchbox.h"
#include "settings.h"

#define SCROLLBAR_PADDING 4


std::map<std::string, PGKeyFunction> TextField::keybindings_noargs;
std::map<std::string, PGKeyFunctionArgs> TextField::keybindings_varargs;
std::map<PGKeyPress, PGKeyFunctionCall> TextField::keybindings;


void TextField::MinimapMouseEvent(bool mouse_enter) {
	this->mouse_in_minimap = mouse_enter;
	this->InvalidateMinimap();
}

TextField::TextField(PGWindowHandle window, TextFile* file) :
	BasicTextField(window, file), display_scrollbar(true), display_minimap(true), display_linenumbers(true) {
	textfile->SetTextField(this);

	ControlManager* manager = GetControlManager(this);
	manager->RegisterMouseRegion(&minimap_region, this, [](Control* tf, bool mouse_enter, void* data) {
		return ((TextField*)tf)->MinimapMouseEvent(mouse_enter);
	});

	textfield_font = PGCreateFont();
	minimap_font = PGCreateFont();

	scrollbar = new Scrollbar(this, window, false, false);
	scrollbar->SetPosition(PGPoint(this->width - SCROLLBAR_SIZE, 0));
	scrollbar->bottom_padding = SCROLLBAR_PADDING;
	scrollbar->top_padding = SCROLLBAR_PADDING + SCROLLBAR_SIZE;
	scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((TextField*)scroll->parent)->GetTextFile().SetLineOffset(value);
	});
	horizontal_scrollbar = new Scrollbar(this, window, true, false);
	horizontal_scrollbar->bottom_padding = SCROLLBAR_PADDING;
	horizontal_scrollbar->top_padding = SCROLLBAR_PADDING + SCROLLBAR_SIZE;
	horizontal_scrollbar->SetPosition(PGPoint(0, this->height - SCROLLBAR_SIZE));
	horizontal_scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((TextField*)scroll->parent)->GetTextFile().SetXOffset(value);
	});

	int size = 0;
	if (PGSettingsManager::GetSetting("font_size", size)) {
		SetTextFontSize(textfield_font, size);
	} else {
		SetTextFontSize(textfield_font, 15);
	}
	PGSettingsManager::GetSetting("display_line_numbers", display_linenumbers);
	PGSettingsManager::GetSetting("display_minimap", display_minimap);
	SetTextFontSize(minimap_font, 2.5f);
	margin_width = 4;
}

TextField::~TextField() {

}

void TextField::DrawTextField(PGRendererHandle renderer, PGFontHandle font, PGIRect* rectangle, bool minimap, PGScalar position_x, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay) {
	PGScalar xoffset = 0;
	PGScalar max_x = position_x_text + width;
	if (!minimap)
		xoffset = textfile->GetXOffset();
	PGScalar y = Y() - rectangle->y;
	lng start_line = textfile->GetLineOffset();
	std::vector<Cursor*> cursors = textfile->GetCursors();
	lng linenr = start_line;
	TextLine current_line;
	PGColor selection_color = PGStyleManager::GetColor(PGColorTextFieldSelection);
	PGScalar line_height = GetTextHeight(font);
	PGScalar initial_position_y = position_y;
	PGScalar start_position_y = position_y;

	if (minimap) {
		// fill in the background of the minimap
		PGRect rect(position_x_text, position_y, this->width - position_x_text, this->height - position_y);
		RenderRectangle(renderer, rect, PGColor(30, 30, 30), PGDrawStyleFill);
		// start line of the minimap
		start_line = GetMinimapStartLine();
		linenr = start_line;
		start_position_y = position_y + GetMinimapOffset();
	}

	textfile->Lock(PGReadLock);
	std::string selected_word = std::string();
	if (!minimap) {
		selected_word = cursors[0]->GetSelectedWord();
	}
	linenr = start_line + (lng)(rectangle->height / line_height);
	PGScalar text_offset = position_x_text - position_x - margin_width * 2;
	if (!minimap && display_linenumbers) {
		// clear the linenumber region
		RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);
		if (xoffset > 0) {
			RenderGradient(renderer, PGRect(position_x + text_offset, position_y, margin_width, this->height), PGColor(0, 0, 0, 128), PGColor(0, 0, 0, 0));
		}
	}

	linenr = start_line;
	position_y = initial_position_y;
	lng block = -1;
	bool parsed = false;

	bool toggle = false;
	TextLineIterator* line_iterator;
	if (textfile->current_linenumber >= 0) {
		line_iterator = textfile->GetLineIterator(this, textfile->current_linenumber);
	} else {
		line_iterator = textfile->GetScrollIterator(this, linenr);
	}
	auto buffer = line_iterator->CurrentBuffer();
	toggle = PGTextBuffer::GetBuffer(textfile->buffers, buffer->start_line) % 2 != 0;
	lng current_cursor = 0;
	lng current_match = 0;
	auto matches = textfile->GetFindMatches();

	if (!minimap) {
		rendered_lines.clear();
	}
	while ((current_line = line_iterator->GetLine()).IsValid()) {
		lng current_start_line = line_iterator->GetCurrentLineNumber();
		lng current_start_position = line_iterator->GetCurrentCharacterNumber();

		if (!minimap) {
			rendered_lines.push_back(RenderedLine(current_line, current_start_line, current_start_position));
		}

		// only render lines that fall within the render rectangle
		if (position_y > rectangle->height) break;
		if (!(position_y + line_height < 0)) {
			// render the linenumber of the current line if it is not wrapped
			if (!minimap && display_linenumbers && current_start_position == 0) {
				lng displayed_line = current_start_line;
				SetTextColor(textfield_font, PGStyleManager::GetColor(PGColorTextFieldLineNumber));

				for (auto it = cursors.begin(); it != cursors.end(); it++) {
					if (displayed_line == (*it)->SelectedPosition().line) {
						RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, line_height), PGStyleManager::GetColor(PGColorTextFieldSelection), PGDrawStyleFill);
						break;
					}
				}

				// render the line number
				auto line_number = std::to_string(displayed_line + 1);
				RenderText(renderer, textfield_font, line_number.c_str(), line_number.size(), position_x + margin_width, position_y);
			}

			SetRenderBounds(renderer, PGRect(position_x_text, y, this->width, this->height));

			char* line = current_line.GetLine();
			lng length = current_line.GetLength();
			// render the selections and cursors, if there are any on this line
			while (current_cursor < cursors.size()) {
				auto begin_pos = cursors[current_cursor]->BeginPosition();
				if (begin_pos.line > current_start_line ||
					begin_pos.line == current_start_line && begin_pos.character > current_start_position + length) {
					// this cursor is not rendered on this line yet
					break;
				}
				auto end_pos = cursors[current_cursor]->EndPosition();
				if (end_pos.line < current_start_line ||
					end_pos.line == current_start_line && end_pos.character < current_start_position) {
					// this cursor has already been rendered
					current_cursor++;
					continue;
				}
				// we should render the cursor on this line
				auto selected_pos = cursors[current_cursor]->SelectedPosition();

				lng start, end;
				if (current_start_line == begin_pos.line) {
					if (current_start_line == end_pos.line) {
						// start and end are on the same line
						start = begin_pos.position - current_start_position;
						end = end_pos.position - current_start_position;
					} else {
						// only "start" is on this line
						start = begin_pos.position - current_start_position;
						end = length + 1;
					}
				} else if (current_start_line == end_pos.line) {
					// only "end" is on this line
					start = 0;
					end = end_pos.position - current_start_position;
				} else {
					// the entire line is located in the selection
					start = 0;
					end = length + 1;
				}

				start = std::min(length, std::max((lng)0, start));
				end = std::min(length + 1, std::max((lng)0, end));
				if (start != end) {
					// if there is a selection, render it
					RenderSelection(renderer,
						font,
						line,
						length,
						position_x_text - xoffset,
						position_y,
						start,
						end,
						selection_color,
						max_x);
				}

				if (!minimap && current_start_line == selected_pos.line) {
					// render the caret if it occurs on this line
					lng selected_position = selected_pos.position - current_start_position;
					if (display_carets && selected_position >= 0 && selected_position <= length) {
						// render the caret on the selected line
						RenderCaret(renderer,
							font,
							current_line.GetLine(),
							current_line.GetLength(),
							position_x_text - xoffset,
							position_y,
							selected_position,
							line_height,
							PGStyleManager::GetColor(PGColorTextFieldCaret));
					}
				}
				if (end < length) {
					current_cursor++;
					continue;
				}
				break;
			}

			// render any search matches
			while (current_match < matches.size()) {
				auto match = matches[current_match];
				if (match.start_line > current_start_line ||
					match.start_line == current_start_line && match.start_character > current_start_position + length) {
					// this match is not rendered on this line yet
					break;
				}
				if (match.end_line < current_start_line ||
					match.end_line == current_start_line && match.end_character <= current_start_position) {
					// this match has already been rendered
					current_match++;
					continue;
				}
				// we should render the match on this line
				lng start, end;
				if (match.start_line == current_start_line) {
					if (match.end_line == current_start_line) {
						// start and end are on the same line
						start = match.start_character - current_start_position;
						end = match.end_character - current_start_position;
					} else {
						start = match.start_character - current_start_position;
						end = length;
					}
				} else if (match.end_line == current_start_line) {
					start = 0;
					end = match.end_character - current_start_position;
				} else {
					start = 0;
					end = length;
				}

				PGScalar x_offset = MeasureTextWidth(font, line, start);
				PGScalar width = MeasureTextWidth(font, line + start, end - start);
				PGRect rect(position_x_text + x_offset - xoffset, position_y, width, line_height);
				RenderRectangle(renderer, rect, PGStyleManager::GetColor(PGColorTextFieldText), PGDrawStyleStroke);
				if (end < length) {
					current_match++;
					continue;
				}
				break;
			}


			// render the actual text
			lng position = 0;

			if (line_iterator->CurrentBuffer() != buffer) {
				buffer = line_iterator->CurrentBuffer();
				toggle = !toggle;
			}

			if (toggle && !minimap) {
				RenderRectangle(renderer, PGRect(position_x_text, position_y, this->width, line_height), PGColor(72, 72, 72, 60), PGDrawStyleFill);
			}

			// because RenderText is expensive, we cache rendered text lines for the minimap
			// this is because the minimap renders far more lines than the textfield (generally 10x more)
			// we cache by simply rendering the textline to a bitmap, and then rendering the bitmap
			// to the screen
			PGBitmapHandle line_bitmap = nullptr;
			PGRendererHandle line_renderer = renderer;
			bool render_text = true;
			if (minimap) {
				// first check if the line is found in the cache, if it is not, we rerender
				render_text = !minimap_line_cache.count(linenr);
				if (render_text) {
					// we have to render, create the bitmap and a renderer for the bitmap
					line_bitmap = CreateBitmapForText(font, line, length);
					line_renderer = CreateRendererForBitmap(line_bitmap);
				}
			}

			PGScalar xpos = position_x_text - xoffset;
			if (render_text) {
				// if we are rendering the line into a bitmap, it starts at (0,0)
				// otherwise we render onto the screen normally
				PGScalar bitmap_x = minimap ? 0 : xpos;
				PGScalar bitmap_y = minimap ? 0 : position_y;
				PGSyntax* syntax = &current_line.syntax;
				while (syntax && syntax->end > 0) {
					bool squiggles = false;
					//assert(syntax->end > position);
					if (syntax->end <= position) {
						syntax = syntax->next;
						continue;
					}
					if (syntax->type == PGSyntaxError) {
						squiggles = true;
					} else if (syntax->type == PGSyntaxNone) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
					} else if (syntax->type == PGSyntaxString) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxString));
					} else if (syntax->type == PGSyntaxConstant) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxConstant));
					} else if (syntax->type == PGSyntaxComment) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxComment));
					} else if (syntax->type == PGSyntaxOperator) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxOperator));
					} else if (syntax->type == PGSyntaxFunction) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxFunction));
					} else if (syntax->type == PGSyntaxKeyword) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxKeyword));
					} else if (syntax->type == PGSyntaxClass1) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxClass1));
					} else if (syntax->type == PGSyntaxClass2) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxClass2));
					} else if (syntax->type == PGSyntaxClass3) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxClass3));
					} else if (syntax->type == PGSyntaxClass4) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxClass4));
					} else if (syntax->type == PGSyntaxClass5) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxClass5));
					} else if (syntax->type == PGSyntaxClass6) {
						SetTextColor(font, PGStyleManager::GetColor(PGColorSyntaxClass6));
					}
					RenderText(line_renderer, font, line + position, syntax->end - position, bitmap_x, bitmap_y);
					PGScalar text_width = MeasureTextWidth(font, line + position, syntax->end - position);
					bitmap_x += text_width;
					position = syntax->end;
					syntax = syntax->next;
				}
				if (length > position) {
					SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
					RenderText(line_renderer, font, line + position, length - position, bitmap_x, bitmap_y);
				}
				if (minimap) {
					// we rendered into a bitmap: delete the renderer and store the line
					DeleteRenderer(line_renderer);
					minimap_line_cache[linenr] = line_bitmap;
				}
			} else {
				// if the line is already cached, simply retrieve the bitmap
				line_bitmap = minimap_line_cache[linenr];
			}
			if (minimap) {
				// render the cached bitmap to the screen
				RenderImage(renderer, line_bitmap, (int)xpos, (int)position_y);
			}
			if ((lng)selected_word.size() > 0 && length >= (lng)selected_word.size()) {
				// FIXME: use strstr here instead of implementing the search ourself
				for (lng i = 0; i <= length - selected_word.size(); i++) {
					if ((i == 0 || GetCharacterClass(line[i - 1]) != PGCharacterTypeText) &&
						GetCharacterClass(line[i]) == PGCharacterTypeText) {
						bool found = true;
						for (lng j = 0; j < selected_word.size(); j++) {
							if (line[i + j] != selected_word[j]) {
								found = false;
								break;
							}
						}
						if (found) {
							if ((i + selected_word.size() == length ||
								GetCharacterClass(line[i + selected_word.size()]) != PGCharacterTypeText)) {
								PGScalar x_offset = MeasureTextWidth(font, line, i);
								PGScalar width = MeasureTextWidth(font, selected_word.c_str(), selected_word.size());
								PGRect rect(position_x_text + x_offset - xoffset, position_y, width, line_height);
								RenderRectangle(renderer, rect, PGStyleManager::GetColor(PGColorTextFieldText), PGDrawStyleStroke);
							}
						}
					}
				}
			}
			ClearRenderBounds(renderer);
		}
		linenr++;
		(*line_iterator)++;
		position_y += line_height;
	}
	textfile->Unlock(PGReadLock);
	ClearRenderBounds(renderer);
	if (!minimap) {
		this->line_height = line_height;
	} else {
		// to prevent potential memory explosion we limit the size of the minimap line cache
		if (minimap_line_cache.size() > MAX_MINIMAP_LINE_CACHE) {
			lng i = 0;
			// we just randomly delete 10% of the line cache when the line cache is full
			// there is probably a better way of doing this
			for (auto it = minimap_line_cache.begin(); it != minimap_line_cache.end(); it++) {
				DeleteImage(it->second);
				minimap_line_cache.erase(it++);
				i++;
				if (i > MAX_MINIMAP_LINE_CACHE / 10) break;
			}
		}

		this->minimap_line_height = line_height;
		if (render_overlay) {
			// render the overlay for the minimap
			PGRect rect(position_x_text, start_position_y, this->width - position_x_text, line_height * GetLineHeight());
			RenderRectangle(renderer, rect,
				this->drag_type == PGDragMinimap ?
				PGStyleManager::GetColor(PGColorMinimapDrag) :
				PGStyleManager::GetColor(PGColorMinimapHover)
				, PGDrawStyleFill);
		}
	}
}

void TextField::Draw(PGRendererHandle renderer, PGIRect* r) {
	bool window_has_focus = WindowHasFocus(window);
	PGIRect rect = PGIRect(r->x, r->y, std::min(r->width, (int)(X() + this->width - r->x)), std::min(r->height, (int)(Y() + this->height - r->y)));
	PGIRect* rectangle = &rect;

	// determine the width of the line numbers
	std::vector<Cursor*> cursors = textfile->GetCursors();
	lng line_count = textfile->GetMaxYScroll();
	text_offset = 0;
	if (this->display_linenumbers) {
		auto line_number = std::to_string(std::max((lng)10, textfile->GetMaxYScroll() + 1));
		text_offset = 10 + MeasureTextWidth(textfield_font, line_number.c_str(), line_number.size());
	}
	PGPoint position = Position();
	PGScalar x = position.x - rectangle->x, y = position.y - rectangle->y;
	// get the mouse position (for rendering hovers)
	PGPoint mouse = GetMousePosition(window, this);
	PGScalar position_x = x;
	PGScalar position_y = y;
	// textfield/minimap dimensions
	PGScalar minimap_width = this->display_minimap ? GetMinimapWidth() : 0;
	PGScalar textfield_width = this->width - minimap_width;
	// determine x-offset and clamp it
	PGScalar max_textsize = textfile->GetMaxLineWidth(textfield_font);
	PGScalar xoffset = 0;
	if (textfile->GetWordWrap()) {
		max_xoffset = 0;
	} else {
		max_xoffset = std::max(max_textsize - textfield_width + text_offset + 2 * margin_width + 10, 0.0f);
		xoffset = this->textfile->GetXOffset();
		if (xoffset > max_xoffset) {
			xoffset = max_xoffset;
			this->textfile->SetXOffset(max_xoffset);
		}
	}
	// render the actual text field
	if (textfile->IsLoaded()) {
		textfield_region.width = textfield_width - text_offset - 5;
		textfield_region.height = this->height;
		DrawTextField(renderer, textfield_font, rectangle, false, x, x + text_offset + margin_width * 2, y, textfield_width, false);
	} else {
		PGScalar offset = this->width / 10;
		PGScalar width = this->width - offset * 2;
		PGScalar height = 5;
		PGScalar padding = 1;

		RenderRectangle(renderer, PGRect(offset - padding, this->height / 2 - height / 2 - padding, width + 2 * padding, height + 2 * padding), PGColor(191, 191, 191), PGDrawStyleFill);
		RenderRectangle(renderer, PGRect(offset, this->height / 2 - height / 2, width * textfile->LoadPercentage(), height), PGColor(20, 60, 255), PGDrawStyleFill);
	}
	// render the minimap
	if (textfile->IsLoaded() && this->display_minimap) {
		bool mouse_in_minimap = window_has_focus && this->mouse_in_minimap;
		PGIRect minimap_rect = PGIRect(x + textfield_width, rectangle->y, minimap_width, rectangle->height);
		PGIRect shadow_rect = PGIRect(minimap_rect.x - 5, y, 5, minimap_rect.height);

		RenderGradient(renderer, shadow_rect, PGColor(0, 0, 0, 0), PGColor(0, 0, 0, 128));
		DrawTextField(renderer, minimap_font, &minimap_rect, true, x + textfield_width, x + textfield_width, y, minimap_width, mouse_in_minimap);
	}

	// render the scrollbar
	if (this->display_scrollbar) {
		scrollbar->UpdateValues(0, textfile->GetMaxYScroll(), GetLineHeight(), textfile->GetLineOffset());
		scrollbar->Draw(renderer, rectangle);
		// horizontal scrollbar
		display_horizontal_scrollbar = max_xoffset > 0;
		if (display_horizontal_scrollbar) {
			horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), textfile->GetXOffset());
			horizontal_scrollbar->Draw(renderer, rectangle);
		}
	}
}

void TextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
}

PGScalar TextField::GetTextfieldWidth() {
	return textfield_region.width;
	return display_minimap ? this->width - SCROLLBAR_SIZE - GetMinimapWidth() : this->width - SCROLLBAR_SIZE;
}

PGScalar TextField::GetTextfieldHeight() {
	return textfield_region.height;
	return display_horizontal_scrollbar ? this->height - SCROLLBAR_SIZE : this->height;
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
	lng lines_rendered = this->height / (minimap_line_height == 0 ? 1 : minimap_line_height);
	// percentage of text
	double percentage = (double)textfile->GetLineOffset() / textfile->GetMaxYScroll();
	return std::min(std::max((lng)(textfile->GetLineOffset() - (lines_rendered * percentage)), (lng)0), this->textfile->GetMaxYScroll());
}

void TextField::SetMinimapOffset(PGScalar offset) {
	// compute lineoffset_y from minimap offset
	double percentage = (double)offset / this->height;
	lng lines_rendered = this->height / minimap_line_height;
	lng start_line = std::max((lng)(((lng)((std::max((lng)1, this->textfile->GetMaxYScroll()) * percentage))) - (lines_rendered * percentage)), (lng)0);
	lng lineoffset_y = start_line + (lng)(lines_rendered * percentage);
	lineoffset_y = std::max((lng)0, std::min(lineoffset_y, this->textfile->GetMaxYScroll()));
	textfile->SetLineOffset(lineoffset_y);
}

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (PGRectangleContains(scrollbar->GetRectangle(), mouse)) {
		scrollbar->UpdateValues(0, textfile->GetMaxYScroll(), GetLineHeight(), textfile->GetLineOffset());
		scrollbar->MouseDown(mouse.x, mouse.y, button, modifier);
		return;
	}
	if (PGRectangleContains(horizontal_scrollbar->GetRectangle(), mouse)) {
		horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), textfile->GetXOffset());
		horizontal_scrollbar->MouseDown(mouse.x, mouse.y, button, modifier);
		return;
	}
	if (button == PGLeftMouseButton) {
		if (this->display_minimap) {
			lng minimap_width = (lng)GetMinimapWidth();
			lng minimap_position = this->width - (this->display_scrollbar ? SCROLLBAR_SIZE : 0) - minimap_width;
			if (mouse.x > minimap_position && mouse.x <= minimap_position + minimap_width && (
				!display_horizontal_scrollbar || mouse.y <= this->height - SCROLLBAR_SIZE)) {
				PGScalar minimap_offset = GetMinimapOffset();
				PGScalar minimap_height = GetMinimapHeight();
				if ((mouse.y < minimap_offset) || (mouse.y > minimap_offset + minimap_height)) {
					// mouse click above/below the minimap, move the minimap to the mouse
					SetMinimapOffset(mouse.y - minimap_height / 2.0f);
					drag_type = PGDragMinimap;
					drag_offset = minimap_height / 2.0f;
					this->Invalidate();
				} else {
					// mouse is on the minimap; enable dragging
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

		PerformMouseClick(mouse);

		if (modifier == PGModifierNone && last_click.clicks == 0) {
			textfile->SetCursorLocation(line, character);

		} else if (modifier == PGModifierShift) {
			textfile->GetActiveCursor()->SetCursorStartLocation(line, character);
		} else if (modifier == PGModifierCtrl) {
			textfile->AddNewCursor(line, character);
		} else if (last_click.clicks == 1) {
			textfile->SetCursorLocation(line, character);
			Cursor* active_cursor = textfile->GetActiveCursor();
			active_cursor->SelectWord();
			minimal_selections[active_cursor] = active_cursor->GetCursorSelection();
		} else if (last_click.clicks == 2) {
			textfile->SetCursorLocation(line, character);
			Cursor* active_cursor = textfile->GetActiveCursor();
			active_cursor->SelectLine();
			minimal_selections[active_cursor] = active_cursor->GetCursorSelection();
		}
	} else if (button == PGMiddleMouseButton) {
		if (drag_type == PGDragSelection) return;
		drag_type = PGDragSelectionCursors;
		drag_offset = mouse.x;
		lng line, character;
		GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
		textfile->SetCursorLocation(line, character);
	} else if (button == PGRightMouseButton) {
		/*
		if (drag_type != PGDragNone) return;
		lng line, character;
		GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
		textfile->ClearExtraCursors();
		textfile->SetCursorLocation(line, character);*/
	}
}

void TextField::ClearDragging() {
	minimal_selections.clear();
	drag_type = PGDragNone;
}

void TextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (PGRectangleContains(scrollbar->GetRectangle(), mouse)) {
		scrollbar->UpdateValues(0, textfile->GetMaxYScroll(), GetLineHeight(), textfile->GetLineOffset());
		scrollbar->MouseUp(mouse.x, mouse.y, button, modifier);
		return;
	}
	if (PGRectangleContains(horizontal_scrollbar->GetRectangle(), mouse)) {
		horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), textfile->GetXOffset());
		horizontal_scrollbar->MouseUp(mouse.x, mouse.y, button, modifier);
		return;
	}
	if (button & PGLeftMouseButton) {
		if (drag_type != PGDragSelectionCursors) {
			ClearDragging();
			this->Invalidate();
		}
	} else if (button & PGMiddleMouseButton) {
		if (drag_type == PGDragSelectionCursors) {
			drag_type = PGDragNone;
		}
	} else if (button & PGRightMouseButton) {
		if (!(mouse.x <= GetTextfieldWidth() && mouse.y <= GetTextfieldHeight())) return;
		PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
		PGPopupMenuInsertEntry(menu, "Show Unsaved Changes...", nullptr, PGPopupMenuGrayed);
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertEntry(menu, "Copy", [](Control* control) {
			SetClipboardText(control->window, dynamic_cast<TextField*>(control)->textfile->CopyText());
		});
		PGPopupMenuInsertEntry(menu, "Cut", nullptr, PGPopupMenuGrayed);
		PGPopupMenuInsertEntry(menu, "Paste", [](Control* control) {
			std::string clipboard_text = GetClipboardText(control->window);
			dynamic_cast<TextField*>(control)->textfile->PasteText(clipboard_text);
		});
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertEntry(menu, "Select All", [](Control* control) {
			dynamic_cast<TextField*>(control)->textfile->SelectEverything();
		});
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuFlags flags = this->textfile->FileInMemory() ? PGPopupMenuGrayed : PGPopupMenuFlagsNone;
		PGPopupMenuInsertEntry(menu, "View File In Explorer", [](Control* control) {
			OpenFolderInExplorer(dynamic_cast<TextField*>(control)->textfile->GetFullPath());
		}, flags);
		PGPopupMenuInsertEntry(menu, "Open Directory in Terminal", [](Control* control) {
			OpenFolderInTerminal(dynamic_cast<TextField*>(control)->textfile->GetFullPath());
		}, flags);
		PGPopupMenuInsertEntry(menu, "Copy File Path", [](Control* control) {
			SetClipboardText(control->window, dynamic_cast<TextField*>(control)->textfile->GetFullPath());
		}, flags);
		PGPopupMenuInsertEntry(menu, "Reveal in Side Bar", nullptr, flags);
		PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
	}
}

void TextField::MouseMove(int x, int y, PGMouseButton buttons) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (scrollbar->IsDragging()) {
		scrollbar->UpdateValues(0, textfile->GetMaxYScroll(), GetLineHeight(), textfile->GetLineOffset());
		scrollbar->MouseMove(mouse.x, mouse.y, buttons);
		return;
	}
	if (horizontal_scrollbar->IsDragging()) {
		horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), textfile->GetXOffset());
		horizontal_scrollbar->MouseMove(mouse.x, mouse.y, buttons);
		return;
	}
	if (buttons & PGLeftMouseButton) {
		if (drag_type == PGDragSelection) {
			// FIXME: when having multiple cursors and we are altering the active cursor,
			// the active cursor can never "consume" the other selections (they should always stay)
			lng line, character;
			GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
			Cursor* active_cursor = textfile->GetActiveCursor();
			auto selected_pos = active_cursor->SelectedPosition();
			if (selected_pos.line != line || selected_pos.character != character) {
				lng old_line = selected_pos.line;
				active_cursor->SetCursorStartLocation(line, character);
				if (minimal_selections.count(active_cursor) > 0) {
					active_cursor->ApplyMinimalSelection(minimal_selections[active_cursor]);
				}
				Cursor::NormalizeCursors(textfile, textfile->GetCursors());
			}
		} else if (drag_type == PGDragMinimap) {
			lng current_offset = textfile->GetLineOffset();
			SetMinimapOffset(mouse.y - drag_offset);
			if (current_offset != textfile->GetLineOffset())
				this->Invalidate();
		}
	} else if (buttons & PGMiddleMouseButton) {
		if (drag_type == PGDragSelectionCursors) {
			/*
			lng line;
			GetLineFromPosition(mouse.y, line);
			std::vector<Cursor*>& cursors = textfile->GetCursors();
			Cursor* active_cursor = textfile->GetActiveCursor();
			auto end_pos = active_cursor->UnselectedPosition();
			lng increment = line > end_pos.line ? 1 : -1;
			textfile->ClearCursors();
			// FIXME: word wrap
			for (auto it = end_pos.line; ; it += increment) {
				lng start_character, end_character;
				TextLine current_line = textfile->GetLine(it);
				GetCharacterFromPosition(drag_offset, current_line, start_character);
				GetCharacterFromPosition(mouse.x, current_line, end_character);
				Cursor* cursor = new Cursor(textfile, it, end_character, it, start_character);
				cursors.push_back(cursor);
				if (it == line)
					break;
			}
			this->Invalidate();*/
		}
	} else {
		ClearDragging();
		if (this->display_minimap && mouse.x >= this->width - SCROLLBAR_SIZE - GetMinimapWidth() && mouse.x <= this->width - SCROLLBAR_SIZE) {
			this->InvalidateMinimap();
		}
	}
}

bool TextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(TextField::keybindings, button, modifier)) {
		return true;
	}
	switch (button) {
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
		return true;
	case PGButtonPageUp:
		if (modifier == PGModifierNone) {
			textfile->OffsetLine(-GetLineHeight());
			return true;
		}
		return false;
	case PGButtonPageDown:
		if (modifier == PGModifierNone) {
			textfile->OffsetLine(GetLineHeight());
			return true;
		}
		return false;
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
		return true;
	default:
		break;
	}
	return BasicTextField::KeyboardButton(button, modifier);
}

void TextField::IncreaseFontSize(int modifier) {
	SetTextFontSize(textfield_font, GetTextFontSize(textfield_font) + modifier);
	this->Invalidate();
}

bool TextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!textfile->IsLoaded()) return false;
	if (this->PressCharacter(TextField::keybindings, character, modifier)) {
		return true;
	}

	if (modifier == PGModifierCtrl) {
		switch (character) {
		case 'S': {
			textfile->SaveChanges();
			return true;
		}
		case '+': {
			SetTextFontSize(textfield_font, GetTextFontSize(textfield_font) + 1);
			this->Invalidate();
			return true;
		}
		case '-': {
			SetTextFontSize(textfield_font, GetTextFontSize(textfield_font) - 1);
			this->Invalidate();
			return true;
		}
		case 'P': {
			// Search project files
			std::vector<SearchEntry> entries;
			ControlManager* cm = GetControlManager(this);
			TabControl* tb = cm->active_tabcontrol;
			for (auto it = tb->tabs.begin(); it != tb->tabs.end(); it++) {
				SearchEntry entry;
				entry.display_name = it->file->name;
				entry.text = it->file->path;
				entry.data = it->file;
				entries.push_back(entry);
			}
			SearchBox* search_box = new SearchBox(this->window, entries);
			search_box->SetSize(PGSize(this->width * 0.5f, GetTextHeight(textfield_font) + 200));
			search_box->SetPosition(PGPoint(this->x + this->width * 0.25f, this->y + 25));
			search_box->OnRender(
				[](PGRendererHandle renderer, PGFontHandle font, SearchRank& rank, SearchEntry& entry, PGScalar& x, PGScalar& y, PGScalar button_height) {
				// render the text file icon next to each open file
				TextFile* file = (TextFile*)entry.data;
				std::string filename = file->GetName();
				std::string ext = file->GetExtension();

				PGScalar file_icon_height = button_height * 0.6;
				PGScalar file_icon_width = file_icon_height * 0.8;

				PGColor color = GetTextColor(font);
				x += 2.5f;
				std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
				RenderFileIcon(renderer, font, ext.c_str(), x, y + (button_height - file_icon_height) / 2, file_icon_width, file_icon_height,
					file->GetLanguage() ? file->GetLanguage()->GetColor() : PGColor(255, 255, 255), PGColor(30, 30, 30), PGColor(91, 91, 91));
				x += file_icon_width + 2.5f;
				SetTextColor(font, color);
			});

			TextFile* active_file = textfile;
			search_box->OnSelectionChanged([](SearchBox* searchbox, SearchRank& rank, SearchEntry& entry, void* data) {
				ControlManager* cm = GetControlManager(searchbox);
				cm->active_tabcontrol->SwitchToTab((TextFile*)entry.data);
			}, (void*) this);
			search_box->OnSelectionCancelled([](SearchBox* searchbox, void* data) {
				ControlManager* cm = GetControlManager(searchbox);
				cm->active_tabcontrol->SwitchToTab((TextFile*)data);
			}, (void*)active_file);
			dynamic_cast<PGContainer*>(this->parent)->AddControl(search_box);
			return true;
		}
		case 'G': {
			// Go To Line
			SimpleTextField* field = new SimpleTextField(this->window);
			field->SetSize(PGSize(this->width * 0.5f, GetTextHeight(textfield_font) + 6));
			field->SetPosition(PGPoint(this->x + this->width * 0.25f, this->y + 25));
			struct ScrollData {
				lng offset;
				TextField* tf;
				std::vector<CursorData> backup_cursors;
			};
			ScrollData* data = new ScrollData();
			data->offset = textfile->GetLineOffset();
			data->tf = this;
			data->backup_cursors = textfile->BackupCursors();

			field->OnTextChanged([](Control* c, void* data) {
				SimpleTextField* input = (SimpleTextField*)c;
				TextField* tf = (TextField*)data;
				TextLine textline = input->GetTextFile().GetLine(0);
				std::string str = std::string(textline.GetLine(), textline.GetLength());
				const char* line = str.c_str();
				char* p = nullptr;
				// attempt to convert the text to a number
				// FIXME: strtoll (long = 32-bit on windows)
				long converted = strtol(line, &p, 10);
				errno = 0;
				if (p != line) { // if p == line, then line is empty so we do nothing
					if (*p == '\0') { // if *p == '\0' the entire string was converted
						bool valid = true;
						// bounds checking
						if (converted <= 0) {
							converted = 1;
							valid = false;
						} else if (converted > tf->GetTextFile().GetLineCount()) {
							converted = tf->GetTextFile().GetLineCount();
							valid = false;
						}
						converted--;
						// move the cursor and offset of the currently active file
						tf->GetTextFile().SetLineOffset(std::max(converted - tf->GetLineHeight() / 2, (long)0));
						tf->GetTextFile().SetCursorLocation(converted, 0);
						tf->Invalidate();
						input->SetValidInput(valid);
						input->Invalidate();
					} else {
						// invalid input, notify the user
						input->SetValidInput(false);
						input->Invalidate();
					}
				}
			}, (void*) this);
			field->OnUserCancel([](Control* c, void* data, PGModifier modifier) {
				// user pressed escape, cancelling the line
				// restore cursors and position
				ScrollData* d = (ScrollData*)data;
				d->tf->GetTextFile().RestoreCursors(d->backup_cursors);
				d->tf->GetTextFile().SetLineOffset(d->offset);
				delete d;
				dynamic_cast<PGContainer*>(c->parent)->RemoveControl(c);
			}, (void*)data);
			field->OnSuccessfulExit([](Control* c, void* data, PGModifier modifier) {
				ScrollData* d = (ScrollData*)data;
				delete d;
				dynamic_cast<PGContainer*>(c->parent)->RemoveControl(c);
			}, (void*)data);
			dynamic_cast<PGContainer*>(this->parent)->AddControl(field);
			return true;
		}
		}
	}
	if (modifier == PGModifierCtrlShift) {
		switch (character) {
		case 'W':
			// toggle word wrap
			textfile->SetWordWrap(!textfile->GetWordWrap(), GetTextfieldWidth());
			this->Invalidate();
			return true;
		default:
			break;
		}
	}
	return BasicTextField::KeyboardCharacter(character, modifier);
}

void TextField::MouseWheel(int x, int y, double distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		textfile->OffsetLineOffset(-distance);
		this->Invalidate();
	}
}

void TextField::InvalidateLine(lng line) {
	this->Invalidate();
}

void TextField::InvalidateBeforeLine(lng line) {
	this->Invalidate();
}

void TextField::InvalidateAfterLine(lng line) {
	this->Invalidate();
}

void TextField::InvalidateBetweenLines(lng start, lng end) {
	this->Invalidate();
}

void TextField::InvalidateMinimap() {
	PGScalar minimap_width = GetMinimapWidth();
	this->Invalidate(PGRect(this->width - SCROLLBAR_SIZE - minimap_width, 0, minimap_width, this->height));
}

void TextField::SetTextFile(TextFile* textfile) {
	this->textfile = textfile;
	textfile->SetTextField(this);
	this->SelectionChanged();
	this->TextChanged();
}

void TextField::OnResize(PGSize old_size, PGSize new_size) {
	if (display_minimap) {
		minimap_region.width = (int)GetMinimapWidth();
		minimap_region.height = (int)(display_horizontal_scrollbar ? new_size.height : new_size.height - SCROLLBAR_SIZE);
		minimap_region.x = (int)(new_size.width - minimap_region.width - SCROLLBAR_SIZE);
		minimap_region.y = 0;
	} else {
		minimap_region.width = 0;
		minimap_region.height = 0;
	}
	scrollbar->SetPosition(PGPoint(this->width - scrollbar->width, SCROLLBAR_PADDING));
	scrollbar->SetSize(PGSize(SCROLLBAR_SIZE, this->height - (display_horizontal_scrollbar ? SCROLLBAR_SIZE : 0) - 2 * SCROLLBAR_PADDING));
	horizontal_scrollbar->SetPosition(PGPoint(SCROLLBAR_PADDING, this->height - horizontal_scrollbar->height));
	horizontal_scrollbar->SetSize(PGSize(this->width - SCROLLBAR_SIZE - 2 * SCROLLBAR_PADDING, SCROLLBAR_SIZE));
	if (textfile->GetWordWrap()) {
		if (textfile->current_linenumber < 0) {
			textfile->current_linenumber = textfile->GetLineFromScrollPosition(this->textfield_font, textfile->wordwrap_width, textfile->GetLineOffset());
		}
	}
}

PGCursorType TextField::GetCursor(PGPoint mouse) {
	mouse.x -= this->x;
	mouse.y -= this->y;
	if (!textfile->IsLoaded()) {
		return PGCursorWait;
	}
	if (mouse.x <= this->width - minimap_region.width &&
		(!display_horizontal_scrollbar || mouse.y <= this->height - SCROLLBAR_SIZE)) {
		return PGCursorIBeam;
	}
	return PGCursorStandard;
}

void TextField::TextChanged() {
	// all text has changed (this typically happens when e.g. switching files)
	// clear the line cache entirely
	for (auto it = minimap_line_cache.begin(); it != minimap_line_cache.end(); it++) {
		DeleteImage(it->second);
	}
	minimap_line_cache.clear();
	BasicTextField::TextChanged();
}

void TextField::TextChanged(std::vector<lng> lines) {
	// a number of specific lines has changed, delete those lines from the cache
	for (auto it = lines.begin(); it != lines.end(); it++) {
		auto res = minimap_line_cache.find(*it);
		if (res != minimap_line_cache.end()) {
			DeleteImage(res->second);
			minimap_line_cache.erase(res);
		}
	}
	BasicTextField::TextChanged(lines);
}

bool TextField::IsDragging() {
	if (scrollbar->IsDragging()) return true;
	if (horizontal_scrollbar->IsDragging()) return true;
	return BasicTextField::IsDragging();
}

void TextField::GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character) {
	if (!textfile->GetWordWrap()) {
		return BasicTextField::GetLineCharacterFromPosition(x, y, line, character);
	}
	lng line_offset = std::max(std::min((lng)(y / GetTextHeight(textfield_font)), (lng)(rendered_lines.size() - 1)), (lng)0);
	line = rendered_lines[line_offset].line;
	_GetCharacterFromPosition(x, rendered_lines[line_offset].tline, character);
	character += rendered_lines[line_offset].position;

}

void TextField::GetLineFromPosition(PGScalar y, lng& line) {
	if (!textfile->GetWordWrap()) {
		return BasicTextField::GetLineFromPosition(y, line);
	}
	lng line_offset = std::max(std::min((lng)(y / GetTextHeight(textfield_font)), (lng)(rendered_lines.size() - 1)), (lng)0);
	line = rendered_lines[line_offset].line;
}

void TextField::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = TextField::keybindings_noargs;
	noargs["save"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->GetTextFile().SaveChanges();
	};
	noargs["save_as"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		std::string filename = ShowSaveFileDialog();
		if (filename.size() != 0) {
			tf->GetTextFile().SaveAs(filename);
		}
	};
	noargs["increase_font_size"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->IncreaseFontSize(1);
	};
	noargs["decrease_font_size"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->IncreaseFontSize(-1);
	};
	noargs["swap_line_up"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		assert(0);
	};
	noargs["swap_line_down"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		assert(0);
	};
	std::map<std::string, PGKeyFunctionArgs>& args = TextField::keybindings_varargs;
	args["insert"] = [](Control* c, std::map<std::string, std::string> args) {
		TextField* tf = (TextField*)c;
		if (args.count("characters") == 0) {
			return;
		}
		tf->GetTextFile().PasteText(args["characters"]);
	};
}


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

void TextField::MinimapMouseEvent(bool mouse_enter) {
	this->InvalidateMinimap();
}

void TextField::ScrollbarMouseEvent(bool mouse_enter) {
	this->InvalidateScrollbar();
}

void TextField::HScrollbarMouseEvent(bool mouse_enter) {
	this->InvalidateHScrollbar();
}

void MMMouseEvent(TextField* textfield, bool mouse_enter) {
	return textfield->MinimapMouseEvent(mouse_enter);
}

void SBMouseEvent(TextField* textfield, bool mouse_enter) {
	return textfield->ScrollbarMouseEvent(mouse_enter);
}

void HSBMouseEvent(TextField* textfield, bool mouse_enter) {
	return textfield->ScrollbarMouseEvent(mouse_enter);
}

TextField::TextField(PGWindowHandle window, TextFile* file) :
	BasicTextField(window, file), display_scrollbar(true), display_minimap(true), display_linenumbers(true) {
	textfile->SetTextField(this);

	line_height = 19;

	ControlManager* manager = (ControlManager*)GetControlManager(window);
	manager->RegisterMouseRegion(&minimap_region, this, (PGMouseCallback)MMMouseEvent);
	manager->RegisterMouseRegion(&scrollbar_region, this, (PGMouseCallback)SBMouseEvent);
	manager->RegisterMouseRegion(&arrow_regions[0], this, (PGMouseCallback)SBMouseEvent);
	manager->RegisterMouseRegion(&arrow_regions[1], this, (PGMouseCallback)SBMouseEvent);
	manager->RegisterMouseRegion(&hscrollbar_region, this, (PGMouseCallback)HSBMouseEvent);
	manager->RegisterMouseRegion(&arrow_regions[2], this, (PGMouseCallback)HSBMouseEvent);
	manager->RegisterMouseRegion(&arrow_regions[3], this, (PGMouseCallback)HSBMouseEvent);
	scrollbar_region.x = this->width - SCROLLBAR_WIDTH;
	scrollbar_region.y = SCROLLBAR_BASE_OFFSET;
	scrollbar_region.width = SCROLLBAR_WIDTH;
	arrow_regions[0].x = this->width - SCROLLBAR_WIDTH;
	arrow_regions[0].y = 0;
	arrow_regions[0].width = SCROLLBAR_BASE_OFFSET;
	arrow_regions[0].height = SCROLLBAR_BASE_OFFSET;
	arrow_regions[1].x = this->width - SCROLLBAR_WIDTH;
	arrow_regions[1].y = this->height - SCROLLBAR_WIDTH;
	arrow_regions[1].width = SCROLLBAR_BASE_OFFSET;
	arrow_regions[1].height = SCROLLBAR_BASE_OFFSET;
	hscrollbar_region.x = 0;
	hscrollbar_region.y = this->height - SCROLLBAR_WIDTH;
	hscrollbar_region.height = SCROLLBAR_WIDTH;
	arrow_regions[2].x = 0;
	arrow_regions[2].y = this->height - SCROLLBAR_WIDTH;
	arrow_regions[2].width = SCROLLBAR_BASE_OFFSET;
	arrow_regions[2].height = SCROLLBAR_BASE_OFFSET;
	arrow_regions[3].x = this->width - 2 * SCROLLBAR_BASE_OFFSET;
	arrow_regions[3].y = this->height - SCROLLBAR_WIDTH;
	arrow_regions[3].width = SCROLLBAR_BASE_OFFSET;
	arrow_regions[3].height = SCROLLBAR_BASE_OFFSET;

	textfield_font = PGCreateFont();
	minimap_font = PGCreateFont();

	SetTextFontSize(textfield_font, 15);
	SetTextFontSize(minimap_font, 2.5f);
}

TextField::~TextField() {

}

void TextField::PeriodicRender(void) {
	// FIXME: thread safety on incrementing display_carets_count and cursors?
	if (drag_type == PGDragHoldScrollArrow) {
		time_t time = GetTime();
		if (time - drag_start >= DOUBLE_CLICK_TIME) {
			switch (drag_region) {
			case PGDragRegionScrollbarArrowUp:
				textfile->OffsetLineOffset(-1);
				break;
			case PGDragRegionScrollbarArrowDown:
				textfile->OffsetLineOffset(1);
				break;
			case PGDragRegionScrollbarArrowLeft:
				textfile->SetXOffset(std::max((PGScalar)0, textfile->GetXOffset() - 10));
				break;
			case PGDragRegionScrollbarArrowRight:
				textfile->SetXOffset(std::min((PGScalar)max_xoffset, textfile->GetXOffset() + 10));
				break;
			case PGDragRegionAboveScrollbar:
				textfile->OffsetLineOffset(-GetLineHeight());
				break;
			case PGDragRegionBelowScrollbar:
				textfile->OffsetLineOffset(GetLineHeight());
				break;
			}
			this->Invalidate();
		}
	}

	BasicTextField::PeriodicRender();
}

void TextField::DrawTextField(PGRendererHandle renderer, PGFontHandle font, PGIRect* rectangle, bool minimap, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay) {
	PGScalar xoffset = 0;
	PGScalar max_x = position_x_text + width;
	if (!minimap)
		xoffset = textfile->GetXOffset();
	PGScalar y = Y();
	lng start_line = textfile->GetLineOffset();
	std::vector<Cursor*> cursors = textfile->GetCursors();
	lng linenr = start_line;
	TextLine *current_line;
	PGColor selection_color = PGStyleManager::GetColor(PGColorTextFieldSelection);
	PGScalar line_height = GetTextHeight(font);
	PGScalar initial_position_y = position_y;
	PGScalar start_position_y = position_y;
	if (minimap) {
		// fill in the background of the minimap
		PGRect rect(position_x_text, position_y, this->width - position_x_text, this->height - position_y);
		RenderRectangle(renderer, rect, PGColor(30, 30, 30), PGDrawStyleFill);
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
	linenr = start_line + rectangle->height / line_height;
	// render the selection and carets
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		lng startline = std::max((*it)->BeginLine(), start_line);
		lng endline = std::min((*it)->EndLine(), linenr);
		position_y = y + (startline - start_line) * line_height - rectangle->y;
		for (; startline <= endline; startline++) {
			current_line = textfile->GetLine(startline);
			assert(current_line);
			lng start, end;
			if (startline == (*it)->BeginLine()) {
				if (startline == (*it)->EndLine()) {
					// start and end are on the same line
					start = (*it)->BeginPosition();
					end = (*it)->EndPosition();
				} else {
					start = (*it)->BeginPosition();
					end = current_line->GetLength() + 1;
				}
			} else if (startline == (*it)->EndLine()) {
				start = 0;
				end = (*it)->EndPosition();
			} else {
				start = 0;
				end = current_line->GetLength() + 1;
			}

			RenderSelection(renderer,
				font,
				current_line->GetLine(),
				current_line->GetLength(),
				position_x_text - xoffset,
				position_y,
				start,
				end,
				selection_color,
				max_x);

			if (!minimap && startline == (*it)->SelectedLine()) {
				if (display_carets) {
					// render the caret on the selected line
					RenderCaret(renderer, font, current_line->GetLine(), current_line->GetLength(), position_x_text - xoffset, position_y, (*it)->SelectedPosition(), line_height, PGStyleManager::GetColor(PGColorTextFieldCaret));
				}
			}
			position_y += line_height;
		}
	}
	linenr = start_line;
	position_y = initial_position_y;
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
			PGScalar xpos = position_x_text - xoffset;
			if (parsed) {
				PGSyntax* syntax = &current_line->syntax;
				while (syntax && syntax->end > 0) {
					bool squiggles = false;
					//assert(syntax->end > position);
					if (syntax->end <= position) {
						syntax = syntax->next;
						continue;
					}
					// FIXME
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
					RenderText(renderer, font, line + position, syntax->end - position, xpos, position_y, max_x);
					PGScalar text_width = MeasureTextWidth(font, line + position, syntax->end - position);
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
			SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
			RenderText(renderer, font, line + position, length - position, xpos, position_y, max_x);
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
								PGScalar x_offset = MeasureTextWidth(font, line, i);
								PGScalar width = MeasureTextWidth(font, selected_word, word_end - word_start);
								PGRect rect(position_x_text + x_offset - xoffset, position_y, width, line_height);
								RenderRectangle(renderer, rect, PGStyleManager::GetColor(PGColorTextFieldText), PGDrawStyleStroke);
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
	if (!minimap) {
		this->line_height = line_height;
	} else {
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
	if (selected_word) free(selected_word);
}

void TextField::Draw(PGRendererHandle renderer, PGIRect* r) {
	bool window_has_focus = WindowHasFocus(window);
	PGIRect rect = PGIRect(r->x, r->y, std::min(r->width, (int)(X() + this->width - r->x)), std::min(r->height, (int)(Y() + this->height - r->y)));
	PGIRect* rectangle = &rect;

	// determine the width of the line numbers
	std::vector<Cursor*> cursors = textfile->GetCursors();
	lng line_count = textfile->GetLineCount();
	text_offset = 0;
	if (this->display_linenumbers) {
		auto line_number = std::to_string(std::max((lng)10, textfile->GetLineCount() + 1));
		text_offset = 10 + MeasureTextWidth(textfield_font, line_number.c_str(), line_number.size());
	}
	PGPoint position = Position();
	PGScalar x = position.x, y = position.y;
	// get the mouse position (for rendering hovers)
	PGPoint mouse = GetMousePosition(window, this);
	PGScalar position_x = x - rectangle->x;
	PGScalar position_y = y - rectangle->y;
	// textfield/minimap dimensions
	PGScalar minimap_width = this->display_minimap ? GetMinimapWidth() : 0;
	PGScalar textfield_width = this->width - minimap_width;
	// determine x-offset and clamp it
	PGScalar max_character_width = MeasureTextWidth(textfield_font, "W", 1);
	max_textsize = textfile->GetMaxLineWidth() * max_character_width;
	max_xoffset = std::max(max_textsize - textfield_width + text_offset, 0.0f);
	PGScalar xoffset = this->textfile->GetXOffset();
	if (xoffset > max_xoffset) {
		xoffset = max_xoffset;
		this->textfile->SetXOffset(max_xoffset);
	}
	// render the actual text field
	if (textfile->IsLoaded()) {
		DrawTextField(renderer, textfield_font, rectangle, false, x + text_offset - rectangle->x, y - rectangle->y, textfield_width, false);
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
		bool mouse_in_minimap = window_has_focus && mouse.x >= this->width - SCROLLBAR_WIDTH - minimap_width && mouse.x <= this->width - SCROLLBAR_WIDTH;
		PGIRect minimap_rect = PGIRect(x + textfield_width, y, minimap_width, this->height);

		DrawTextField(renderer, minimap_font, &minimap_rect, true, x + textfield_width - rectangle->x, y - rectangle->y, minimap_width, mouse_in_minimap);
	}

	// render the line numbers
	lng linenr = textfile->GetLineOffset();
	line_height = GetTextHeight(textfield_font);
	if (this->display_linenumbers) {
		// fill in the background of the line numbers
		position_y = y - rectangle->y;
		RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);

		SetTextColor(textfield_font, PGStyleManager::GetColor(PGColorTextFieldLineNumber));

		TextLine *current_line;
		while ((current_line = textfile->GetLine(linenr)) != nullptr) {
			// only render lines that fall within the render rectangle
			if (position_y > rectangle->height) break;
			if (position_y + line_height >= 0) {
				// if the line is selected by a cursor, render an overlay
				for (auto it = cursors.begin(); it != cursors.end(); it++) {
					if (linenr == (*it)->SelectedLine()) {
						RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, line_height), PGStyleManager::GetColor(PGColorTextFieldSelection), PGDrawStyleFill);
						break;
					}
				}

				// render the line number
				auto line_number = std::to_string(linenr + 1);
				RenderText(renderer, textfield_font, line_number.c_str(), line_number.size(), position_x, position_y);
			}
			linenr++;
			position_y += line_height;
		}
	}
	// render the scrollbar
	if (this->display_scrollbar) {
		bool mouse_in_scrollbar = window_has_focus && mouse.x >= this->width - SCROLLBAR_WIDTH && mouse.x <= this->width;
		x = x - rectangle->x;
		y = y - rectangle->y;

		// the background of the scrollbar
		RenderRectangle(renderer, PGRect(x + this->width - SCROLLBAR_WIDTH, y, SCROLLBAR_WIDTH, this->height), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);
		// the arrows above/below the scrollbar
		PGColor arrowColor = PGStyleManager::GetColor(PGColorScrollbarForeground);
		if (mouse_in_scrollbar && mouse.y >= 0 && mouse.y <= 16)
			arrowColor = PGStyleManager::GetColor(PGColorScrollbarHover);
		RenderTriangle(renderer, PGPoint(x + this->width - 8, y + 4), PGPoint(x + this->width - 13, y + 12), PGPoint(x + this->width - 3, y + 12), arrowColor, PGDrawStyleFill);
		arrowColor = PGStyleManager::GetColor(PGColorScrollbarForeground);
		if (mouse_in_scrollbar && mouse.y >= this->height - 16 && mouse.y <= this->height)
			arrowColor = PGStyleManager::GetColor(PGColorScrollbarHover);
		RenderTriangle(renderer, PGPoint(x + this->width - 8, y + this->height - 4), PGPoint(x + this->width - 13, y + this->height - 12), PGPoint(x + this->width - 3, y + this->height - 12), arrowColor, PGDrawStyleFill);
		// the actual scrollbar
		PGScalar scrollbar_height = GetScrollbarHeight();
		PGScalar scrollbar_offset = GetScrollbarOffset();
		scrollbar_region.height = scrollbar_height;
		scrollbar_region.y = y + scrollbar_offset;
		PGColor scrollbar_color = PGStyleManager::GetColor(PGColorScrollbarForeground);
		if (this->drag_type == PGDragScrollbar)
			scrollbar_color = PGStyleManager::GetColor(PGColorScrollbarDrag);
		else if (mouse_in_scrollbar && mouse.y >= scrollbar_offset && mouse.y <= scrollbar_offset + scrollbar_height)
			scrollbar_color = PGStyleManager::GetColor(PGColorScrollbarHover);
		RenderRectangle(renderer, PGRect(x + this->width - 12, y + scrollbar_offset, 8, scrollbar_height), scrollbar_color, PGDrawStyleFill);

		// horizontal scrollbar
		display_horizontal_scrollbar = max_xoffset > 0;
		if (display_horizontal_scrollbar) {
			bool mouse_in_horizontal_scrollbar = window_has_focus && mouse.y >= this->height - SCROLLBAR_WIDTH && mouse.y <= this->height && mouse.x <= this->width - SCROLLBAR_WIDTH;
			// background of the scrollbar
			RenderRectangle(renderer, PGRect(x, y + this->height - SCROLLBAR_WIDTH, this->width - SCROLLBAR_WIDTH, SCROLLBAR_WIDTH), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);
			// the arrows
			PGColor arrowColor = PGStyleManager::GetColor(PGColorScrollbarForeground);
			if (mouse_in_horizontal_scrollbar && mouse.x >= 0 && mouse.x <= SCROLLBAR_BASE_OFFSET)
				arrowColor = PGStyleManager::GetColor(PGColorScrollbarHover);
			RenderTriangle(renderer, PGPoint(x + 4, y + this->height - 8), PGPoint(x + 12, y + this->height - 12), PGPoint(x + 12, y + this->height - 4), arrowColor, PGDrawStyleFill);
			arrowColor = PGStyleManager::GetColor(PGColorScrollbarForeground);
			if (mouse_in_horizontal_scrollbar && mouse.x >= this->width - SCROLLBAR_BASE_OFFSET * 2 && mouse.x <= this->width - SCROLLBAR_BASE_OFFSET)
				arrowColor = PGStyleManager::GetColor(PGColorScrollbarHover);
			RenderTriangle(renderer, PGPoint(x + this->width - 4 - SCROLLBAR_BASE_OFFSET, y + this->height - 8), PGPoint(x + this->width - 12 - SCROLLBAR_BASE_OFFSET, y + this->height - 12), PGPoint(x + this->width - 12 - SCROLLBAR_BASE_OFFSET, y + this->height - 4), arrowColor, PGDrawStyleFill);
			// render the actual scrollbar
			scrollbar_height = GetHScrollbarHeight();
			scrollbar_offset = GetHScrollbarOffset();
			hscrollbar_region.width = scrollbar_height;
			hscrollbar_region.x = x + scrollbar_offset;
			scrollbar_color = PGStyleManager::GetColor(PGColorScrollbarForeground);
			if (this->drag_type == PGDragHorizontalScrollbar)
				scrollbar_color = PGStyleManager::GetColor(PGColorScrollbarDrag);
			else if (mouse_in_horizontal_scrollbar && mouse.x >= scrollbar_offset && mouse.x <= scrollbar_offset + scrollbar_height)
				scrollbar_color = PGStyleManager::GetColor(PGColorScrollbarHover);
			RenderRectangle(renderer, PGRect(x + scrollbar_offset, y + this->height - 12, scrollbar_height, 8), scrollbar_color, PGDrawStyleFill);
		}
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

PGScalar TextField::GetHScrollbarHeight() {
	PGScalar width_percentage = (this->width - minimap_region.width - SCROLLBAR_WIDTH) / (max_textsize);
	return std::max((PGScalar)16, (this->width - SCROLLBAR_BASE_OFFSET * 2 - SCROLLBAR_WIDTH) * width_percentage);
}

PGScalar TextField::GetHScrollbarOffset() {
	PGScalar percentage = textfile->GetXOffset() / max_xoffset;
	return SCROLLBAR_BASE_OFFSET + (this->width - GetHScrollbarHeight() - SCROLLBAR_BASE_OFFSET * 2 - SCROLLBAR_WIDTH) * percentage;
}

void TextField::SetHScrollbarOffset(PGScalar offset) {
	PGScalar scrollbar_width = (this->width - GetHScrollbarHeight() - SCROLLBAR_BASE_OFFSET * 2 - SCROLLBAR_WIDTH);
	// offset = SCROLLBAR_BASE_OFFSET + scrollbar_width * percentage;
	// offset - SCROLLBAR_BASE_OFFSET =  * (textfile->GetXOffset() / max_xoffset);
	// textfile->GetXOffset() = ((offset - SCROLLBAR_BASE_OFFSET) / scrollbar_width) * max_xoffset
	PGScalar xoffset = ((offset - SCROLLBAR_BASE_OFFSET) / scrollbar_width) * max_xoffset;
	xoffset = std::min(max_xoffset, std::max(0.0f, xoffset));
	textfile->SetXOffset(xoffset);
	hscrollbar_region.x = X() + GetHScrollbarOffset();
}

PGScalar TextField::GetTextfieldWidth() {
	return display_minimap ? this->width - SCROLLBAR_WIDTH - GetMinimapWidth() : this->width - SCROLLBAR_WIDTH;
}

PGScalar TextField::GetTextfieldHeight() {
	return display_horizontal_scrollbar ? this->height - SCROLLBAR_WIDTH : this->height;
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
	lng lines_rendered = this->height / minimap_line_height;
	lng start_line = std::max((lng)(((lng)((std::max((lng)1, this->textfile->GetLineCount() - 1) * percentage))) - (lines_rendered * percentage)), (lng)0);
	lng lineoffset_y = start_line + (lng)(lines_rendered * percentage);
	lineoffset_y = std::max((lng)0, std::min(lineoffset_y, this->textfile->GetLineCount() - 1));
	textfile->SetLineOffset(lineoffset_y);
}

void TextField::SetScrollbarOffset(PGScalar offset) {
	// compute lineoffset_y from scrollbar offset
	lng lineoffset_y = (lng)((std::max((lng)1, this->textfile->GetLineCount() - 1) * (offset - SCROLLBAR_BASE_OFFSET)) / (this->height - GetScrollbarHeight() - 2 * SCROLLBAR_BASE_OFFSET));
	lineoffset_y = std::max((lng)0, std::min(lineoffset_y, this->textfile->GetLineCount() - 1));
	textfile->SetLineOffset(lineoffset_y);
	scrollbar_region.y = Y() + GetScrollbarOffset();
}

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (button == PGLeftMouseButton) {
		if (this->display_scrollbar && mouse.x > this->width - SCROLLBAR_WIDTH) {
			// scrollbar
			if (mouse.y <= SCROLLBAR_BASE_OFFSET) {
				drag_start = GetTime();
				drag_region = PGDragRegionScrollbarArrowUp;
				drag_type = PGDragHoldScrollArrow;
				textfile->OffsetLineOffset(-1);
				this->Invalidate();
			} else if (mouse.y >= this->height - SCROLLBAR_BASE_OFFSET) {
				drag_start = GetTime();
				drag_region = PGDragRegionScrollbarArrowDown;
				drag_type = PGDragHoldScrollArrow;
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
					drag_start = GetTime();
					drag_region = PGDragRegionAboveScrollbar;
					drag_type = PGDragHoldScrollArrow;
					textfile->OffsetLineOffset(-GetLineHeight());
					this->Invalidate();
				} else {
					// mouse click below the scrollbar
					drag_start = GetTime();
					drag_region = PGDragRegionBelowScrollbar;
					drag_type = PGDragHoldScrollArrow;
					textfile->OffsetLineOffset(GetLineHeight());
					this->Invalidate();
				}
			}
			return;
		} else if (this->display_horizontal_scrollbar && mouse.y >= this->height - SCROLLBAR_WIDTH) {
			if (mouse.x <= SCROLLBAR_BASE_OFFSET) {
				drag_start = GetTime();
				drag_region = PGDragRegionScrollbarArrowLeft;
				drag_type = PGDragHoldScrollArrow;
				textfile->SetXOffset(std::max((PGScalar)0, textfile->GetXOffset() - 10));
				this->Invalidate();
			} else if (mouse.x >= this->width - SCROLLBAR_BASE_OFFSET - SCROLLBAR_WIDTH) {
				drag_start = GetTime();
				drag_region = PGDragRegionScrollbarArrowRight;
				drag_type = PGDragHoldScrollArrow;
				textfile->SetXOffset(std::min((PGScalar)max_xoffset, textfile->GetXOffset() + 10));
				this->Invalidate();
			} else {
				PGScalar scrollbar_offset = GetHScrollbarOffset();
				PGScalar scrollbar_width = GetHScrollbarHeight();

				if (mouse.x >= scrollbar_offset && mouse.x <= scrollbar_offset + scrollbar_width) {
					// mouse is on the scrollbar; enable dragging of the scrollbar
					drag_type = PGDragHorizontalScrollbar;
					drag_offset = mouse.x - scrollbar_offset;
					this->Invalidate();
				} else {
					// mouse click on the bar around the scrollbar
					drag_type = PGDragHorizontalScrollbar;
					this->SetHScrollbarOffset(mouse.x - scrollbar_width / 2.0f);
					drag_offset = mouse.x - GetHScrollbarOffset();
					this->Invalidate();
				}
			}
			return;
		}
		if (this->display_minimap) {
			lng minimap_width = (lng)GetMinimapWidth();
			lng minimap_position = this->width - (this->display_scrollbar ? SCROLLBAR_WIDTH : 0) - minimap_width;
			if (mouse.x > minimap_position && mouse.x <= minimap_position + minimap_width && (
				!display_horizontal_scrollbar || mouse.y <= this->height - SCROLLBAR_WIDTH)) {
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

void TextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (button & PGLeftMouseButton) {
		if (drag_type != PGDragSelectionCursors) {
			drag_type = PGDragNone;
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
			dynamic_cast<TextField*>(control)->textfile->PasteText(GetClipboardText(control->window));
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
				Cursor::NormalizeCursors(textfile, textfile->GetCursors());
			}
		} else if (drag_type == PGDragScrollbar) {
			lng current_offset = textfile->GetLineOffset();
			SetScrollbarOffset(mouse.y - drag_offset);
			if (current_offset != textfile->GetLineOffset())
				this->Invalidate();
		} else if (drag_type == PGDragHorizontalScrollbar) {
			PGScalar current_offset = textfile->GetXOffset();
			SetHScrollbarOffset(mouse.x - drag_offset);
			if (current_offset != textfile->GetXOffset())
				this->Invalidate();
		} else if (drag_type == PGDragMinimap) {
			lng current_offset = textfile->GetLineOffset();
			SetMinimapOffset(mouse.y - drag_offset);
			if (current_offset != textfile->GetLineOffset())
				this->Invalidate();
		}
	} else if (buttons & PGMiddleMouseButton) {
		if (drag_type == PGDragSelectionCursors) {
			lng line;
			GetLineFromPosition(mouse.y, line);
			std::vector<Cursor*>& cursors = textfile->GetCursors();
			Cursor* active_cursor = textfile->GetActiveCursor();
			lng start_line = active_cursor->end_line;
			lng increment = line > active_cursor->end_line ? 1 : -1;
			cursors[0] = active_cursor;
			textfile->ClearExtraCursors();
			for (auto it = active_cursor->end_line; ; it += increment) {
				lng start_character, end_character;
				TextLine* current_line = textfile->GetLine(it);
				GetCharacterFromPosition(drag_offset, current_line, start_character);
				GetCharacterFromPosition(mouse.x, current_line, end_character);
				if (start_character != end_character) {
					Cursor* cursor = new Cursor(textfile, it, start_character);
					cursor->start_character = end_character;
					cursors.push_back(cursor);
				}
				if (it == line) break;
			}
			this->InvalidateBetweenLines(cursors[0]->start_line, line);
		}
	} else {
		drag_type = PGDragNone;
		if (this->display_scrollbar && mouse.x >= this->width - SCROLLBAR_WIDTH && mouse.x <= this->width) {
			this->InvalidateScrollbar();
		} else if (this->display_minimap && mouse.x >= this->width - SCROLLBAR_WIDTH - GetMinimapWidth() && mouse.x <= this->width - SCROLLBAR_WIDTH) {
			this->InvalidateMinimap();
		} else if (this->display_scrollbar && this->display_horizontal_scrollbar && mouse.y >= this->height - SCROLLBAR_WIDTH) {
			this->InvalidateHScrollbar();
		}
	}
}

bool TextField::KeyboardButton(PGButton button, PGModifier modifier) {
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
	}
	return BasicTextField::KeyboardButton(button, modifier);
}

bool TextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!textfile->IsLoaded()) return false;

	if (modifier & PGModifierCtrl) {
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
		case 'G': {
			// Go To Line
			SimpleTextField* field = new SimpleTextField(this->window);
			field->SetSize(PGSize(this->width * 0.5f, GetTextHeight(textfield_font) + 6));
			field->SetPosition(PGPoint(this->x + this->width * 0.25f, this->y + 25));
			dynamic_cast<PGContainer*>(this->parent)->AddControl(field);
			return true;
		}
		}
	}
	return BasicTextField::KeyboardCharacter(character, modifier);
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
	this->Invalidate(PGRect(X(), Y() + (start - lineoffset_y) * line_height, this->width,
		(end - lineoffset_y) * line_height - (start - lineoffset_y) * line_height + line_height));
}

void TextField::InvalidateScrollbar() {
	this->Invalidate(PGRect(this->width - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, this->height));
}

void TextField::InvalidateHScrollbar() {
	this->Invalidate(PGRect(0, this->height - SCROLLBAR_WIDTH, this->width, SCROLLBAR_WIDTH));
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
	if (display_minimap) {
		minimap_region.width = GetMinimapWidth();
		minimap_region.height = new_size.height;
		minimap_region.x = new_size.width - minimap_region.width - SCROLLBAR_WIDTH;
		minimap_region.y = 0;
	} else {
		minimap_region.width = 0;
		minimap_region.height = 0;
	}
	if (display_scrollbar) {
		scrollbar_region.x = new_size.width - SCROLLBAR_WIDTH;
		hscrollbar_region.y = new_size.height - SCROLLBAR_WIDTH;

		arrow_regions[0].x = this->width - SCROLLBAR_WIDTH;
		arrow_regions[0].y = 0;
		arrow_regions[0].width = SCROLLBAR_BASE_OFFSET;
		arrow_regions[0].height = SCROLLBAR_BASE_OFFSET;
		arrow_regions[1].x = this->width - SCROLLBAR_WIDTH;
		arrow_regions[1].y = this->height - SCROLLBAR_WIDTH;
		arrow_regions[1].width = SCROLLBAR_BASE_OFFSET;
		arrow_regions[1].height = SCROLLBAR_BASE_OFFSET;
		arrow_regions[2].x = 0;
		arrow_regions[2].y = this->height - SCROLLBAR_WIDTH;
		arrow_regions[2].width = SCROLLBAR_BASE_OFFSET;
		arrow_regions[2].height = SCROLLBAR_BASE_OFFSET;
		arrow_regions[3].x = this->width - 2 * SCROLLBAR_BASE_OFFSET;
		arrow_regions[3].y = this->height - SCROLLBAR_WIDTH;
		arrow_regions[3].width = SCROLLBAR_BASE_OFFSET;
		arrow_regions[3].height = SCROLLBAR_BASE_OFFSET;
	}
}

PGCursorType TextField::GetCursor(PGPoint mouse) {
	mouse.x -= this->x;
	mouse.y -= this->y;
	if (!textfile->IsLoaded()) {
		return PGCursorWait;
	}
	if (mouse.x <= this->width - minimap_region.width &&
		mouse.y <= this->height - SCROLLBAR_WIDTH) {
		return PGCursorIBeam;
	}
	return PGCursorStandard;
}

void TextField::SelectionChanged() {
	if (statusbar) {
		statusbar->Invalidate();
	}
	BasicTextField::SelectionChanged();
}

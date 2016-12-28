
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

#define SCROLLBAR_PADDING 4

void TextField::MinimapMouseEvent(bool mouse_enter) {
	this->mouse_in_minimap = mouse_enter;
	this->InvalidateMinimap();
}

TextField::TextField(PGWindowHandle window, TextFile* file) :
	BasicTextField(window, file), display_scrollbar(true), display_minimap(true), display_linenumbers(true) {
	textfile->SetTextField(this);

	line_height = 19;

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

	SetTextFontSize(textfield_font, 15);
	SetTextFontSize(minimap_font, 2.5f);
}

TextField::~TextField() {

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

	// render search matches
	textfile->Lock(PGReadLock);
	if (!minimap) {
		auto matches = textfile->GetFindMatches();
		position_y = initial_position_y;
		for (auto it = matches.begin(); it != matches.end(); it++) {
			if (it->start_line > linenr || it->end_line < start_line) continue;
			lng startline = std::max(it->start_line, start_line);
			lng endline = std::min(it->end_line, linenr);
			position_y = y + (startline - start_line) * line_height - rectangle->y;
			for (; startline <= endline; startline++) {
				current_line = textfile->GetLine(startline);
				char* line = current_line->GetLine();
				lng length = current_line->GetLength();
				lng start, end;
				if (startline == it->start_line) {
					if (startline == it->end_line) {
						// start and end are on the same line
						start = it->start_character;
						end = it->end_character;
					} else {
						start = it->start_character;
						end = length;
					}
				} else if (startline == it->end_line) {
					start = 0;
					end = it->end_character;
				} else {
					start = 0;
					end = length;
				}

				PGScalar x_offset = MeasureTextWidth(font, line, start);
				PGScalar width = MeasureTextWidth(font, line + start, end - start);
				PGRect rect(position_x_text + x_offset - xoffset, position_y, width, line_height);
				RenderRectangle(renderer, rect, PGStyleManager::GetColor(PGColorTextFieldText), PGDrawStyleStroke);
			}
		}
	}

	linenr = start_line;
	position_y = initial_position_y;
	lng block = -1;
	bool parsed = false;

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

			// because RenderText is expensive, we cache rendered text lines for the minimap
			// this is because the minimap renders far more lines than the textfield (generally 10x more)
			// we cache by simply rendering the textline to a bitmap, and then rendering the bitmap
			// to the screen
			PGBitmapHandle line_bitmap = nullptr;
			PGRendererHandle line_renderer = renderer;
			bool render_text = true;
			if (minimap) {
				// first check if the line is found in the cache, if it is not, we rerender
				render_text = !minimap_line_cache.count(current_line);
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
				if (parsed) {
					PGSyntax* syntax = &current_line->syntax;
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
				}
				SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
				RenderText(line_renderer, font, line + position, length - position, bitmap_x, bitmap_y);
				if (minimap) {
					// we rendered into a bitmap: delete the renderer and store the line
					DeleteRenderer(line_renderer);
					minimap_line_cache[current_line] = line_bitmap;
				}
			} else {
				// if the line is already cached, simply retrieve the bitmap
				line_bitmap = minimap_line_cache[current_line];
			}
			if (minimap) {
				// render the cached bitmap to the screen
				RenderImage(renderer, line_bitmap, xpos, position_y);
			}
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
	textfile->Unlock(PGReadLock);
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
	PGScalar max_textsize = textfile->GetMaxLineWidth() * max_character_width;
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
		bool mouse_in_minimap = window_has_focus && this->mouse_in_minimap;
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
		scrollbar->UpdateValues(0, textfile->GetLineCount() - 1, GetLineHeight(), textfile->GetLineOffset());
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
	return display_minimap ? this->width - SCROLLBAR_SIZE - GetMinimapWidth() : this->width - SCROLLBAR_SIZE;
}

PGScalar TextField::GetTextfieldHeight() {
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

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (PGRectangleContains(scrollbar->GetRectangle(), mouse)) {
		scrollbar->UpdateValues(0, textfile->GetLineCount() - 1, GetLineHeight(), textfile->GetLineOffset());
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
			textfile->GetActiveCursor()->SelectWord();
		} else if (last_click.clicks == 2) {
			textfile->SetCursorLocation(line, character);
			textfile->GetActiveCursor()->SelectLine();
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
	if (PGRectangleContains(scrollbar->GetRectangle(), mouse)) {
		scrollbar->UpdateValues(0, textfile->GetLineCount() - 1, GetLineHeight(), textfile->GetLineOffset());
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
	if (scrollbar->IsDragging()) {
		scrollbar->UpdateValues(0, textfile->GetLineCount() - 1, GetLineHeight(), textfile->GetLineOffset());
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
			if (active_cursor->start_line != line || active_cursor->start_character != character) {
				lng old_line = active_cursor->start_line;
				active_cursor->SetCursorStartLocation(line, character);
				Logger::GetInstance()->WriteLogMessage(std::string("if (!active_cursor) active_cursor = cursors.front();\nactive_cursor->SetCursorStartLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\nCursor::NormalizeCursors(textfield, cursors, false);"));
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
		if (this->display_minimap && mouse.x >= this->width - SCROLLBAR_SIZE - GetMinimapWidth() && mouse.x <= this->width - SCROLLBAR_SIZE) {
			this->InvalidateMinimap();
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
			struct ScrollData {
				lng offset;
				TextField* tf;
				std::vector<Cursor> backup_cursors;
			};
			ScrollData* data = new ScrollData();
			data->offset = textfile->GetLineOffset();
			data->tf = this;
			data->backup_cursors = textfile->BackupCursors();

			field->OnTextChanged([](Control* c, void* data) {
				SimpleTextField* input = (SimpleTextField*)c;
				TextField* tf = (TextField*)data;
				char* line = input->GetTextFile().GetLine(0)->GetLine();
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
		minimap_region.width = GetMinimapWidth();
		minimap_region.height = display_horizontal_scrollbar ? new_size.height : new_size.height - SCROLLBAR_SIZE;
		minimap_region.x = new_size.width - minimap_region.width - SCROLLBAR_SIZE;
		minimap_region.y = 0;
	} else {
		minimap_region.width = 0;
		minimap_region.height = 0;
	}
	scrollbar->SetPosition(PGPoint(this->width - scrollbar->width, SCROLLBAR_PADDING));
	scrollbar->SetSize(PGSize(SCROLLBAR_SIZE, this->height - (display_horizontal_scrollbar ? SCROLLBAR_SIZE : 0) - 2 * SCROLLBAR_PADDING));
	horizontal_scrollbar->SetPosition(PGPoint(SCROLLBAR_PADDING, this->height - horizontal_scrollbar->height));
	horizontal_scrollbar->SetSize(PGSize(this->width - SCROLLBAR_SIZE - 2 * SCROLLBAR_PADDING, SCROLLBAR_SIZE));
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

void TextField::TextChanged(std::vector<TextLine*> lines) {
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

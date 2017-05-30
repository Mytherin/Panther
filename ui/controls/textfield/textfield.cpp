
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
#include "tabcontrol.h"

#include "goto.h"
#include "notification.h"
#include "searchbox.h"
#include "settings.h"

#include "textiterator.h"
#include "wrappedtextiterator.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(TextField);


void TextField::MinimapMouseEvent(bool mouse_enter) {
	this->mouse_in_minimap = mouse_enter;
	this->InvalidateMinimap();
}

TextField::TextField(PGWindowHandle window, std::shared_ptr<TextView> view) :
	BasicTextField(window, view), display_scrollbar(true), display_minimap(true),
	display_linenumbers(true), notification(nullptr), tabcontrol(nullptr),
	active_searchbox(nullptr),
	is_minimap_dirty(true) {
	if (view) {
		view->SetTextField(this);
	}

	this->support_multiple_lines = true;

	ControlManager* manager = GetControlManager(this);
	manager->RegisterMouseRegion(&minimap_region, this, [](Control* tf, bool mouse_enter, void* data) {
		return ((TextField*)tf)->MinimapMouseEvent(mouse_enter);
	});

   	textfield_font = PGCreateFont(PGFontTypeTextField);
	minimap_font = PGCreateFont(PGFontTypeTextField);

	int size = 0;
	if (PGSettingsManager::GetSetting("font_size", size)) {
		SetTextFontSize(textfield_font, size);
	}
	PGSettingsManager::GetSetting("display_line_numbers", display_linenumbers);
	PGSettingsManager::GetSetting("display_minimap", display_minimap);
	SetTextFontSize(minimap_font, 2.5f);
	margin_width = 4;
}

TextField::~TextField() {
	ControlManager* manager = GetControlManager(this);
	if (manager) {
		manager->UnregisterMouseRegion(&minimap_region);	
	}
}

void TextField::Initialize() {
	scrollbar = std::unique_ptr<DecoratedScrollbar>(new DecoratedScrollbar(this, window, false, false));
	scrollbar->SetPosition(PGPoint(this->width - SCROLLBAR_SIZE, 0));
	scrollbar->padding.bottom = SCROLLBAR_PADDING;
	scrollbar->padding.top = SCROLLBAR_PADDING;
	scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((TextField*)scroll->parent.lock().get())->view->SetScrollOffset(value);
	});
	horizontal_scrollbar = std::unique_ptr<Scrollbar>(new Scrollbar(this, window, true, false));
	horizontal_scrollbar->padding.right = SCROLLBAR_PADDING;
	horizontal_scrollbar->padding.left = SCROLLBAR_PADDING;
	horizontal_scrollbar->SetPosition(PGPoint(0, this->height - SCROLLBAR_SIZE));
	horizontal_scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((TextField*)scroll->parent.lock().get())->view->SetXOffset(value);
	});

}

void TextField::Update() {
	if (view && !view->file->IsLoaded() && view->file->bytes < 0) {
		// error loading file
		if (!notification) {
			CreateNotification(PGNotificationTypeError, std::string("Error loading file."));
			assert(notification);
			notification->AddButton([](Control* control, void* data) {
				// FIXME: close tab
				((TextField*)control)->ClearNotification();
			}, this, notification, "Close");
			ShowNotification();
		}
	}
	BasicTextField::Update();
}

void TextField::DrawTextField(PGRendererHandle renderer, PGFontHandle font, bool minimap, PGScalar position_x, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay) {
	PGScalar xoffset = 0;
	PGScalar max_x = position_x_text + width;
	if (!minimap)
		xoffset = view->GetXOffset();
	PGScalar y = Y();
	auto start_line = view->GetLineOffset();
	const std::vector<Cursor>& cursors = view->GetCursors();
	TextLine current_line;
	PGColor selection_color = PGStyleManager::GetColor(PGColorTextFieldSelection);
	PGScalar line_height = GetTextHeight(font);
	PGScalar initial_position_y = position_y;
	PGScalar render_width = GetTextfieldWidth();
	if (minimap) {
		// start line of the minimap
		start_line = GetMinimapStartLine();
	}
	// set tab width
	SetTextTabWidth(font, view->file->GetTabWidth());

	std::string selected_word = std::string();
	if (!minimap) {
		selected_word = cursors[0].GetSelectedWord();
	}
	PGScalar text_offset = position_x_text - position_x - margin_width * 2;
	if (!minimap && display_linenumbers) {
		// clear the linenumber region
		RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);
		if (xoffset > 0) {
			RenderGradient(renderer, PGRect(position_x + text_offset, position_y, margin_width, this->height), PGColor(0, 0, 0, 128), PGColor(0, 0, 0, 0));
		}
	}

	lng block = -1;
	bool parsed = false;

	TextLineIterator* line_iterator = view->GetScrollIterator(this, start_line);
	auto buffer = line_iterator->CurrentBuffer();
	lng current_cursor = 0;
	lng current_match = 0;
	const std::vector<PGTextRange>& matches = view->GetFindMatches();
	lng selected_line = -1;
	assert(start_line.line_fraction >= 0 && start_line.line_fraction <= 1);
	initial_position_y -= line_height * start_line.line_fraction;
	position_y = initial_position_y;

	if (!minimap) {
		rendered_lines.clear();
	}
	std::vector<std::vector<PGScalar>> cumulative_character_widths;
	std::vector<std::pair<lng, lng>> render_start_end;

	while ((current_line = line_iterator->GetLine()).IsValid()) {
		if (position_y > initial_position_y + this->height) break;

		// only render lines that fall within the render rectangle
		if (!(position_y + line_height < 0)) {
			PGTextRange current_range = line_iterator->GetCurrentRange();
			if (selected_line < 0) {
				// figure out the first cursor we could want to render with a binary search
				auto entry = std::lower_bound(cursors.begin(), cursors.end(), current_range.startpos(), [](const Cursor& lhs, const PGTextPosition& rhs) -> bool {
					return lhs.BeginCursorPosition() < rhs;
				});
				lng selected_entry = entry - cursors.begin();
				current_cursor = std::max((lng)0, std::min(selected_entry, (lng)cursors.size() - 1));
				selected_line = cursors[current_cursor].SelectedPosition().line;
			}

			assert(current_line.GetLength() >= 0);
			lng current_start_line = line_iterator->GetCurrentLineNumber();
			lng current_start_position = line_iterator->GetCurrentCharacterNumber();

			if (!minimap) {
				rendered_lines.push_back(RenderedLine(current_line, current_start_line,
					current_start_position, line_iterator->GetInnerLine(), view->wordwrap && current_line.syntax ? *current_line.syntax : PGSyntax()));
			}

			// render the linenumber of the current line if it is not wrapped
			if (!minimap && display_linenumbers && current_start_position == 0) {
				lng displayed_line = current_start_line;
				SetTextColor(textfield_font, PGStyleManager::GetColor(PGColorTextFieldLineNumber));

				if (displayed_line == selected_line) {
					// if a cursor selects this line, highlight the linenumber
					RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, line_height), PGStyleManager::GetColor(PGColorTextFieldSelection), PGDrawStyleFill);
				}

				// render the line number
				auto line_number = std::to_string(displayed_line + 1);
				RenderText(renderer, textfield_font, line_number.c_str(), line_number.size(), position_x + margin_width, position_y);
			}

			char* line = current_line.GetLine();
			lng length = current_line.GetLength();

			// find the part of the line that we actually have to render
			lng render_start = 0, render_end = length;
			auto character_widths = CumulativeCharacterWidths(font, line, length, xoffset, render_width, render_start, render_end);
			render_end = render_start + character_widths.size();
			if (character_widths.size() == 0) {
				if (render_start == 0 && render_end == 0) {
					// empty line, render cursor/selections
					character_widths.push_back(0);
					render_end = 1;
				} else {
					// the entire line is out of bounds, nothing to render
					if (!minimap) {
						cumulative_character_widths.push_back(character_widths);
						render_start_end.push_back(std::pair<lng, lng>(-1, -1));
					}
					goto next_line;
				}
			}
			if (!minimap) {
				cumulative_character_widths.push_back(character_widths);
				render_start_end.push_back(std::pair<lng, lng>(render_start, render_end));
			}

			bool render_search_match = false;
			// render any search matches
			while (current_match < matches.size()) {
				auto match = matches[current_match];
				if (match > current_range) {
					// this match is not rendered on this line yet
					break;
				}
				if (match <= current_range) {
					// this match has already been rendered
					current_match++;
					continue;
				}

				// we should render the match on this line
				lng start, end;
				if (match.startpos() < current_range.startpos()) {
					start = 0;
				} else {
					start = match.start_position - current_range.start_position;
				}
				if (match.endpos() > current_range.endpos()) {
					end = length + 1;
				} else {
					end = match.end_position - current_range.start_position;
				}
				if ((end >= render_start && start <= render_end) && start != end) {
					PGScalar x_offset = panther::clamped_access<PGScalar>(character_widths, start - render_start);
					PGScalar width = panther::clamped_access<PGScalar>(character_widths, end - render_start) - x_offset;
					if (end == length + 1) {
						width += MeasureTextWidth(font, " ");
					}
					PGRect rect(position_x_text + x_offset, position_y, width, line_height);
					RenderRectangle(renderer, rect, PGStyleManager::GetColor(PGColorTextFieldFindMatch), PGDrawStyleFill);
					render_search_match = true;
					if (end < length) {
						current_match++;
						continue;
					}
				} else if (end < render_start) {
					current_match++;
					continue;
				}
				break;
			}
			if (render_search_match) {
				// search match rendered on this line
				if (minimap) {
					// on the minimap, render a small rectangle to the left of the line to indicate that there is a search match on this line
					RenderRectangle(renderer, PGRect(position_x_text - line_height, position_y, line_height, line_height), PGStyleManager::GetColor(PGColorTextFieldFindMatch), PGDrawStyleFill);
				}
			}

			// render the selections and cursors, if there are any on this line
			while (current_cursor < cursors.size()) {
				PGTextRange range = cursors[current_cursor].GetCursorSelection();
				if (range > current_range) {
					// this cursor is not rendered on this line yet
					break;
				}
				if (range < current_range) {
					// this cursor has already been rendered
					// move to the next cursor
					current_cursor++;
					if (current_cursor < cursors.size()) {
						selected_line = cursors[current_cursor].SelectedPosition().line;
					}
					continue;
				}
				// we have to render the cursor on this line
				lng start, end;
				if (range.startpos() < current_range.startpos()) {
					start = 0;
				} else {
					start = range.start_position - current_range.start_position;
				}
				if (range.endpos() > current_range.endpos()) {
					end = length + 1;
				} else {
					end = range.end_position - current_range.start_position;
				}
				start = std::min(length, std::max((lng)0, start));
				end = std::min(length + 1, std::max((lng)0, end));
				if ((end >= render_start && start <= render_end) && start != end) {
					// if there is a selection and it is on the screen, render it
					RenderSelection(renderer,
						font,
						line,
						length,
						position_x_text,
						position_y,
						start,
						end,
						render_start,
						render_end,
						character_widths,
						selection_color);
				} else if (render_start > render_end) {
					// we are already past the to-be rendered text
					// any subsequent cursors will also be past this point
					break;
				}

				if (!minimap && display_carets) {
					PGTextPosition selected_position = cursors[current_cursor].SelectedCursorPosition();
					if (selected_position >= current_range.startpos() && selected_position <= current_range.endpos()) {
						// render the caret on the selected line
						lng render_position = selected_position.position - current_range.start_position;
						if (render_position >= render_start && render_position < render_end) {
							// check if the caret is in the rendered area first
							RenderCaret(renderer,
								font,
								character_widths[render_position - render_start],
								position_x_text,
								position_y,
								PGStyleManager::GetColor(PGColorTextFieldCaret));
						}
					}
				}
				if (end <= length) {
					current_cursor++;
					if (current_cursor < cursors.size()) {
						selected_line = cursors[current_cursor].SelectedPosition().line;
					}
					continue;
				}
				break;
			}
			
			// render the actual text
			if (!minimap) {
				// for the main textfield we render the actual text later
				goto next_line;
			}

			lng position = 0;
			
			PGScalar bitmap_x = position_x_text + character_widths[0];
			PGScalar bitmap_y = position_y;
			PGSyntax* syntax = current_line.syntax;
			if (syntax) {
				for (auto it = syntax->syntax.begin(); it != syntax->syntax.end(); it++) {
					bool squiggles = false;
					//assert(syntax->end > position);
					if (it->end <= position) {
						continue;
					}
					if (it->end >= render_start && position < render_end) {
						lng spos = std::max(position, render_start);
						lng epos = std::min(it->end, render_end - 1);
						PGColor color = PGStyleManager::GetColor(PGColorTextFieldText);
						if (it->type == PGSyntaxError) {
							squiggles = true;
						} else if (it->type == PGSyntaxNone) {
							color = PGStyleManager::GetColor(PGColorTextFieldText);
						} else if (it->type == PGSyntaxString) {
							color = PGStyleManager::GetColor(PGColorSyntaxString);
						} else if (it->type == PGSyntaxConstant) {
							color = PGStyleManager::GetColor(PGColorSyntaxConstant);
						} else if (it->type == PGSyntaxComment) {
							color = PGStyleManager::GetColor(PGColorSyntaxComment);
						} else if (it->type == PGSyntaxOperator) {
							color = PGStyleManager::GetColor(PGColorSyntaxOperator);
						} else if (it->type == PGSyntaxFunction) {
							color = PGStyleManager::GetColor(PGColorSyntaxFunction);
						} else if (it->type == PGSyntaxKeyword) {
							color = PGStyleManager::GetColor(PGColorSyntaxKeyword);
						} else if (it->type == PGSyntaxClass1) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass1);
						} else if (it->type == PGSyntaxClass2) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass2);
						} else if (it->type == PGSyntaxClass3) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass3);
						} else if (it->type == PGSyntaxClass4) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass4);
						} else if (it->type == PGSyntaxClass5) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass5);
						} else if (it->type == PGSyntaxClass6) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass6);
						}
						if (it->transparent) {
							color.a = 128;
						}
						SetTextColor(font, color);
						RenderText(renderer, font, line + spos, epos - spos, bitmap_x, bitmap_y);

						PGScalar text_width = character_widths[epos - render_start] - character_widths[spos - render_start];
						bitmap_x += text_width;
					}
					position = it->end;
				}
			}

			if (render_end > position) {
				position = std::max(position, render_start);
				SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
				RenderText(renderer, font, line + position, render_end - position - 1, bitmap_x, bitmap_y);
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
		}
next_line:
		(*line_iterator)++;
		position_y += line_height;
	}
	if (!minimap) {
		SetRenderBounds(renderer, PGRect(position_x_text, y, textfield_region.width, this->height));
		position_y = initial_position_y;
		lng index = 0;
		for (auto it2 = rendered_lines.begin(); it2 != rendered_lines.end(); it2++) {
			char* line = it2->tline.GetLine();
			lng length = it2->tline.GetLength();

			lng render_start = render_start_end[index].first, render_end = render_start_end[index].second;
			auto character_widths = cumulative_character_widths[index++];
			if (character_widths.size() == 0) {
				position_y += line_height;
				continue;
			}
			lng position = 0;

			PGScalar bitmap_x = position_x_text + character_widths[0];
			PGScalar bitmap_y = position_y;
			PGSyntax* syntax = view->wordwrap ? &it2->syntax : it2->tline.syntax;
			if (syntax) {
				for (auto it = syntax->syntax.begin(); it != syntax->syntax.end(); it++) {
					bool squiggles = false;
					//assert(syntax->end > position);
					if (it->end <= position) {
						continue;
					}
					if (it->end >= render_start && position < render_end) {
						lng spos = std::max(position, render_start);
						lng epos = std::min(it->end, render_end - 1);
						PGColor color = PGStyleManager::GetColor(PGColorTextFieldText);
						if (it->type == PGSyntaxError) {
							squiggles = true;
						} else if (it->type == PGSyntaxNone) {
							color = PGStyleManager::GetColor(PGColorTextFieldText);
						} else if (it->type == PGSyntaxString) {
							color = PGStyleManager::GetColor(PGColorSyntaxString);
						} else if (it->type == PGSyntaxConstant) {
							color = PGStyleManager::GetColor(PGColorSyntaxConstant);
						} else if (it->type == PGSyntaxComment) {
							color = PGStyleManager::GetColor(PGColorSyntaxComment);
						} else if (it->type == PGSyntaxOperator) {
							color = PGStyleManager::GetColor(PGColorSyntaxOperator);
						} else if (it->type == PGSyntaxFunction) {
							color = PGStyleManager::GetColor(PGColorSyntaxFunction);
						} else if (it->type == PGSyntaxKeyword) {
							color = PGStyleManager::GetColor(PGColorSyntaxKeyword);
						} else if (it->type == PGSyntaxClass1) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass1);
						} else if (it->type == PGSyntaxClass2) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass2);
						} else if (it->type == PGSyntaxClass3) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass3);
						} else if (it->type == PGSyntaxClass4) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass4);
						} else if (it->type == PGSyntaxClass5) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass5);
						} else if (it->type == PGSyntaxClass6) {
							color = PGStyleManager::GetColor(PGColorSyntaxClass6);
						}
						if (it->transparent) {
							color.a = 128;
						}
						SetTextColor(font, color);
						RenderText(renderer, font, line + spos, epos - spos, bitmap_x, bitmap_y);

						PGScalar text_width = character_widths[epos - render_start] - character_widths[spos - render_start];
						bitmap_x += text_width;
					}
					position = it->end;
				}
			}

			if (render_end > position) {
				position = std::max(position, render_start);
				SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
				RenderText(renderer, font, line + position, render_end - position - 1, bitmap_x, bitmap_y);
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
			position_y += line_height;
		}
		ClearRenderBounds(renderer);
	}

	if (!minimap) {
		this->line_height = line_height;
	} else {
		this->minimap_line_height = line_height;
		if (render_overlay) {
			// render the overlay for the minimap
			PGRect rect(position_x_text, initial_position_y + GetMinimapOffset(), max_x - position_x_text, line_height * GetLineHeight());
			RenderRectangle(renderer, rect,
				this->drag_type == PGDragMinimap ?
				PGStyleManager::GetColor(PGColorMinimapDrag) :
				PGStyleManager::GetColor(PGColorMinimapHover)
				, PGDrawStyleFill);
		}
	}
}

void TextField::Draw(PGRendererHandle renderer) {
	bool window_has_focus = WindowHasFocus(window);
	PGPoint position = Position();
	PGScalar x = position.x, y = position.y;

	// prevent rendering outside of the textfield
	SetRenderBounds(renderer, PGRect(x, y, this->width, this->height));

	if (view->file->IsLoaded()) {
		view->file->Lock(PGReadLock);
		// determine the width of the line numbers
		lng line_count = view->GetMaxYScroll();
		text_offset = 0;
		if (this->display_linenumbers) {
			auto line_number = std::to_string(std::max((lng)10, view->GetMaxYScroll() + 1));
			text_offset = 10 + MeasureTextWidth(textfield_font, line_number.c_str(), line_number.size());
		}
		// get the mouse position (for rendering hovers)
		PGPoint mouse = GetMousePosition(window, this);
		PGScalar position_x = x;
		PGScalar position_y = y;
		// textfield/minimap dimensions
		PGScalar textfield_width = this->width - (this->display_minimap ? minimap_region.width + 5 : 0);
		// determine x-offset and clamp it
		PGScalar max_textsize = view->file->GetMaxLineWidth(textfield_font);
		PGScalar xoffset = 0;
		if (view->GetWordWrap()) {
			max_xoffset = 0;
		} else {
			max_xoffset = std::max(max_textsize - textfield_width + text_offset + 2 * margin_width + 10, 0.0f);
			xoffset = view->GetXOffset();
			if (xoffset > max_xoffset) {
				xoffset = max_xoffset;
				view->SetXOffset(max_xoffset);
			} else if (xoffset < 0) {
				xoffset = 0;
				view->SetXOffset(xoffset);
			}
		}
		// render the background of the textfield
		RenderRectangle(renderer, PGIRect(x, y, textfield_width, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);

		// render the actual text field
		textfield_region.width = textfield_width - text_offset - margin_width * 2;
		textfield_region.height = this->height;
		view->SetWordWrap(view->GetWordWrap(), GetTextfieldWidth());
		PGIRect rectangle = PGIRect(0, 0, this->width, this->height);
		DrawTextField(renderer, textfield_font, false, x, x + text_offset + margin_width * 2, y, textfield_width, false);

		// render the minimap
		if (this->display_minimap) {
			bool mouse_in_minimap = window_has_focus && this->mouse_in_minimap;
			PGIRect shadow_rect = PGIRect(x + textfield_width - 5, y, 5, this->height);
			if (is_minimap_dirty) {
				// only rerender the actual minimap if it is dirty

				// render the background of the minimap
				RenderRectangle(renderer, PGIRect(x + textfield_width, y, minimap_region.width, this->height), PGStyleManager::GetColor(PGColorMainMenuBackground), PGDrawStyleFill);
				DrawTextField(renderer, minimap_font, true, x + textfield_width + 1, x + textfield_width, y, minimap_region.width, mouse_in_minimap);
			}
			if (!view->GetWordWrap()) {
				// render the minimap gradient between the minimap and the textfield
				RenderGradient(renderer, shadow_rect, PGColor(0, 0, 0, 0), PGColor(0, 0, 0, 128));	
			}
		}

		// render the scrollbar
		if (this->display_scrollbar) {
			scrollbar->UpdateValues(0,
				view->GetMaxYScroll(),
				GetLineHeight(),
				view->GetWordWrap() ? view->GetScrollPercentage() *  view->GetMaxYScroll() : view->GetLineOffset().linenumber);
			scrollbar->Draw(renderer);
			// horizontal scrollbar
			display_horizontal_scrollbar = max_xoffset > 0;
			if (display_horizontal_scrollbar) {
				horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), view->GetXOffset());
				horizontal_scrollbar->Draw(renderer);
			}
		}
		view->file->Unlock(PGReadLock);
	} else {
		// render the background
		RenderRectangle(renderer, PGIRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);

		if (view->file->bytes >= 0) {
			// file is currently being loaded, display the progress bar
			PGScalar offset = this->width / 10;
			PGScalar width = this->width - offset * 2;
			PGScalar height = 5;
			PGScalar padding = 1;
			double load_percentage = view->file->LoadPercentage();

			PGScalar x = X(), y = Y();

			RenderRectangle(renderer, PGRect(x + offset - padding, y + this->height / 2 - height / 2 - padding, width + 2 * padding, height + 2 * padding), PGColor(191, 191, 191), PGDrawStyleFill);
			RenderRectangle(renderer, PGRect(x + offset, y + this->height / 2 - height / 2, width * load_percentage, height), PGColor(20, 60, 255), PGDrawStyleFill);
		} else {
			// error loading file: display nothing
		}
	}
	ClearRenderBounds(renderer);

	is_minimap_dirty = false;
	Control::Draw(renderer);
}

PGScalar TextField::GetTextfieldWidth() {
	return textfield_region.width;
}

PGScalar TextField::GetTextfieldHeight() {
	return textfield_region.height;
}

#define MAXIMUM_MINIMAP_WIDTH 150.0f
PGScalar TextField::GetMinimapWidth() {
	return std::min(MAXIMUM_MINIMAP_WIDTH, this->width / 7.0f);
}

PGScalar TextField::GetMinimapHeight() {
	return minimap_line_height * GetLineHeight();
}

void TextField::GetMinimapLinesRendered(lng& lines_rendered, double& percentage) {
	lines_rendered = minimap_region.height / (minimap_line_height == 0 ? 1 : minimap_line_height);
	lng max_y_scroll = view->GetMaxYScroll();
	if (!view->GetWordWrap()) {
		percentage = max_y_scroll == 0 ? 0 : (double)view->GetLineOffset().linenumber / max_y_scroll;
	} else {
		percentage = view->GetScrollPercentage();
	}
}

PGScalar TextField::GetMinimapOffset() {
	lng total_lines_rendered;
	double percentage = 0;
	GetMinimapLinesRendered(total_lines_rendered, percentage);
	lng line_offset;
	view->OffsetVerticalScroll(view->GetLineOffset(), -(total_lines_rendered * percentage), line_offset);
	return this->height * ((double)line_offset / (double)total_lines_rendered);
}

PGVerticalScroll TextField::GetMinimapStartLine() {
	lng lines_rendered;
	double percentage = 0;
	GetMinimapLinesRendered(lines_rendered, percentage);
	auto current_scroll = view->GetLineOffset();
	auto scroll = view->OffsetVerticalScroll(current_scroll, -(lines_rendered * percentage));
	auto max_scroll = view->OffsetVerticalScroll(view->OffsetVerticalScroll(current_scroll, lines_rendered), -(lines_rendered - 1));
	if (scroll.linenumber > max_scroll.linenumber ||
		(scroll.linenumber == max_scroll.linenumber && scroll.inner_line > max_scroll.inner_line)) {
		scroll = max_scroll;
	}
	scroll.line_fraction = 0;
	return scroll;
}

void TextField::SetMinimapOffset(PGScalar offset) {
	// compute lineoffset_y from minimap offset
	double percentage = (double)offset / this->height;
	lng lines_rendered = this->height / minimap_line_height;
	lng start_line = std::max((lng)(((lng)((std::max((lng)1, view->GetMaxYScroll()) * percentage))) - (lines_rendered * percentage)), (lng)0);
	lng lineoffset_y = start_line + (lng)(lines_rendered * percentage);
	lineoffset_y = std::max((lng)0, std::min(lineoffset_y, view->GetMaxYScroll()));
	view->SetScrollOffset(lineoffset_y);
}

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	if (!view->file->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (PGRectangleContains(scrollbar->GetRectangle(), mouse)) {
		scrollbar->UpdateValues(0, view->GetMaxYScroll(), GetLineHeight(), view->GetLineOffset().linenumber);
		scrollbar->MouseDown(mouse.x, mouse.y, button, modifier, click_count);
		return;
	}
	if (PGRectangleContains(horizontal_scrollbar->GetRectangle(), mouse)) {
		horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), view->GetXOffset());
		horizontal_scrollbar->MouseDown(mouse.x, mouse.y, button, modifier, click_count);
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
				drag_button = PGLeftMouseButton;
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
	}
	lng line, character;
	GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
	if (this->PressMouseButton(TextField::mousebindings, button, mouse, modifier, click_count, line, character)) {
		return;
	}
	if (this->PressMouseButton(BasicTextField::mousebindings, button, mouse, modifier, click_count, line, character)) {
		return;
	}
}

void TextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (PGRectangleContains(scrollbar->GetRectangle(), mouse)) {
		scrollbar->UpdateValues(0, view->GetMaxYScroll(), GetLineHeight(), view->GetLineOffset().linenumber);
		scrollbar->MouseUp(mouse.x, mouse.y, button, modifier);
		return;
	}
	if (PGRectangleContains(horizontal_scrollbar->GetRectangle(), mouse)) {
		horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), view->GetXOffset());
		horizontal_scrollbar->MouseUp(mouse.x, mouse.y, button, modifier);
		return;
	}
	if (button & PGRightMouseButton) {
		if (!(mouse.x <= GetTextfieldWidth() && mouse.y <= GetTextfieldHeight())) return;
		PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
		PGPopupMenuInsertEntry(menu, "Show Unsaved Changes...", nullptr, PGPopupMenuGrayed);
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertCommand(menu, "Copy", "copy", BasicTextField::keybindings_noargs, BasicTextField::keybindings, BasicTextField::keybindings_images);
		PGPopupMenuInsertCommand(menu, "Cut", "cut", BasicTextField::keybindings_noargs, BasicTextField::keybindings, BasicTextField::keybindings_images);
		PGPopupMenuInsertCommand(menu, "Paste", "paste", BasicTextField::keybindings_noargs, BasicTextField::keybindings, BasicTextField::keybindings_images);
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertCommand(menu, "Find In File", "show_find", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images);
		PGPopupMenuInsertCommand(menu, "Select Everything", "select_all", BasicTextField::keybindings_noargs, BasicTextField::keybindings, BasicTextField::keybindings_images);
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuFlags flags = view->file->FileInMemory() ? PGPopupMenuGrayed : PGPopupMenuFlagsNone;
		PGPopupMenuInsertCommand(menu, "Open File in Explorer", "open_file_in_explorer", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, flags);
		PGPopupMenuInsertCommand(menu, "Open File in Terminal", "open_file_in_terminal", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, flags);
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertCommand(menu, "Copy File Name", "copy_file_name", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, flags);
		PGPopupMenuInsertCommand(menu, "Copy Full Path", "copy_file_path", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, flags);
		PGPopupMenuInsertCommand(menu, "Reveal in Side Bar", "reveal_in_sidebar", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, flags);
		PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
		return;
	}
	BasicTextField::MouseUp(x, y, button, modifier);
}

void TextField::MouseMove(int x, int y, PGMouseButton buttons) {
	if (!view->file->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);
	if (scrollbar->IsDragging()) {
		scrollbar->UpdateValues(0, view->GetMaxYScroll(), GetLineHeight(), view->GetLineOffset().linenumber);
		scrollbar->MouseMove(mouse.x, mouse.y, buttons);
		return;
	}
	if (horizontal_scrollbar->IsDragging()) {
		horizontal_scrollbar->UpdateValues(0, max_xoffset, GetTextfieldWidth(), view->GetXOffset());
		horizontal_scrollbar->MouseMove(mouse.x, mouse.y, buttons);
		return;
	}

	if (this->drag_type != PGDragNone) {
		if (buttons & drag_button) {
			if (drag_type == PGDragMinimap) {
				PGVerticalScroll current_scroll = view->GetLineOffset();
				SetMinimapOffset(mouse.y - drag_offset);
				if (current_scroll != view->GetLineOffset())
					this->Invalidate();
				return;
			} else if (drag_type == PGDragSelectionCursors) {
				// FIXME: this should be moved inside the textfile
				lng line, character;
				mouse.x = mouse.x - text_offset + view->GetXOffset();
				GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
				std::vector<Cursor>& cursors = view->GetCursors();
				Cursor& active_cursor = view->GetActiveCursor();
				auto end_pos = active_cursor.UnselectedPosition();
				auto start_scroll = view->GetVerticalScroll(end_pos.line, end_pos.position);
				cursors.clear();
				bool backwards = end_pos.line > line || (end_pos.line == line && end_pos.position > character);
				PGScalar start_x;
				bool single_cursor = true;
				auto iterator = view->GetScrollIterator(this, start_scroll);
				while (true) {
					TextLine textline = iterator->GetLine();
					if (!textline.IsValid()) break;

					lng iterator_line = iterator->GetCurrentLineNumber();
					lng iterator_character = iterator->GetCurrentCharacterNumber();

					if (backwards) {
						if (iterator_line < line ||
							(iterator_line == line && iterator_character + textline.GetLength() < character))
							break;
					} else {
						if (iterator_line > line ||
							(iterator_line == line && iterator_character > character))
							break;
					}

					lng end_position = GetPositionInLine(textfield_font, mouse.x, textline.GetLine(), textline.GetLength());
					lng start_position = GetPositionInLine(textfield_font, drag_offset - text_offset + view->GetXOffset(), textline.GetLine(), textline.GetLength());
					if (start_position != end_position && single_cursor) {
						single_cursor = false;
						cursors.clear();
					}

					if (single_cursor || start_position != end_position) {
						Cursor cursor = Cursor(view.get(), iterator_line, iterator_character + end_position, iterator_line, iterator_character + start_position);
						if (backwards) {
							cursors.insert(cursors.begin(), cursor);
						} else {
							cursors.push_back(cursor);
						}
					}

					if (backwards) {
						(*iterator)--;
					} else {
						(*iterator)++;
					}
				}
				if (backwards) {
					view->active_cursor = cursors.size() - 1;
				}
				this->Invalidate();
				return;
			}
		}
	}
	BasicTextField::MouseMove(x, y, buttons);
}

bool TextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(TextField::keybindings, button, modifier)) {
		return true;
	}
	return BasicTextField::KeyboardButton(button, modifier);
}

void TextField::IncreaseFontSize(int modifier) {
	SetTextFontSize(textfield_font, GetTextFontSize(textfield_font) + modifier);
	this->Invalidate();
}

bool TextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!view->file->IsLoaded()) return false;
	if (this->PressCharacter(TextField::keybindings, character, modifier)) {
		return true;
	}

	if (modifier == PGModifierCtrl) {
		switch (character) {
			case 'Q':
			{
				PGNotification* notification = new PGNotification(window, PGNotificationTypeError, this->width * 0.6f, "Unknown error.");
				notification->SetPosition(PGPoint(this->x + this->width * 0.2f, this->y));

				notification->AddButton([](Control* control, void* data) {
					dynamic_cast<PGContainer*>(control->parent.lock().get())->RemoveControl((Control*)data);
				}, this, notification, "Cancel");
				dynamic_cast<PGContainer*>(this->parent.lock().get())->AddControl(std::shared_ptr<Control>(notification));

				return true;
			}
		}
	}
	if (modifier == PGModifierCtrlShift) {
		switch (character) {
			case 'Q':
				// toggle word wrap
				view->SetWordWrap(!view->GetWordWrap(), GetTextfieldWidth());
				this->Invalidate();
				return true;
			default:
				break;
		}
	}
	return BasicTextField::KeyboardCharacter(character, modifier);
}

void TextField::DisplayGotoDialog(PGGotoType goto_type) {
	PGGotoAnything* goto_anything;
	if (active_searchbox != nullptr) {
		goto_anything = dynamic_cast<PGGotoAnything*>(active_searchbox);
		if (goto_anything == nullptr) {
			dynamic_cast<PGContainer*>(this->parent.lock().get())->RemoveControl(active_searchbox);
			this->active_searchbox = nullptr;
		} else {
			goto_anything->SetType(goto_type);
			return;
		}
	}

	auto g = make_shared_control<PGGotoAnything>(this, this->window, goto_type);
	goto_anything = g.get();
	goto_anything->SetSize(PGSize(goto_anything->width, GetTextHeight(textfield_font) + 306));
	goto_anything->SetPosition(PGPoint(this->x + (this->width - goto_anything->width) * 0.5f, this->y));
	this->active_searchbox = goto_anything;
	this->active_searchbox->OnDestroy([](Control* c, void* data) {
		TextField* tf = static_cast<TextField*>(data);
		tf->ClearSearchBox(c);
	}, this);
	dynamic_cast<PGContainer*>(this->parent.lock().get())->AddControl(g);
}

void TextField::DisplaySearchBox(std::vector<SearchEntry>& entries, SearchBoxCloseFunction close_function, void* close_data) {
	if (active_searchbox != nullptr) {
		dynamic_cast<PGContainer*>(this->parent.lock().get())->RemoveControl(active_searchbox);
		this->active_searchbox = nullptr;
	}
	auto searchbox = make_shared_control<SearchBox>(this->window, entries, false);
	searchbox->SetSize(PGSize(500, GetTextHeight(textfield_font) + 306));
	searchbox->SetPosition(PGPoint(this->x + (this->width - searchbox->width) * 0.5f, this->y));
	searchbox->OnClose(close_function, close_data);
	this->active_searchbox = searchbox.get();
	this->active_searchbox->OnDestroy([](Control* c, void* data) {
		TextField* tf = static_cast<TextField*>(data);
		tf->ClearSearchBox(c);
	}, this);
	dynamic_cast<PGContainer*>(this->parent.lock().get())->AddControl(searchbox);
}

void TextField::CloseSearchBox() {
	if (active_searchbox != nullptr) {
		dynamic_cast<PGContainer*>(this->parent.lock().get())->RemoveControl(active_searchbox);
		this->active_searchbox = nullptr;
	}
}

void TextField::ClearSearchBox(Control * searchbox) {
	if (this->active_searchbox == searchbox) {
		this->active_searchbox = nullptr;
	}
}

void TextField::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		if (distance != 0) {
			//double offset = -distance / GetTextHeight(textfield_font);
			//vscroll_left -= offset;
			//vscroll_speed += std::abs(offset / 5.0);
			view->OffsetLineOffset(-distance / GetTextHeight(textfield_font));
			this->Invalidate();
		}
	}
	if (hdistance != 0) {
		view->SetXOffset(std::max(0.0, view->GetXOffset() - hdistance / 10.0));
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
	this->Invalidate();
}

void TextField::SetTextView(std::shared_ptr<TextView> view) {
	this->prev_loaded = false;
	if (this->view != view && this->view) {
		this->view->file->last_modified_deletion = false;
		this->view->file->last_modified_notification = this->view->file->last_modified_time;
		ClearNotification();
	}
	this->view = view;
	view->SetTextField(this);
	GetControlManager(this)->ActiveFileChanged(this);
	this->SelectionChanged();
	this->TextChanged();
}

void TextField::SelectionChanged() {
	if (view->file->path.size() > 0) {
		if (!notification) {
			auto stats = PGGetFileFlags(view->file->path);
			if (stats.flags == PGFileFlagsFileNotFound) {
				// the current file has been deleted
				// notify the user if they have not been notified before
				if (!view->file->last_modified_deletion) {
					view->file->last_modified_deletion = true;
					CreateNotification(PGNotificationTypeWarning, std::string("File \"") + PGFile(view->file->path).Filename() + std::string("\" appears to have been moved or deleted."));
					assert(notification);
					notification->AddButton([](Control* control, void* data) {
						((TextField*)control)->ClearNotification();
					}, this, notification, "OK");
					ShowNotification();
				}
			} else if (stats.flags == PGFileFlagsEmpty) {
				// the current file exists
				view->file->last_modified_deletion = false;

				if (view->file->last_modified_notification < 0) {
					view->file->last_modified_notification = stats.modification_time;
				} else if (view->file->last_modified_notification < stats.modification_time &&
						view->file->reload_on_changed) {
					// however, it has been modified by an external program since we last touched it
					// notify the user and prompt to reload the file
					lng threshold = 0;
					assert(stats.file_size >= 0);
					PGSettingsManager::GetSetting("automatic_reload_threshold", threshold);
					if (stats.file_size < threshold) {
						// file is below automatic threshold: reload without prompting the user
						view->file->last_modified_time = stats.modification_time;
						view->file->last_modified_notification = stats.modification_time;
						PGFileError error;
						if (!view->file->Reload(error)) {
							DisplayNotification(error);
						}
					} else {
						// file is too large for automatic reload: prompt the user
						view->file->last_modified_notification = stats.modification_time;
						CreateNotification(PGNotificationTypeWarning, std::string("File \"") + PGFile(view->file->path).Filename() + std::string("\" has been modified."));
						assert(notification);
						notification->AddButton([](Control* control, void* data) {
							TextField* tf = (TextField*)control;
							tf->view->file->reload_on_changed = false;
							((TextField*)control)->ClearNotification();
						}, this, notification, "Never For This File");
						notification->AddButton([](Control* control, void* data) {
							((TextField*)control)->ClearNotification();
						}, this, notification, "Cancel");
						notification->AddButton([](Control* control, void* data) {
							((TextField*)control)->ClearNotification();
							PGFileError error;
							if (!((TextFile*)data)->Reload(error)) {
								((TextField*)control)->DisplayNotification(error);
							}
						}, this, view->file.get(), "Reload");
						ShowNotification();
					}
				}
			}
		}
	}
	BasicTextField::SelectionChanged();
	GetControlManager(this)->SelectionChanged(this);
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
	scrollbar->SetPosition(PGPoint(this->width - scrollbar->width, 0));
	scrollbar->SetSize(PGSize(SCROLLBAR_SIZE, this->height));
	horizontal_scrollbar->SetPosition(PGPoint(0, this->height - SCROLLBAR_SIZE));
	horizontal_scrollbar->SetSize(PGSize(this->width - SCROLLBAR_SIZE, SCROLLBAR_SIZE));
}

PGCursorType TextField::GetCursor(PGPoint mouse) {
	mouse.x -= this->x;
	mouse.y -= this->y;
	if (!view->file->IsLoaded()) {
		if (view->file->bytes < 0) {
			return PGCursorStandard;
		}
		return PGCursorWait;
	}

	if (mouse.x >= text_offset && mouse.x <= this->width - minimap_region.width &&
		(!display_horizontal_scrollbar || mouse.y <= this->height - SCROLLBAR_SIZE)) {
		return PGCursorIBeam;
	}
	return PGCursorStandard;
}

void TextField::TextChanged() {
	BasicTextField::TextChanged();
	GetControlManager(this)->TextChanged(this);
}

bool TextField::IsDragging() {
	if (scrollbar->IsDragging()) return true;
	if (horizontal_scrollbar->IsDragging()) return true;
	return BasicTextField::IsDragging();
}

void TextField::GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character) {
	if (!view->GetWordWrap()) {
		return BasicTextField::GetLineCharacterFromPosition(x, y, line, character);
	}
	auto offset = view->GetLineOffset();
	PGScalar line_height = GetTextHeight(textfield_font);
	y += line_height * offset.line_fraction;
	if (y < 0) {
		lng line_offset = (lng)(y / line_height);
		PGVerticalScroll scroll = PGVerticalScroll(rendered_lines.front().line, rendered_lines.front().inner_line);
		auto iterator = view->GetScrollIterator(this, scroll);
		while(line_offset < 0 && iterator->GetLine().IsValid()) {
			(*iterator)--;
			line_offset++;
		}
		line = iterator->GetCurrentLineNumber();
		character = iterator->GetCurrentCharacterNumber();
	} else if (y > this->height) {
		lng line_offset = (lng)((y - this->height) / line_height);
		PGVerticalScroll scroll = PGVerticalScroll(rendered_lines.back().line, rendered_lines.back().inner_line);
		auto iterator = view->GetScrollIterator(this, scroll);
		while(line_offset > 0 && iterator->GetLine().IsValid()) {
			(*iterator)++;
			line_offset--;
		}
		line = iterator->GetCurrentLineNumber();
		character = iterator->GetCurrentCharacterNumber();
	} else {
		lng line_offset = std::max(std::min((lng)(y / line_height), (lng)(rendered_lines.size() - 1)), (lng)0);
		line = rendered_lines[line_offset].line;
		_GetCharacterFromPosition(x, rendered_lines[line_offset].tline, character);
		character += rendered_lines[line_offset].position;
	}

}

void TextField::GetLineFromPosition(PGScalar y, lng& line) {
	if (!view->GetWordWrap()) {
		return BasicTextField::GetLineFromPosition(y, line);
	}
	lng line_offset = std::max(std::min((lng)(y / GetTextHeight(textfield_font)), (lng)(rendered_lines.size() - 1)), (lng)0);
	line = rendered_lines[line_offset].line;
}

void TextField::GetPositionFromLineCharacter(lng line, lng character, PGScalar& x, PGScalar& y) {
	if (!view->GetWordWrap()) {
		return BasicTextField::GetPositionFromLineCharacter(line, character, x, y);
	}
	x = 0;
	y = 0;
	if (rendered_lines.size() == 0) return;
	if (rendered_lines.front().line > line || rendered_lines.back().line < line) return;
	lng index = 0;
	for (index = 1; index < rendered_lines.size(); index++) {
		if (rendered_lines[index].line > line ||
			(rendered_lines[index].line == line && rendered_lines[index].position > character)) {
			index = index - 1;
			break;
		} else if (index == rendered_lines.size() - 1) {
			return;
		}
	}
	if (rendered_lines[index].line != line) return;

	y = index * GetTextHeight(textfield_font);
	_GetPositionFromCharacter(character - rendered_lines[index].position, rendered_lines[index].tline, x);
}

void TextField::GetPositionFromLine(lng line, PGScalar& y) {
	if (!view->GetWordWrap()) {
		return BasicTextField::GetPositionFromLine(line, y);
	}
	PGScalar x;
	lng character = 0;
	this->GetPositionFromLineCharacter(line, character, x, y);
}

void TextField::InitializeKeybindings() {
	std::map<std::string, PGBitmapHandle>& images = TextField::keybindings_images;
	std::map<std::string, PGKeyFunction>& noargs = TextField::keybindings_noargs;
	images["save"] = PGStyleManager::GetImage("data/icons/save.png");
	noargs["save"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		auto file = tf->view->file;
		if (file->FileInMemory()) {
			std::string filename = ShowSaveFileDialog();
			if (filename.size() != 0) {
				file->SaveAs(filename);
			}
		} else {
			file->SaveChanges();
		}
	};
	images["save_as"] = PGStyleManager::GetImage("data/icons/saveas.png");
	noargs["save_as"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		std::string filename = ShowSaveFileDialog();
		if (filename.size() != 0) {
			tf->view->file->SaveAs(filename);
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
	noargs["swap_line_down"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		assert(0);
	};
	noargs["show_find"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		ControlManager* cm = GetControlManager(tf);
		std::string text = tf->view->CopyText();
		cm->ShowFindReplace(PGFindSingleFile, text);
	};
	noargs["insert_newline_before"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->view->AddEmptyLine(PGDirectionLeft);
	};
	noargs["insert_newline_after"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->view->AddEmptyLine(PGDirectionRight);
	};
	images["increase_indent"] = PGStyleManager::GetImage("data/icons/increaseindent.png");
	noargs["increase_indent"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->view->IndentText(PGDirectionRight);
	};
	images["decrease_indent"] = PGStyleManager::GetImage("data/icons/decreaseindent.png");
	noargs["decrease_indent"] = [](Control* c) {
		TextField* tf = (TextField*)c;
		tf->view->IndentText(PGDirectionLeft);
	};
	noargs["open_file_in_explorer"] = [](Control* c) {
		TextField* t = (TextField*)c;
		OpenFolderInExplorer(t->view->file->GetFullPath());
	};
	noargs["open_file_in_terminal"] = [](Control* c) {
		TextField* t = (TextField*)c;
		OpenFolderInTerminal(t->view->file->GetFullPath());
	};
	noargs["copy_file_name"] = [](Control* c) {
		TextField* t = (TextField*)c;
		SetClipboardText(t->window, t->view->file->GetName());
	};
	noargs["copy_file_path"] = [](Control* c) {
		TextField* t = (TextField*)c;
		SetClipboardText(t->window, t->view->file->GetFullPath());
	};
	noargs["reveal_in_sidebar"] = [](Control* c) {
		TextField* t = (TextField*)c;
		ControlManager* cm = GetControlManager(t);
		cm->active_projectexplorer->RevealFile(t->view->file->GetFullPath(), false);
	};
	std::map<std::string, PGKeyFunctionArgs>& args = TextField::keybindings_varargs;
	// FIXME: duplicate of BasicTextField::insert
	args["insert"] = [](Control* c, std::map<std::string, std::string> args) {
		TextField* tf = (TextField*)c;
		if (args.count("characters") == 0) {
			return;
		}
		tf->view->PasteText(args["characters"]);
	};
	args["show_goto"] = [](Control* c, std::map<std::string, std::string> args) {
		TextField* tf = (TextField*)c;
		if (args.count("type") == 0) {
			return;
		}
		if (args["type"] == "line") {
			tf->DisplayGotoDialog(PGGotoLine);
		} else if (args["type"] == "file") {
			tf->DisplayGotoDialog(PGGotoFile);
		}
	};
	std::map<std::string, PGMouseFunction>& mouse_bindings = TextField::mousebindings_noargs;
	mouse_bindings["drag_region"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		TextField* tf = (TextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->StartDragging(PGMiddleMouseButton, PGDragSelectionCursors);
		tf->drag_offset = mouse.x;
		tf->view->SetCursorLocation(line, character);
	};
}

void TextField::CreateNotification(PGNotificationType type, std::string text) {
	if (this->notification) {
		this->ClearNotification();
	}
	this->notification = new PGNotification(window, type, this->width * 0.6f, text);
	this->notification->SetPosition(PGPoint(this->x + this->width * 0.2f, this->y));
}

void TextField::ShowNotification() {
	assert(notification);
	dynamic_cast<PGContainer*>(this->parent.lock().get())->AddControl(std::shared_ptr<Control>(notification));
}

void TextField::ClearNotification() {
	if (this->notification) {
		dynamic_cast<PGContainer*>(this->parent.lock().get())->RemoveControl(this->notification);
		this->notification = nullptr;
	}
}

void TextField::DisplayNotification(PGFileError error) {
	if (error == PGFileSuccess) return;
	std::string error_message;
	switch (error) {
		case PGFileAccessDenied:
			error_message = "Access denied.";
			break;
		case PGFileNoSpace:
			error_message = "No space left on device.";
			break;
		case PGFileTooLarge:
			error_message = "File too large.";
			break;
		case PGFileReadOnlyFS:
			error_message = "Read only file system.";
			break;
		case PGFileNameTooLong:
			error_message = "File name too long.";
			break;
		case PGFileIOError:
			error_message = "Unspecified I/O Error.";
			break;
		case PGFileEncodingFailure:
			error_message = "Failed to encode/decode file.";
			break;
		default:
			return;
	}
	CreateNotification(PGNotificationTypeError, error_message);

	assert(notification);
	notification->AddButton([](Control* control, void* data) {
		// FIXME: close tab
		((TextField*)control)->ClearNotification();
	}, this, notification, "Close");
	ShowNotification();
}

void TextField::Invalidate(bool initial_invalidate) {
	is_minimap_dirty = true;
	Control::Invalidate(initial_invalidate);
}

void TextField::SearchMatchesChanged() {
	lng line, character;
	scrollbar->center_decorations.clear();
	double prev_percentage = -1.0;
	double total_lines = view->file->GetLineCount();
	for (auto it = view->matches.begin(); it != view->matches.end(); it++) {
		PGTextPosition positions[2] = { it->startpos(), it->endpos() };
		for (int i = 0; i < 2; i++) {
			PGTextPosition pos = positions[i];
			pos.buffer->GetCursorFromBufferLocation(pos.position, line, character);
			double percentage = line / total_lines;
			lng distance = std::abs((percentage * scrollbar->height) - (prev_percentage * scrollbar->height));
			if (distance >= 1) {
				scrollbar->center_decorations.push_back(PGScrollbarDecoration(percentage, PGStyleManager::GetColor(PGColorTextFieldFindMatch)));
				prev_percentage = percentage;
			}
		}

	}
}

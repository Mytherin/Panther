
#include "simpletextfield.h"
#include "style.h"

#include "container.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(SimpleTextField);

SimpleTextField::SimpleTextField(PGWindowHandle window, bool multiline) :
	BasicTextField(window, std::shared_ptr<TextFile>(new TextFile(nullptr))), valid_input(true),
	on_user_cancel(), on_user_confirm(), on_prev_entry(), on_next_entry(), render_background(true) {
	this->support_multiple_lines = multiline;
	this->height = std::ceil(GetTextHeight(textfield_font)) + 6;
}

SimpleTextField::~SimpleTextField() {
}

void SimpleTextField::LosesFocus(void) {
	this->Invalidate();
	BasicTextField::LosesFocus();
}

void SimpleTextField::GainsFocus(void) {
	this->Invalidate();
	BasicTextField::GainsFocus();
}

void SimpleTextField::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();
	PGScalar initial_position_y = y;
	PGScalar max_x = x + this->width;
	PGScalar xoffset = textfile->GetXOffset();

	PGRect textfield_region = PGRect(x, y, this->width, this->height);

	bool has_focus = this->ControlHasFocus();
	RenderRectangle(renderer, textfield_region, PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);

	x += 4;
	y += 2;

	const std::vector<Cursor>& cursors = textfile->GetCursors();
	lng current_cursor = 0;

	SetTextColor(textfield_font, PGStyleManager::GetColor(PGColorTextFieldText));
	PGScalar line_height = GetTextHeight(textfield_font);

	auto line_iterator = textfile->GetLineIterator(this, textfile->GetLineOffset().linenumber);
	TextLine textline;
	SetRenderBounds(renderer, textfield_region);
	while ((textline = line_iterator->GetLine()).IsValid()) {
		if (y > initial_position_y + this->height) break;

		char* line = textline.GetLine();
		lng length = textline.GetLength();
		PGTextRange current_range = line_iterator->GetCurrentRange();

		lng render_start = 0, render_end = length;
		auto character_widths = CumulativeCharacterWidths(textfield_font, line, length, xoffset, this->width, render_start, render_end);
		render_end = render_start + character_widths.size();
		if (character_widths.size() == 0) {
			if (render_start == 0 && render_end == 0) {
				// empty line, render cursor/selections
				character_widths.push_back(0);
				render_end = 1;
			} else {
				// the entire line is out of bounds, nothing to render
				return;
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
					textfield_font,
					line,
					length,
					x,
					y,
					start,
					end,
					render_start,
					render_end,
					character_widths,
					PGStyleManager::GetColor(PGColorTextFieldSelection));
			} else if (render_start > render_end) {
				// we are already past the to-be rendered text
				// any subsequent cursors will also be past this point
				break;
			}

			if (display_carets) {
				PGTextPosition selected_position = cursors[current_cursor].SelectedCursorPosition();
				if (selected_position >= current_range.startpos() && selected_position <= current_range.endpos()) {
					// render the caret on the selected line
					lng render_position = selected_position.position - current_range.start_position;
					if (render_position >= render_start && render_position < render_end) {
						// check if the caret is in the rendered area first
						RenderCaret(renderer,
							textfield_font,
							character_widths[render_position - render_start],
							x,
							y,
							PGStyleManager::GetColor(PGColorTextFieldCaret));
					}
				}
			}
			if (end <= length) {
				current_cursor++;
				continue;
			}
			break;
		}
		RenderText(renderer, textfield_font, line + render_start, render_end - render_start - 1, x + character_widths[0], y, max_x);

		if (!support_multiple_lines) break;
		(*line_iterator)++;
		y += line_height;
	}
	ClearRenderBounds(renderer);
	RenderRectangle(renderer, textfield_region,
		valid_input ?
		(has_focus ? PGStyleManager::GetColor(PGColorTabControlSelected) : PGStyleManager::GetColor(PGColorTextFieldCaret)) : PGStyleManager::GetColor(PGColorTextFieldError),
		PGDrawStyleStroke);
	Control::Draw(renderer);
}

bool SimpleTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(SimpleTextField::keybindings, button, modifier)) {
		return true;
	}
	return BasicTextField::KeyboardButton(button, modifier);
}

std::string SimpleTextField::GetText() {
	return textfile->GetText();
}

void SimpleTextField::SetText(std::string text) {
	textfile->SelectEverything();
	if (text.size() > 0) {
		textfile->PasteText(text);
	} else {
		textfile->DeleteCharacter(PGDirectionLeft);
	}
	textfile->SelectEverything();
	textfile->SetXOffset(0);
}

void SimpleTextField::SetValidInput(bool valid) {
	valid_input = valid;
}

void SimpleTextField::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = SimpleTextField::keybindings_noargs;
	noargs["cancel"] = [](Control* c) {
		SimpleTextField* t = (SimpleTextField*)c;
		if (t->on_user_cancel.function) {
			t->on_user_cancel.function(t, t->on_user_cancel.data);
		}
	};
	noargs["confirm"] = [](Control* c) {
		SimpleTextField* t = (SimpleTextField*)c;
		if (t->on_user_confirm.function) {
			t->on_user_confirm.function(t, t->on_user_confirm.data);
		}
	};
	noargs["prev_entry"] = [](Control* c) {
		SimpleTextField* t = (SimpleTextField*)c;
		if (t->on_prev_entry.function) {
			t->on_prev_entry.function(t, t->on_prev_entry.data);
		}
	};
	noargs["next_entry"] = [](Control* c) {
		SimpleTextField* t = (SimpleTextField*)c;
		if (t->on_next_entry.function) {
			t->on_next_entry.function(t, t->on_next_entry.data);
		}
	};
	std::map<std::string, PGKeyFunctionArgs>& args = SimpleTextField::keybindings_varargs;
	// FIXME: duplicate of BasicTextField::insert
	args["insert"] = [](Control* c, std::map<std::string, std::string> args) {
		SimpleTextField* tf = (SimpleTextField*)c;
		if (args.count("characters") == 0) {
			return;
		}
		tf->GetTextFile().PasteText(args["characters"]);
	};
}

void SimpleTextField::OnResize(PGSize old_size, PGSize new_size) {
	PGVerticalScroll scroll = textfile->GetLineOffset();
	lng rendered_lines = std::max(1LL, (lng)std::floor(new_size.height / GetTextHeight(textfield_font)));
	lng max_scroll = std::max(0LL, textfile->GetLineCount() - rendered_lines);
	if (scroll.linenumber > max_scroll) {
		textfile->SetLineOffset(PGVerticalScroll(max_scroll, 0));
	}
	BasicTextField::OnResize(old_size, new_size);
}

void SimpleTextField::SelectionChanged() {
	if (this->on_selection_changed.function) {
		this->on_selection_changed.function(this, this->on_selection_changed.data);
	}
	BasicTextField::SelectionChanged();
}

void SimpleTextField::TextChanged() {
	if (this->on_text_changed.function) {
		this->on_text_changed.function(this, this->on_text_changed.data);
	}
	BasicTextField::TextChanged();
}

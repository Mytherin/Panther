
#include "simpletextfield.h"
#include "style.h"

#include "container.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(SimpleTextField);

SimpleTextField::SimpleTextField(PGWindowHandle window) :
	BasicTextField(window, std::shared_ptr<TextFile>(new TextFile(nullptr))), valid_input(true),
	on_user_cancel(), on_user_confirm(), on_prev_entry(), on_next_entry(), render_background(true) {
	this->height = GetTextHeight(textfield_font) + 6;
}

SimpleTextField::~SimpleTextField() {
}

void SimpleTextField::Draw(PGRendererHandle renderer, PGIRect* rectangle) {
	PGScalar x = X() - rectangle->x;
	PGScalar y = Y() - rectangle->y;
	PGScalar max_x = x + this->width;
	PGScalar xoffset = textfile->GetXOffset();

	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height),
		valid_input ? PGStyleManager::GetColor(PGColorTextFieldCaret) : PGStyleManager::GetColor(PGColorTextFieldError),
		PGDrawStyleStroke);

	x += 4;
	y += 2;
	
	Cursor& cursor = textfile->GetCursors()[0];
	SetTextColor(textfield_font, PGStyleManager::GetColor(PGColorTextFieldText));
	PGScalar line_height = GetTextHeight(textfield_font);
	TextLine textline = textfile->GetLine(0);
	char* line = textline.GetLine();
	lng length = textline.GetLength();

	lng render_start = 0, render_end = length;
	auto character_widths = CumulativeCharacterWidths(textfield_font, line, length, xoffset, this->width, render_start, render_end);
	if (character_widths.size() == 0) {
		if (render_start == 0 && render_end == 0) {
			character_widths.push_back(0);
		} else {
			// the entire line is out of bounds, nothing to render
			return;
		}
	}

	auto begin_pos = cursor.BeginPosition();
	auto end_pos = cursor.EndPosition();

	if (begin_pos.position != end_pos.position) {
		RenderSelection(renderer, 
			textfield_font, 
			line,
			length, 
			x,
			y,
			begin_pos.position,
			end_pos.position,
			render_start,
			render_end,
			character_widths,
			PGStyleManager::GetColor(PGColorTextFieldSelection));
	}
	if (display_carets) {
		RenderCaret(renderer,
			textfield_font,
			panther::clamped_access<PGScalar>(character_widths, cursor.SelectedPosition().position - render_start),
			x,
			y,
			PGStyleManager::GetColor(PGColorTextFieldCaret));
	}

	RenderText(renderer, textfield_font, line + render_start, render_end - render_start, x + character_widths[0], y, max_x);
}

bool SimpleTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(SimpleTextField::keybindings, button, modifier)) {
		return true;
	}
	return BasicTextField::KeyboardButton(button, modifier);
}

std::string SimpleTextField::GetText() {
	TextLine line = textfile->GetLine(0);
	return std::string(line.GetLine(), line.GetLength());
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
}

void SimpleTextField::SelectionChanged() {
	if (this->on_selection_changed.function) {
		this->on_selection_changed.function(this, this->on_selection_changed.data);
	}
}

void SimpleTextField::TextChanged() {
	if (this->on_text_changed.function) {
		this->on_text_changed.function(this, this->on_text_changed.data);
	}
}

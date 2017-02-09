
#include "simpletextfield.h"
#include "style.h"

#include "container.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(SimpleTextField);

SimpleTextField::SimpleTextField(PGWindowHandle window) :
	BasicTextField(window, new TextFile(nullptr)), valid_input(true), 
	on_user_cancel(), on_user_confirm(), on_prev_entry(), on_next_entry() {
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
	
	Cursor* cursor = textfile->GetCursors()[0];
	SetTextColor(textfield_font, PGStyleManager::GetColor(PGColorTextFieldText));
	PGScalar line_height = GetTextHeight(textfield_font);
	TextLine textline = textfile->GetLine(0);
	char* line = textline.GetLine();
	lng length = textline.GetLength();

	lng render_start = 0, render_end = length;
	auto character_widths = CumulativeCharacterWidths(textfield_font, line, length, xoffset, this->width, render_start, render_end);
	if (character_widths.size() == 0) {
		// the entire line is out of bounds, nothing to render
		return;
	}

	auto begin_pos = cursor->BeginPosition();
	auto end_pos = cursor->EndPosition();

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
			panther::clamped_access<PGScalar>(character_widths, cursor->SelectedPosition().position - render_start),
			x,
			y,
			PGStyleManager::GetColor(PGColorTextFieldCaret));
	}

	RenderText(renderer, textfield_font, line + render_start, render_end - render_start, x + character_widths[0], y, max_x);
}

void SimpleTextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);

	if (button == PGLeftMouseButton) {
		if (drag_type == PGDragSelectionCursors) return;
		drag_type = PGDragSelection;
		lng line = 0, character = 0;
		_GetCharacterFromPosition(mouse.x, textfile->GetLine(0), character);

		PerformMouseClick(mouse);

		if (modifier == PGModifierNone && last_click.clicks == 0) {
			textfile->SetCursorLocation(line, character);
		} else if (modifier == PGModifierShift) {
			textfile->GetActiveCursor()->SetCursorStartLocation(line, character);
		} else if (last_click.clicks == 1) {
			textfile->SetCursorLocation(line, character);
			textfile->GetActiveCursor()->SelectWord();
		} else if (last_click.clicks == 2) {
			textfile->SetCursorLocation(line, character);
			textfile->GetActiveCursor()->SelectLine();
		}
		this->Invalidate();
	}
}

void SimpleTextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);

	if (button & PGLeftMouseButton) {
		if (drag_type != PGDragSelectionCursors) {
			drag_type = PGDragNone;
			this->Invalidate();
		}
	} else if (button & PGRightMouseButton) {
		if (!(mouse.x <= this->width && mouse.y <= this->height)) return;
		PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
		PGPopupMenuInsertEntry(menu, PGPopupInformation("Copy", "Ctrl+C"), [](Control* control, PGPopupInformation* info) {
			SetClipboardText(control->window, dynamic_cast<SimpleTextField*>(control)->textfile->CopyText());
		});
		PGPopupMenuInsertEntry(menu, "Cut", nullptr, PGPopupMenuGrayed);
		PGPopupMenuInsertEntry(menu, "Paste", [](Control* control, PGPopupInformation* info) {
			std::string clipboard_text = GetClipboardText(control->window);
			dynamic_cast<SimpleTextField*>(control)->textfile->PasteText(clipboard_text);
		});
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertEntry(menu, "Select All", [](Control* control, PGPopupInformation* info) {
			dynamic_cast<SimpleTextField*>(control)->textfile->SelectEverything();
			control->Invalidate();
		});
		PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
	}
}

void SimpleTextField::MouseMove(int x, int y, PGMouseButton buttons) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);

	if (buttons & PGLeftMouseButton) {
		if (drag_type == PGDragSelection) {
			lng character;
			_GetCharacterFromPosition(mouse.x, textfile->GetLine(0), character);
			Cursor* active_cursor = textfile->GetActiveCursor();
			if (active_cursor->SelectedCharacterPosition().character != character) {
				active_cursor->SetCursorStartLocation(0, character);
				Cursor::NormalizeCursors(textfile, textfile->GetCursors());
				this->Invalidate();
			}
		}
	} else {
		drag_type = PGDragNone;
	}
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
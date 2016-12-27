
#include "simpletextfield.h"
#include "style.h"

#include "container.h"

SimpleTextField::SimpleTextField(PGWindowHandle window) :
	BasicTextField(window, new TextFile(nullptr)), valid_input(true) {
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
	TextLine* line = textfile->GetLine(0);
	RenderSelection(renderer, 
		textfield_font, 
		line->GetLine(), 
		line->GetLength(), 
		x - xoffset, 
		y, cursor->BeginPosition(), 
		cursor->EndPosition(), 
		PGStyleManager::GetColor(PGColorTextFieldSelection), 
		max_x);
	if (display_carets) {
		RenderCaret(renderer, 
			textfield_font, 
			line->GetLine(),
			line->GetLength(),
			x - xoffset,
			y,
			cursor->SelectedPosition(),
			line_height,
			PGStyleManager::GetColor(PGColorTextFieldCaret));
	}

	RenderText(renderer, textfield_font, line->GetLine(), line->GetLength(), x - xoffset, y, max_x);
}

void SimpleTextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (!textfile->IsLoaded()) return;
	PGPoint mouse(x - this->x, y - this->y);

	if (button == PGLeftMouseButton) {
		if (drag_type == PGDragSelectionCursors) return;
		drag_type = PGDragSelection;
		lng line = 0, character = 0;
		GetCharacterFromPosition(mouse.x, textfile->GetLine(0), character);

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
		PGPopupMenuInsertEntry(menu, "Copy", [](Control* control) {
			SetClipboardText(control->window, dynamic_cast<SimpleTextField*>(control)->textfile->CopyText());
		});
		PGPopupMenuInsertEntry(menu, "Cut", nullptr, PGPopupMenuGrayed);
		PGPopupMenuInsertEntry(menu, "Paste", [](Control* control) {
			dynamic_cast<SimpleTextField*>(control)->textfile->PasteText(GetClipboardText(control->window));
		});
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertEntry(menu, "Select All", [](Control* control) {
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
			GetCharacterFromPosition(mouse.x, textfile->GetLine(0), character);
			Cursor* active_cursor = textfile->GetActiveCursor();
			if (active_cursor->start_character != character) {
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
	switch (button) {
	case PGButtonEscape:
		if (on_user_cancel)
			on_user_cancel(this, on_user_cancel_data);
		return true;
	case PGButtonEnter:
		if (on_successful_exit)
			on_successful_exit(this, on_successful_exit_data);
		return true;
	}
	return BasicTextField::KeyboardButton(button, modifier);
}

std::string SimpleTextField::GetText() {
	return textfile->GetLine(0)->GetString();
}

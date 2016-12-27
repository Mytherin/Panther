
#include "basictextfield.h"
#include "style.h"

#include "container.h"

BasicTextField::BasicTextField(PGWindowHandle window, TextFile* textfile) :
	Control(window), textfile(textfile) {
	if (textfile)
		textfile->SetTextField(this);

	drag_type = PGDragNone;

	textfield_font = PGCreateFont();
	SetTextFontSize(textfield_font, 15);
}

BasicTextField::~BasicTextField() {
	delete textfile;
}

void BasicTextField::PeriodicRender(void) {
	if (!WindowHasFocus(window) || (this->parent && this->parent->GetActiveControl() != this)) {
		display_carets = false;
		display_carets_count = 0;
		if (!current_focus) {
			current_focus = true;
			this->Invalidate();
		}
		return;
	} else if (current_focus) {
		if (current_focus) {
			current_focus = false;
			this->Invalidate();
		}
	}

	display_carets_count++;
	if (display_carets_count % FLICKER_CARET_INTERVAL == 0) {
		display_carets_count = 0;
		display_carets = !display_carets;
		this->Invalidate();
	}

	if (!textfile->IsLoaded()) {
		this->Invalidate();
	}
}

bool BasicTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	switch (button) {

	case PGButtonLeft:
		if (modifier == PGModifierNone) {
			textfile->OffsetCharacter(PGDirectionLeft);
		} else if (modifier == PGModifierShift) {
			textfile->OffsetSelectionCharacter(PGDirectionLeft);
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetWord(PGDirectionLeft);
		} else if (modifier == PGModifierCtrlShift) {
			textfile->OffsetSelectionWord(PGDirectionLeft);
		} else {
			return false;
		}
		return true;
	case PGButtonRight:
		if (modifier == PGModifierNone) {
			textfile->OffsetCharacter(PGDirectionRight);
		} else if (modifier == PGModifierShift) {
			textfile->OffsetSelectionCharacter(PGDirectionRight);
		} else if (modifier == PGModifierCtrl) {
			textfile->OffsetWord(PGDirectionRight);
		} else if (modifier == PGModifierCtrlShift) {
			textfile->OffsetSelectionWord(PGDirectionRight);
		} else {
			return false;
		}
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
		} else {
			return false;
		}
		return true;
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
		return true;
	case PGButtonBackspace:
		if (modifier == PGModifierNone || modifier == PGModifierShift) {
			this->textfile->DeleteCharacter(PGDirectionLeft);
		} else if (modifier == PGModifierCtrl) {
			this->textfile->DeleteWord(PGDirectionLeft);
		} else if (modifier == PGModifierCtrlShift) {
			this->textfile->DeleteLine(PGDirectionLeft);
		} else {
			return false;
		}
		return true;
	}
	return false;
}

bool BasicTextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!textfile->IsLoaded()) return false;

	if (modifier == PGModifierNone) {
		this->textfile->InsertText(character);
		return true;
	} else if (modifier & PGModifierCtrl) {
		switch (character) {
		case 'Z':
			this->textfile->Undo();
			return true;
		case 'Y':
			this->textfile->Redo();
			return true;
		case 'A':
			this->textfile->SelectEverything();
			return true;
		case 'C': {
			std::string text = textfile->CopyText();
			SetClipboardText(window, text);
			return true;
		}
		case 'V': {
			if (modifier & PGModifierShift) {
				// FIXME: cycle through previous copy-pastes
			} else {
				textfile->PasteText(GetClipboardText(window));
			}
			return true;
		}
		}
	}
	return false;
}

bool BasicTextField::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	if (!textfile->IsLoaded()) return false;

	if (modifier == PGModifierNone) {
		this->textfile->InsertText(character);
		return true;
	}
	return false;
}

PGCursorType BasicTextField::GetCursor(PGPoint mouse) {
	return PGCursorIBeam;
}

void BasicTextField::GetCharacterFromPosition(PGScalar x, TextLine* line, lng& character) {
	if (!line) {
		character = 0;
		return;
	}
	x -= text_offset;
	x += textfile->GetXOffset();
	char* text = line->GetLine();
	lng length = line->GetLength();
	character = GetPositionInLine(textfield_font, x, text, length);
}

void BasicTextField::GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character) {
	GetLineFromPosition(y, line);
	GetCharacterFromPosition(x, textfile->GetLine(line), character);
}

void BasicTextField::GetLineFromPosition(PGScalar y, lng& line) {
	// find the line position of the mouse
	lng lineoffset_y = textfile->GetLineOffset();
	lng line_offset = std::max(std::min((lng)(y / GetTextHeight(textfield_font)), textfile->GetLineCount() - lineoffset_y - 1), (lng)0);
	line = lineoffset_y + line_offset;
}

void BasicTextField::RefreshCursors() {
	// FIXME: thread safety on setting display_carets_count?
	display_carets_count = 0;
	display_carets = true;
}

int BasicTextField::GetLineHeight() {
	return (int)(this->height / GetTextHeight(textfield_font));
}

void BasicTextField::PerformMouseClick(PGPoint mouse) {
	time_t time = GetTime();
	if (time - last_click.time < DOUBLE_CLICK_TIME &&
		panther::abs(mouse.x - last_click.x) < 2 &&
		panther::abs(mouse.y - last_click.y) < 2) {
		last_click.clicks = last_click.clicks == 2 ? 0 : last_click.clicks + 1;
	} else {
		last_click.clicks = 0;
	}
	last_click.time = time;
	last_click.x = mouse.x;
	last_click.y = mouse.y;
}

PGScalar BasicTextField::GetMaxXOffset() {
	PGScalar max_character_width = MeasureTextWidth(textfield_font, "W", 1);
	PGScalar max_textsize = textfile->GetMaxLineWidth() * max_character_width;
	return std::max(max_textsize - GetTextfieldWidth() + text_offset, 0.0f);
}

void BasicTextField::SelectionChanged() {
	for (auto it = selection_changed_callbacks.begin(); it != selection_changed_callbacks.end(); it++) {
		(it->first)(this, it->second);
	}
	this->Invalidate();
}

void BasicTextField::TextChanged() {
	for (auto it = text_changed_callbacks.begin(); it != text_changed_callbacks.end(); it++) {
		(it->first)(this, it->second);
	}
	this->Invalidate();
}

void BasicTextField::TextChanged(std::vector<TextLine*> lines) {
	this->TextChanged();
}


void BasicTextField::OnSelectionChanged(PGControlDataCallback callback, void* data) {
	this->selection_changed_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void BasicTextField::OnTextChanged(PGControlDataCallback callback, void* data) {
	this->text_changed_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}




#include "basictextfield.h"
#include "style.h"

#include "controlmanager.h"
#include "container.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(BasicTextField);

BasicTextField::BasicTextField(PGWindowHandle window, std::shared_ptr<TextFile> textfile) :
	Control(window), textfile(textfile), prev_loaded(false) {
	if (textfile) {
		textfile->SetTextField(this);
	}

	drag_type = PGDragNone;

	textfield_font = PGCreateFont();
	SetTextFontSize(textfield_font, 15);
}

BasicTextField::~BasicTextField() {
}

void BasicTextField::PeriodicRender(void) {
	bool loaded = textfile->IsLoaded();
	if (loaded != prev_loaded || !loaded) {
		this->Invalidate();
		prev_loaded = loaded;
	}
	if (!WindowHasFocus(window) || !ControlHasFocus()) {
		display_carets = false;
		display_carets_count = 0;
		if (!current_focus) {
			current_focus = true;
			this->InvalidateTextField();
		}
		return;
	} else if (current_focus) {
		if (current_focus) {
			current_focus = false;
			this->InvalidateTextField();
		}
	}

	display_carets_count++;
	if (display_carets_count % FLICKER_CARET_INTERVAL == 0) {
		display_carets_count = 0;
		display_carets = !display_carets;
		this->InvalidateTextField();
	}
}

bool BasicTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(BasicTextField::keybindings, button, modifier)) {
		return true;
	}
	return false;
}

bool BasicTextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!textfile->IsLoaded()) return false;
	if (modifier == PGModifierNone) {
		this->textfile->InsertText(character);
		return true;
	} else if (this->PressCharacter(BasicTextField::keybindings, character, modifier)) {
		return true;
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

void BasicTextField::_GetCharacterFromPosition(PGScalar x, TextLine line, lng& character) {
	if (!line.IsValid()) {
		character = 0;
		return;
	}
	x -= text_offset;
	x += textfile->GetXOffset();
	char* text = line.GetLine();
	lng length = line.GetLength();
	character = GetPositionInLine(textfield_font, x, text, length);
}

void BasicTextField::GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character) {
	GetLineFromPosition(y, line);
	_GetCharacterFromPosition(x, textfile->GetLine(line), character);
}

void BasicTextField::GetLineFromPosition(PGScalar y, lng& line) {
	// find the line position of the mouse
	lng lineoffset_y = textfile->GetLineOffset().linenumber;
	lng line_offset = std::max(std::min((lng)(y / GetTextHeight(textfield_font)), textfile->GetLineCount() - lineoffset_y - 1), (lng)0);
	line = lineoffset_y + line_offset;
}

void BasicTextField::GetPositionFromLineCharacter(lng line, lng pos, PGScalar& x, PGScalar& y) {
	GetPositionFromLine(line, y);
	_GetPositionFromCharacter(pos, textfile->GetLine(line), x);
}

void BasicTextField::GetPositionFromLine(lng line, PGScalar& y) {
	lng lineoffset_y = textfile->GetLineOffset().linenumber;
	y = (line - lineoffset_y) * GetTextHeight(textfield_font);
}

void BasicTextField::_GetPositionFromCharacter(lng pos, TextLine line, PGScalar& x) {
	if (!line.IsValid()) {
		x = 0;
		return;
	}
	x = MeasureTextWidth(textfield_font, line.GetLine(), pos);
	x += text_offset;
	x -= textfile->GetXOffset();
}

void BasicTextField::RefreshCursors() {
	display_carets_count = 0;
	display_carets = true;
}

int BasicTextField::GetLineHeight() {
	return (int)(GetTextfieldHeight() / GetTextHeight(textfield_font)) - 1;
}

PGScalar BasicTextField::GetMaxXOffset() {
	if (textfile->GetWordWrap()) return 0;
	PGScalar max_textsize = textfile->GetMaxLineWidth(textfield_font);
	return std::max(max_textsize - GetTextfieldWidth() + text_offset, 0.0f);
}

void BasicTextField::SelectionChanged() {
	this->RefreshCursors();
	GetControlManager(this)->SelectionChanged(this);
	this->Invalidate();
}

void BasicTextField::TextChanged() {
	GetControlManager(this)->TextChanged(this);
	this->Invalidate();
}

void BasicTextField::PasteHistory() {
	std::vector<std::string> history = GetClipboardTextHistory();
	PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
	for (auto it = history.begin(); it != history.end(); it++) {
		PGPopupInformation info(menu);
		info.text = *it;
		if (info.text.size() > 20) {
			info.text.substr(0, 20);
		}
		panther::replace(info.text, "\n", "\\n");
		info.data = *it;
		PGPopupMenuInsertEntry(menu, info, [](Control* c, PGPopupInformation* info) {
			dynamic_cast<BasicTextField*>(c)->textfile->PasteText(info->data);
		});
	}
	Cursor& c = textfile->GetActiveCursor();
	PGScalar x, y;
	auto position = c.BeginPosition();
	GetPositionFromLineCharacter(position.line, position.position + 1, x, y);
	PGDisplayPopupMenu(menu, ConvertWindowToScreen(window,
		PGPoint(this->X() + x, this->Y() + y + GetTextHeight(textfield_font))),
		PGTextAlignLeft | PGTextAlignTop);
}


void BasicTextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (drag_type != PGDragNone && button == this->drag_button) {
		this->ClearDragging();
	}
}

void BasicTextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	lng line, character;
	GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
	if (this->PressMouseButton(BasicTextField::mousebindings, button, mouse, modifier, click_count, line, character)) {
		return;
	}
	return;
}

void BasicTextField::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (this->drag_type != PGDragNone) {
		if (buttons & drag_button) {
			if (drag_type == PGDragSelection) {
				// FIXME: when having multiple cursors and we are altering the active cursor,
				// the active cursor can never "consume" the other selections (they should always stay)
				// FIXME: this should be done in the textfile
				lng line, character;
				GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
				Cursor& active_cursor = textfile->GetActiveCursor();
				auto selected_pos = active_cursor.SelectedCharacterPosition();
				if (selected_pos.line != line || selected_pos.character != character) {
					lng old_line = selected_pos.line;
					active_cursor.SetCursorStartLocation(line, character);
					if (minimal_selections.count(textfile->GetActiveCursorIndex()) > 0) {
						active_cursor.ApplyMinimalSelection(minimal_selections[textfile->GetActiveCursorIndex()]);
					}
					Cursor::NormalizeCursors(textfile.get(), textfile->GetCursors());
				}
			}
		} else {
			this->ClearDragging();
			this->Invalidate();
		}
	}
}

void BasicTextField::StartDragging(PGMouseButton button, PGDragType drag_type) {
	if (this->drag_type == PGDragNone) {
		this->drag_button = button;
		this->drag_type = drag_type;
		auto cursors = textfile->GetCursors();
		for (lng i = 0; i < cursors.size(); i++) {
			minimal_selections[i] = cursors[i].GetCursorSelection();
		}
		this->Invalidate();
	}
}

void BasicTextField::ClearDragging() {
	minimal_selections.clear();
	drag_type = PGDragNone;
}

void BasicTextField::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = BasicTextField::keybindings_noargs;
	noargs["undo"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->Undo();
	};
	noargs["redo"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->Redo();
	};
	noargs["select_all"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->SelectEverything();
	};
	noargs["copy"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		std::string text = t->textfile->CopyText();
		SetClipboardText(t->window, text);
	};
	noargs["paste"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		std::string text = GetClipboardText(t->window);
		t->textfile->PasteText(text);
	};
	noargs["paste_from_history"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->PasteHistory();
	};
	noargs["cut"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		std::string text = t->textfile->CutText();
		SetClipboardText(t->window, text);
	};
	noargs["left_delete"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteCharacter(PGDirectionLeft);
	};
	noargs["left_delete_word"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteWord(PGDirectionLeft);
	};
	noargs["left_delete_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteLine(PGDirectionLeft);
	};
	noargs["right_delete"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteCharacter(PGDirectionRight);
	};
	noargs["right_delete_word"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteWord(PGDirectionRight);
	};
	noargs["right_delete_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteLine(PGDirectionRight);
	};
	noargs["delete_selected_lines"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->DeleteLines();
	};
	noargs["offset_start_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->OffsetStartOfLine();
	};
	noargs["select_start_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->SelectStartOfLine();
	};
	noargs["offset_end_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->OffsetEndOfLine();
	};
	noargs["select_end_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->SelectEndOfLine();
	};
	noargs["offset_start_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->OffsetStartOfFile();
	};
	noargs["select_start_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->SelectStartOfFile();
	};
	noargs["offset_end_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->OffsetEndOfFile();
	};
	noargs["select_end_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->textfile->SelectEndOfFile();
	};
	std::map<std::string, PGKeyFunctionArgs>& args = BasicTextField::keybindings_varargs;
	args["insert"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (args.count("characters") == 0) {
			return;
		}
		tf->GetTextFile().PasteText(args["characters"]);
	};
	args["offset_character"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (args.count("direction") == 0) {
			return;
		}
		bool word = args.count("word") != 0;
		bool selection = args.count("selection") != 0;
		PGDirection direction = args["direction"] == "left" ? PGDirectionLeft : PGDirectionRight;
		if (word) {
			if (selection) {
				tf->textfile->OffsetSelectionWord(direction);
			} else {
				tf->textfile->OffsetWord(direction);
			}
		} else {
			if (selection) {
				tf->textfile->OffsetSelectionCharacter(direction);
			} else {
				tf->textfile->OffsetCharacter(direction);
			}
		}
	};
	std::map<std::string, PGMouseFunction>& mouse_bindings = BasicTextField::mousebindings_noargs;
	mouse_bindings["add_cursor"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->GetTextFile().AddNewCursor(line, character);
		tf->StartDragging(button, PGDragSelection);
	};
	mouse_bindings["set_cursor_location"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->GetTextFile().SetCursorLocation(line, character);
		tf->StartDragging(button, PGDragSelection);
	};
	mouse_bindings["set_cursor_selection"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->GetTextFile().GetActiveCursor().SetCursorStartLocation(line, character);
		tf->StartDragging(button, PGDragSelection);
	};
	mouse_bindings["select_word"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->GetTextFile().SetCursorLocation(line, character);
		tf->GetTextFile().GetActiveCursor().SelectWord();
		tf->StartDragging(button, PGDragSelection);
	};
	mouse_bindings["select_line"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->GetTextFile().SetCursorLocation(line, character);
		tf->GetTextFile().GetActiveCursor().SelectLine();
		tf->StartDragging(button, PGDragSelection);
	};
}

void BasicTextField::InvalidateTextField() {
	Control::Invalidate(true);
}

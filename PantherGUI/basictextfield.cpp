
#include "basictextfield.h"
#include "style.h"

#include "container.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(BasicTextField);

BasicTextField::BasicTextField(PGWindowHandle window, TextFile* textfile) :
	Control(window), textfile(textfile), prev_loaded(false) {
	if (textfile)
		textfile->SetTextField(this);

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
}

bool BasicTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(BasicTextField::keybindings, button, modifier)) {
		return true;
	}
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
	default:
		return false;
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
	y =  (line - lineoffset_y) * GetTextHeight(textfield_font);
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
	if (textfile->GetWordWrap()) return 0;
	PGScalar max_textsize = textfile->GetMaxLineWidth(textfield_font);
	return std::max(max_textsize - GetTextfieldWidth() + text_offset, 0.0f);
}

void BasicTextField::SelectionChanged() {
	for (auto it = selection_changed_callbacks.begin(); it != selection_changed_callbacks.end(); it++) {
		(it->first)(this, it->second);
	}
	this->RefreshCursors();
	this->Invalidate();
}

void BasicTextField::TextChanged() {
	for (auto it = text_changed_callbacks.begin(); it != text_changed_callbacks.end(); it++) {
		(it->first)(this, it->second);
	}
	this->Invalidate();
}

void BasicTextField::TextChanged(std::vector<lng> lines) {
	this->TextChanged();
}


void BasicTextField::OnSelectionChanged(PGControlDataCallback callback, void* data) {
	this->selection_changed_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void BasicTextField::OnTextChanged(PGControlDataCallback callback, void* data) {
	this->text_changed_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void BasicTextField::UnregisterOnTextChanged(PGControlDataCallback callback, void* data) {
	for (lng i = 0; i < text_changed_callbacks.size(); i++) {
		if (text_changed_callbacks[i].first == callback && text_changed_callbacks[i].second == data) {
			text_changed_callbacks.erase(text_changed_callbacks.begin() + i);
			return;
		}
	}
	assert(0);
}

void BasicTextField::PasteHistory() {
	std::vector<std::string> history = GetClipboardTextHistory();
	PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
	for (auto it = history.begin(); it != history.end(); it ++) {
		PGPopupInformation info;
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
	std::map<std::string, PGKeyFunctionArgs>& args = BasicTextField::keybindings_varargs;
	args["insert"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (args.count("characters") == 0) {
			return;
		}
		tf->GetTextFile().PasteText(args["characters"]);
	};
}

#include "textfield.h"
#include <sstream>
#include <algorithm>

//"E:\\Github Projects\\Tibialyzer\\Database Scan\\tibiawiki_pages_current.xml"
//"C:\\Users\\wieis\\Desktop\\syntaxtest.py"
TextField::TextField(PGWindowHandle window) : 
	Control(window), textfile("C:\\Users\\wieis\\Desktop\\syntaxtest.py") {
	RegisterRefresh(window, this);
	cursors.push_back(Cursor(&textfile));
}

void TextField::Draw(PGRendererHandle renderer, PGRect* rectangle) {
	// FIXME: get line height from renderer without drawing
	// FIXME: mouse = caret over textfield
	// FIXME: draw background of textfield

	SetTextFont(renderer, NULL);
	int position_x = this->x + this->offset_x + 5;
	int position_y = 0;
	ssize_t linenr = lineoffset_y;
	TextLine *current_line;

	SetTextColor(renderer, PGColor(0, 0, 255));
	while ((current_line = textfile.GetLine(linenr)) != NULL) {
		if (position_y > rectangle->y + rectangle->height) break;
		if (!(position_y + line_height < rectangle->y)) {
			PGSize size = RenderText(renderer, current_line->GetLine(), current_line->GetLength(), position_x, position_y);
			line_height = size.height;
		}
		linenr++;
		position_y += line_height;
	}
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		ssize_t startline = std::max(it->BeginLine(), lineoffset_y);
		ssize_t endline = std::min(it->EndLine(), linenr);
		position_y = (startline - lineoffset_y) * line_height;
		for (; startline <= endline; startline++) {
			current_line = textfile.GetLine(startline);
			assert(current_line);
			ssize_t start, end;
			if (startline == it->BeginLine()) {
				if (startline == it->EndLine()) {
					// start and end are on the same line
					start = it->BeginCharacter();
					end = it->EndCharacter();
				} else {
					start = it->BeginCharacter();
					end = current_line->GetLength() + 1;
				}
			} else if (startline == it->EndLine()) {
				start = 0;
				end = it->EndCharacter();
			} else {
				start = 0;
				end = current_line->GetLength() + 1;
			}

			if (startline == it->SelectedLine()) {
				// render the caret on the selected line
				RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x, position_y, it->SelectedCharacter());
			}

			RenderSelection(renderer,
				current_line->GetLine(),
				current_line->GetLength(),
				position_x,
				position_y,
				start,
				end);

			position_y += line_height;
		}
		/*
		ssize_t characternr = it->SelectedCharacter();
		position_y = (linenr - lineoffset_y) * line_height;
		if (!(position_y + line_height < rectangle->y && position_y > rectangle->y + rectangle->height)) {
			current_line = textfile.GetLine(linenr);
			RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x, position_y, characternr);
		}*/
	}
}

void TextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void TextField::KeyboardButton(PGButton button, PGModifier modifier) {
	switch (button) {
		// FIXME: when moving up/down, count \t as multiple characters (equal to tab size)
	case PGButtonDown:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it).OffsetLine(1);
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionLine(1);
			} else if (modifier == PGModifierCtrl) {
				SetScrollOffset(this->lineoffset_y + 1);
			} else if (modifier == PGModifierCtrlShift) {
				// FIXME move line down
			}
		}
		Invalidate();
		break;
	case PGButtonUp:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it).OffsetLine(-1);
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionLine(-1);
			} else if (modifier == PGModifierCtrl) {
				SetScrollOffset(this->lineoffset_y - 1);
			} else if (modifier == PGModifierCtrlShift) {
				// FIXME move line up
			}
		}
		Invalidate();
		break;
	case PGButtonLeft:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				if (it->SelectionIsEmpty()) {
					(*it).OffsetCharacter(-1);
				} else {
					it->SetCursorLocation(it->BeginLine(), it->BeginCharacter());
				}
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionCharacter(-1);
			} else if (modifier == PGModifierCtrl) {
				it->OffsetWord(PGDirectionLeft);
			} else if (modifier == PGModifierCtrlShift) {
				it->OffsetSelectionWord(PGDirectionLeft);
			}
		}
		Invalidate();
		break;
	case PGButtonRight:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				if (it->SelectionIsEmpty()) {
					(*it).OffsetCharacter(1);
				} else {
					it->SetCursorLocation(it->EndLine(), it->EndCharacter());
				}
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionCharacter(1);
			} else if (modifier == PGModifierCtrl) {
				it->OffsetWord(PGDirectionRight);
			} else if (modifier == PGModifierCtrlShift) {
				it->OffsetSelectionWord(PGDirectionRight);
			}
		}
		Invalidate();
		break;
	case PGButtonEnd:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it).OffsetEndOfLine();
			} else if (modifier == PGModifierShift) {
				(*it).SelectEndOfLine();
			} else if (modifier == PGModifierCtrl) {
				// FIXME go to end of file
			} else if (modifier == PGModifierCtrlShift) {
				// FIXME select until end of file
			}
		}
		Invalidate();
		break;
	case PGButtonHome:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it).OffsetStartOfLine();
			} else if (modifier == PGModifierShift) {
				(*it).SelectStartOfLine();
			} else if (modifier == PGModifierCtrl) {
				// FIXME go to start of file
			} else if (modifier == PGModifierCtrlShift) {
				// FIXME select until start of file
			}
		}
		Invalidate();
		break;
	case PGButtonPageUp:
		// FIXME: amount of lines in textfield
		if (modifier == PGModifierNone) {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetLine(-47);
			}
			Invalidate();
		}
		break;
	case PGButtonPageDown:
		// FIXME: amount of lines in textfield
		if (modifier == PGModifierNone) {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetLine(47);
			}
			Invalidate();
		}
		break;
	case PGButtonDelete:
		if (modifier == PGModifierNone) {
			this->textfile.DeleteCharacter(cursors, PGDirectionRight);
		} else if (modifier == PGModifierCtrl) {
			this->textfile.DeleteWord(cursors, PGDirectionRight);
		} else if (modifier == PGModifierShift) {
			//FIXME Shift+Delete = delete current line
		} else if (modifier == PGModifierCtrlShift) {
			//FIXME Shift+Backspace = normal backspace
		}
		this->Invalidate();
		break;
	case PGButtonBackspace:
		if (modifier == PGModifierNone || modifier == PGModifierShift) {
			this->textfile.DeleteCharacter(cursors, PGDirectionLeft);
		} else if (modifier == PGModifierCtrl) {
			this->textfile.DeleteWord(cursors, PGDirectionLeft);
		} else if (modifier == PGModifierCtrlShift) {
			// FIXME Ctrl+Shift+Backspace = delete everything on the line before the END SELECTED character (NOT the cursor)
		}
		this->Invalidate();
		break;
	case PGButtonEnter:
		if (modifier == PGModifierNone) {
			this->textfile.AddNewLine(cursors);
		} else if (modifier == PGModifierCtrl) {
			// FIXME: new line at end of every cursor's line
		} else if (modifier == PGModifierCtrlShift) {
			// FIXME: new line before every cursor's line
		}
		this->Invalidate();
	default:
		break;
	}
}

void TextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		this->textfile.InsertText(character, cursors);
		this->Invalidate();
	} else {
		if (modifier == PGModifierCtrl) {
			switch (character) {
			case 'Z':
				this->textfile.Undo();
				this->Invalidate();
				break;
			case 'Y':
				this->textfile.Redo();
				this->Invalidate();
				break;
			case 'A':
				this->cursors.clear();
				this->cursors.push_back(Cursor(&textfile, textfile.GetLineCount() - 1, textfile.GetLine(textfile.GetLineCount() - 1)->GetLength()));
				this->cursors.back().end_character = 0;
				this->cursors.back().end_line = 0;
				this->Invalidate();
				break;
			}
		}
	}
}

void TextField::KeyboardUnicode(char *character, PGModifier modifier) {
	// FIXME
}

void TextField::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		if (SetScrollOffset(this->lineoffset_y - (distance / 120) * 2)) {
			this->Invalidate();
		}
	}
}

bool 
TextField::SetScrollOffset(ssize_t offset) {
	ssize_t new_y = std::min(std::max(offset, (ssize_t)0), (ssize_t)(textfile.GetLineCount() - 1));
	if (new_y != this->lineoffset_y) {
		this->lineoffset_y = new_y;
		return true;
	}
	return false;
}

void TextField::InvalidateLine(int line) {
	this->Invalidate(PGRect(0, (line - this->lineoffset_y) * line_height, this->width, line_height));
}

void TextField::InvalidateBeforeLine(int line) {
	this->Invalidate(PGRect(0, 0, this->width, (line - this->lineoffset_y) * line_height));
}

void TextField::InvalidateAfterLine(int line) {
	this->Invalidate(PGRect(0, (line - this->lineoffset_y) * line_height, this->width, this->height));
}
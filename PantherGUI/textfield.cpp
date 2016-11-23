#include "textfield.h"
#include <sstream>
#include <algorithm>

//"E:\\Github Projects\\Tibialyzer\\Database Scan\\tibiawiki_pages_current.xml"
//"C:\\Users\\wieis\\Desktop\\syntaxtest.py"
TextField::TextField(PGWindowHandle window) : 
	Control(window), textfile("C:\\Users\\wieis\\Desktop\\syntaxtest.py") {
	RegisterRefresh(window, this);
	cursors.push_back(Cursor(&textfile));
	text_offset = 25;
}

void TextField::Draw(PGRendererHandle renderer, PGRect* rectangle) {
	// FIXME: get line height from renderer without drawing
	// FIXME: mouse = caret over textfield
	// FIXME: draw background of textfield

	// determine the width of the line numbers 
	ssize_t line_count = textfile.GetLineCount();
	auto line_number = std::to_string(std::max((ssize_t) 10, textfile.GetLineCount() + 1));
	text_offset = 10 + GetRenderWidth(renderer, line_number.c_str(), line_number.size());
	// determine the render position
	int position_x = this->x + this->offset_x;
	int position_x_text = position_x + text_offset;
	int position_y = 0;
	ssize_t linenr = lineoffset_y;
	TextLine *current_line;
	SetTextFont(renderer, NULL);
	SetTextColor(renderer, PGColor(0, 0, 255));
	while ((current_line = textfile.GetLine(linenr)) != NULL) {
		// only render lines that fall within the render rectangle
		if (this->offsets.size() <= linenr - lineoffset_y) {
			this->offsets.push_back(std::vector<short>());
		}
		if (position_y > rectangle->y + rectangle->height) break;
		if (!(position_y + line_height < rectangle->y)) {
			// determine render offsets for mouse selection
			GetRenderOffsets(renderer, current_line->GetLine(), current_line->GetLength(), this->offsets[linenr - lineoffset_y]);
			// render the actual text
			PGSize size = RenderText(renderer, current_line->GetLine(), current_line->GetLength(), position_x_text, position_y);
			line_height = size.height;
			
			// render the line number
			auto line_number = std::to_string(linenr + 1);
			SetTextAlign(renderer, PGTextAlignRight);
			RenderText(renderer, line_number.c_str(), line_number.size(), position_x, position_y);
		}

		linenr++;
		position_y += line_height;
	}
	// render the selection and carets
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
				RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x_text, position_y, it->SelectedCharacter());
			}

			RenderSelection(renderer,
				current_line->GetLine(),
				current_line->GetLength(),
				position_x_text,
				position_y,
				start,
				end);

			position_y += line_height;
		}
	}
}

void TextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
}

void TextField::GetLineCharacterFromPosition(int x, int y, ssize_t& line, ssize_t& character) {
	// find the line position of the mouse
	ssize_t line_offset = std::min((ssize_t) y / this->line_height, textfile.GetLineCount() - this->lineoffset_y - 1);
	line = this->lineoffset_y + line_offset;
	// find the character position within the line
	x = x - this->text_offset - this->offset_x;
	ssize_t width = 0;
	character = textfile.GetLine(line)->GetLength();
	for (int i = 0; i < this->offsets[line_offset].size(); i++) {
		if (width <= x && width + offsets[line_offset][i] > x) {
			character = i;
			break;
		}
		width += offsets[line_offset][i];
	}
}

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	x = x - this->x;
	y = y - this->y;
	if (button == PGLeftMouseButton) {
		ssize_t line, character;
		GetLineCharacterFromPosition(x, y, line, character);
		if (cursors.size() > 1) {
			cursors.erase(cursors.begin() + 1, cursors.end());
		}
		cursors[0].SetCursorLocation(line, character);
		this->Invalidate();
	}
}

void TextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void TextField::MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void TextField::MouseMove(int x, int y, PGMouseButton buttons) {
	if (buttons & PGLeftMouseButton) {
		ssize_t line, character;
		GetLineCharacterFromPosition(x, y, line, character);
		if (cursors[0].start_line != line || cursors[0].start_character != character) {
			ssize_t old_line = cursors[0].start_line;
			cursors[0].SetCursorStartLocation(line, character);
			this->InvalidateBetweenLines(old_line, cursors[0].start_line);
		}
	}
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

void TextField::InvalidateBetweenLines(int start, int end) {
	if (start > end) {
		InvalidateBetweenLines(end, start);
		return;
	}
	this->Invalidate(PGRect(0, (start - this->lineoffset_y) * line_height, this->width, 
		(end - this->lineoffset_y) * line_height - (start - this->lineoffset_y) * line_height + line_height));
}

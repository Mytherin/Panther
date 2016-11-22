
#include "textfield.h"
#include <sstream>
#include <algorithm>


TextField::TextField(PGWindowHandle window) : Control(window), textfile("C:\\Users\\wieis\\Desktop\\syntaxtest.py") {
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
		ssize_t startline = std::max(std::min(it->SelectedLine(), it->EndLine()), lineoffset_y);
		ssize_t endline = std::min(std::max(it->SelectedLine(), it->EndLine()), linenr);
		position_y = (startline - lineoffset_y) * line_height;
		for (; startline <= endline; startline++) {
			current_line = textfile.GetLine(startline);
			assert(current_line);
			ssize_t start, end;
			if (startline == it->SelectedLine()) {
				if (startline == it->EndLine()) {
					// start and end are on the same line
					start = std::min(it->SelectedCharacter(), it->EndCharacter());
					end = std::max(it->SelectedCharacter(), it->EndCharacter());
				} else if (it->EndLine() < it->SelectedLine()) {
					// start occurs after end
					start = 0;
					end = it->SelectedCharacter();
				} else {
					// end occurs after start
					start = it->SelectedCharacter();
					end = current_line->GetLength();
				}
				RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x, position_y, it->SelectedCharacter());
			} else if (startline == it->EndLine()) {
				if (it->EndLine() < it->SelectedLine()) {
					// start occurs after end
					start = it->EndCharacter();
					end = current_line->GetLength();
				} else {
					// end occurs after start
					start = 0;
					end = it->EndCharacter();
				}
			} else {
				start = 0;
				end = current_line->GetLength();
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
		// FIXME: Up/Down on last line = move to first (or last) character
	case PGButtonDown:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it).OffsetLine(1);
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionLine(1);
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
					if (it->SelectedLine() < it->EndLine() ||
						(it->SelectedLine() == it->EndLine() && it->SelectedCharacter() < it->EndCharacter())) {
						it->SetCursorLocation(it->SelectedLine(), it->SelectedCharacter());
					} else {
						it->SetCursorLocation(it->EndLine(), it->EndCharacter());
					}
				}
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionCharacter(-1);
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
					if (it->SelectedLine() > it->EndLine() ||
						(it->SelectedLine() == it->EndLine() && it->SelectedCharacter() > it->EndCharacter())) {
						it->SetCursorLocation(it->SelectedLine(), it->SelectedCharacter());
					} else {
						it->SetCursorLocation(it->EndLine(), it->EndCharacter());
					}
				}
			} else if (modifier == PGModifierShift) {
				(*it).OffsetSelectionCharacter(1);
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
		// FIXME
		/*
		if (this->selected_line == this->textfile.GetLineCount() - 1 &&
			this->selected_character == textfile.GetLine(this->textfile.GetLineCount() - 1)->GetLength()) break;
		if (this->selected_character == textfile.GetLine(this->selected_line)->GetLength()) {
			this->textfile.DeleteCharacter(this->selected_line + 1, 0);
			this->InvalidateAfterLine(selected_line);
		}
		else {
			this->textfile.DeleteCharacter(this->selected_line, this->selected_character + 1);
			this->InvalidateLine(selected_line);
		}*/
		break;
	case PGButtonBackspace:
		if (modifier == PGModifierNone) {
			this->textfile.DeleteCharacter(cursors);
			this->Invalidate();
		}
		break;
	case PGButtonEnter:
		if (modifier == PGModifierNone) {
			this->textfile.AddNewLine(cursors);
			this->Invalidate();
		} else if (modifier == PGModifierCtrl) {
			// FIXME: new line at end of every cursor's line
		}
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
				// FIXME: adjust selected line/character after Undo
				this->textfile.Undo();
				this->Invalidate();
				break;
			case 'Y':
				// FIXME: adjust selected line/character after Redo
				this->textfile.Redo();
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
		size_t new_y = std::min(std::max(this->lineoffset_y - (distance / 120) * 2, (ssize_t)0), (ssize_t)(textfile.GetLineCount() - 1));
		if (new_y != this->lineoffset_y) {
			this->lineoffset_y = new_y;
			this->Invalidate();
		}
	}
}

void TextField::InvalidateLine(int line) {
	// FIXME: compute line rectangle
	this->Invalidate(PGRect(0, (line - this->lineoffset_y) * line_height, this->width, line_height));
}

void TextField::InvalidateBeforeLine(int line) {
	// FIXME: 
}

void TextField::InvalidateAfterLine(int line) {
	this->Invalidate(PGRect(0, (line - this->lineoffset_y) * line_height, this->width, this->height));
}

#include "textfield.h"
#include <sstream>
#include <algorithm>


TextField::TextField(PGWindowHandle window) : Control(window), textfile("C:\\Users\\wieis\\Desktop\\syntaxtest.py") {
	RegisterRefresh(window, this);
	cursors.push_back(Cursor(&textfile));
}

void TextField::Draw(PGRendererHandle renderer, PGRect* rectangle) {
	// FIXME: get line height from renderer without drawing
	// FIXME: draw caret
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
		ssize_t linenr = it->SelectedLine();
		ssize_t characternr = it->SelectedCharacter();
		position_y = (linenr - lineoffset_y) * line_height;
		if (!(position_y + line_height < rectangle->y && position_y > rectangle->y + rectangle->height)) {
			current_line = textfile.GetLine(linenr);
			RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x, position_y, characternr);
		}
	}
}

void TextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void TextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		switch (button) {
			// FIXME: when moving up/down, count \t as multiple characters (equal to tab size)
			// FIXME: Up/Down on last line = move to first (or last) character
		case PGButtonDown:
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetLine(1);
			}
			Invalidate();
			/*
			this->selected_line = std::min(this->selected_line + 1, textfile.GetLineCount() - 1);
			this->selected_character = std::min(this->selected_character, (ssize_t) textfile.GetLine(this->selected_line)->GetLength());*/
			break;
		case PGButtonUp:
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetLine(-1);
			}
			Invalidate();
			//this->selected_line = std::max(this->selected_line - 1, (ssize_t) 0);
			break;
		case PGButtonLeft:
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetCharacter(-1);
			}
			Invalidate();
			break;
		case PGButtonRight:
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetCharacter(1);
			}
			Invalidate();
			break;
		case PGButtonEnd:
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).SelectEndOfLine();
			}
						Invalidate();
			break;
		case PGButtonHome:
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).SelectStartOfLine();
			}
			Invalidate();
			break;
		case PGButtonPageUp:
			// FIXME: amount of lines in textfield
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetLine(-47);
			}
			Invalidate();
			break;
		case PGButtonPageDown:
			// FIXME: amount of lines in textfield
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it).OffsetLine(47);
			}
			Invalidate();
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
			this->textfile.DeleteCharacter(cursors);
			this->Invalidate();
			/*
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
			}
			this->textfile.DeleteCharacter(this->selected_line, this->selected_character);
			if (this->selected_character == 0) {
				this->selected_line--;
				this->selected_character = this->textfile.GetLine(this->selected_line)->GetLength();
				this->InvalidateAfterLine(selected_line);
			}
			else {
				this->selected_character--;
				this->InvalidateLine(selected_line);
			}*/
			break;
		case PGButtonEnter:
			this->textfile.AddNewLine(cursors);
			this->Invalidate();
			/*
			this->selected_line++;
			this->selected_character = 0;
			this->InvalidateAfterLine(this->selected_line - 1);*/
		default:
			break;
		}
	}
}

void TextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		this->textfile.InsertText(character, cursors);
		this->Invalidate();
	}
	else {
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
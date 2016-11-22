
#include "cursor.h"
#include "textfile.h"

#include <algorithm>

Cursor::Cursor(TextFile* file) : 
	file(file), start_line(0), start_character(0), end_line(0), end_character(0) {

}

Cursor::Cursor(TextFile* file, ssize_t line, ssize_t character) : 
	file(file), start_line(line), start_character(character), end_line(line), end_character(character) {
}

void Cursor::OffsetLine(ssize_t offset) {
	OffsetSelectionLine(offset);
	end_line = start_line;
	end_character = start_character;
}

void Cursor::OffsetSelectionLine(ssize_t offset) {
	start_line = std::min(std::max(start_line + offset, (ssize_t)0), this->file->GetLineCount() - 1);
	start_character = std::min(start_character, file->GetLine(start_line)->GetLength());
}

void Cursor::OffsetCharacter(ssize_t offset) {
	OffsetSelectionCharacter(offset);
	end_character = start_character;
	end_line = start_line;
}

void Cursor::OffsetSelectionCharacter(ssize_t offset) {
	start_character += offset;
	while (start_character < 0) {
		if (start_line > 0) {
			start_line--;
			start_character += this->file->GetLine(start_line)->GetLength() + 1;
		}
		else {
			start_character = 0;
			break;
		}
	}
	while (start_character > this->file->GetLine(start_line)->GetLength()) {
		if (start_line < this->file->GetLineCount() - 1) {
			start_character -= this->file->GetLine(start_line)->GetLength() + 1;
			start_line++;
		}
		else {
			start_character = this->file->GetLine(start_line)->GetLength() - 1;
			break;
		}
	}
}

void Cursor::OffsetStartOfLine() {
	// FIXME: offset to indentation first (i.e. right after initial tabs/spaces)
	SetCursorLocation(this->start_line, 0);
}

void Cursor::OffsetEndOfLine() {
	SetCursorLocation(this->start_line, this->file->GetLine(start_line)->GetLength());
}

void Cursor::SelectStartOfLine() {
	// FIXME: offset to indentation first (i.e. right after initial tabs/spaces)
	this->start_character = 0;
}

void Cursor::SelectEndOfLine() {
	this->start_character = this->file->GetLine(end_line)->GetLength();
}

void Cursor::SetCursorLocation(int linenr, int characternr) {
	this->start_character = this->end_character = characternr;
	this->start_line = this->end_line = linenr;
}

void Cursor::SetCursorLine(int linenr) {
	this->start_line = this->end_line = linenr;
}

void Cursor::SetCursorCharacter(int characternr) {
	this->start_character = this->end_character = characternr;
}

void Cursor::RestoreCursor(Cursor cursor) {
	this->start_line = cursor.start_line;
	this->start_character = cursor.start_character;
	this->end_line = cursor.end_line;
	this->end_character = cursor.end_character;
}

bool Cursor::SelectionIsEmpty() {
	return this->start_character == this->end_character && this->start_line == this->end_line;
}

#include "cursor.h"
#include "textfile.h"
#include "text.h"

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
			start_character = this->file->GetLine(start_line)->GetLength();
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

void Cursor::OffsetSelectionWord(PGDirection direction) {
	if (direction == PGDirectionLeft && this->SelectedCharacter() == 0) {
		OffsetSelectionCharacter(-1);
	} else if (direction == PGDirectionRight && start_character == this->file->GetLine(start_line)->GetLength()) {
		OffsetSelectionCharacter(1);
	} else {
		TextLine* textline = this->file->GetLine(start_line);
		char* line = textline->GetLine();
		ssize_t length = textline->GetLength();
		int offset = direction == PGDirectionLeft ? -1 : 1;
		ssize_t index = direction == PGDirectionLeft ? start_character + offset : start_character;
		PGCharacterClass type = GetCharacterClass(line[index]);
		for (index += offset; index >= 0 && index < length; index += offset) {
			if (GetCharacterClass(line[index]) != type) {
				index -= direction == PGDirectionLeft ? offset : 0;
				break;
			}
		}
		index = std::min(std::max(index, (ssize_t)0), textline->GetLength());
		start_character = index;
	}
}

void Cursor::SelectWord() {
	TextLine* textline = this->file->GetLine(start_line);
	char* line = textline->GetLine();
	ssize_t start_index = start_character - 1;
	ssize_t end_index = start_index + 1;
	PGCharacterClass type = GetCharacterClass(line[start_index]);
	for (end_index = start_index + 1; end_index < textline->GetLength(); end_index++) {
		if (GetCharacterClass(line[end_index]) != type) {
			break;
		}
	}
	for (start_index--; start_index >= 0; start_index--) {
		if (GetCharacterClass(line[start_index]) != type) {
			start_index++;
			break;
		}
	}
	end_character = std::max(start_index, (ssize_t) 0);
	start_character = std::min(end_index, textline->GetLength());
	end_line = start_line;
}

void Cursor::OffsetWord(PGDirection direction) {
	OffsetSelectionWord(direction);
	end_character = start_character;
	end_line = start_line;
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

void Cursor::SetCursorStartLocation(int linenr, int characternr) {
	this->start_character = characternr;
	this->start_line = linenr;
}

void Cursor::SetCursorEndLocation(int linenr, int characternr) {
	this->end_character = characternr;
	this->end_line = linenr;
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

ssize_t Cursor::BeginCharacter() {
	if (start_line < end_line) return start_character;
	if (end_line < start_line) return end_character;
	return std::min(start_character, end_character);
}

ssize_t Cursor::BeginLine() {
	return std::min(start_line, end_line);
}

ssize_t Cursor::EndCharacter() {
	if (start_line < end_line) return end_character;
	if (end_line < start_line) return start_character;
	return std::max(start_character, end_character);
}

ssize_t Cursor::EndLine() {
	return std::max(start_line, end_line);
}

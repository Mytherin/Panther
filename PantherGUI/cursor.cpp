
#include "cursor.h"
#include "textfile.h"

#include <algorithm>

Cursor::Cursor(TextFile* file) : 
	file(file), selected_line(0), selected_character(0) {

}

Cursor::Cursor(TextFile* file, ssize_t line, ssize_t character) : 
	file(file), selected_line(line), selected_character(character) {
}

void Cursor::OffsetLine(ssize_t offset) {
	selected_line = std::min(std::max(selected_line + offset, (ssize_t)0), this->file->GetLineCount() - 1);
	selected_character = std::min(selected_character, file->GetLine(selected_line)->GetLength());
}

void Cursor::OffsetCharacter(ssize_t offset) {
	selected_character += offset;
	while (selected_character < 0) {
		if (selected_line > 0) {
			selected_line--;
			selected_character += this->file->GetLine(selected_line)->GetLength() + 1;
		}
		else {
			selected_character = 0;
			break;
		}
	}
	while (selected_character > this->file->GetLine(selected_line)->GetLength()) {
		if (selected_line < this->file->GetLineCount() - 1) {
			selected_character -= this->file->GetLine(selected_line)->GetLength() + 1;
			selected_line++;
		}
		else {
			selected_character = this->file->GetLine(selected_line)->GetLength() - 1;
			break;
		}
	}

}

void Cursor::SelectStartOfLine() {
	this->selected_character = 0;
}

void Cursor::SelectEndOfLine() {
	this->selected_character = this->file->GetLine(selected_line)->GetLength();
}

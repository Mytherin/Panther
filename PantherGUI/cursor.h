#pragma once

#include "utils.h"

class TextFile;

class Cursor {
	friend class TextFile;
public:
	void OffsetLine(ssize_t offset);
	void OffsetCharacter(ssize_t offset);
	void SelectStartOfLine();
	void SelectEndOfLine();

	ssize_t SelectedCharacter() { return selected_character; }
	ssize_t SelectedLine() { return selected_line; }

	Cursor(TextFile* file);
	Cursor(TextFile* file, ssize_t line, ssize_t character);
private:
	TextFile* file;
	ssize_t selected_line;
	ssize_t selected_character;
};

#pragma once

#include "utils.h"

class TextFile;

class Cursor {
	friend class TextFile;
	friend class TextField;
public:
	void OffsetLine(ssize_t offset);
	void OffsetCharacter(ssize_t offset);
	void OffsetSelectionLine(ssize_t offset);
	void OffsetSelectionCharacter(ssize_t offset);
	void OffsetStartOfLine();
	void OffsetEndOfLine();
	void SelectStartOfLine();
	void SelectEndOfLine();

	ssize_t SelectedCharacter() { return start_character; }
	ssize_t SelectedLine() { return start_line; }
	ssize_t EndCharacter() { return end_character; }
	ssize_t EndLine() { return end_line; }

	bool SelectionIsEmpty();

	void RestoreCursor(Cursor cursor);

	Cursor(TextFile* file);
	Cursor(TextFile* file, ssize_t line, ssize_t character);
private:
	TextFile* file;
	ssize_t start_line;
	ssize_t start_character;
	ssize_t end_line;
	ssize_t end_character;

	void SetCursorLocation(int linenr, int characternr);
	void SetCursorLine(int linenr);
	void SetCursorCharacter(int characternr);
};

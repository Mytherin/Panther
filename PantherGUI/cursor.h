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
	void OffsetWord(PGDirection direction);
	void OffsetSelectionWord(PGDirection direction);
	void SelectWord();
	void SelectLine();

	ssize_t SelectedCharacter() { return start_character; }
	ssize_t SelectedLine() { return start_line; }
	ssize_t BeginCharacter();
	ssize_t BeginLine();
	ssize_t EndCharacter();
	ssize_t EndLine();

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
	ssize_t min_character;
	ssize_t min_line;
	ssize_t max_character;
	ssize_t max_line;

	void SetCursorStartLocation(int linenr, int characternr);
	void SetCursorEndLocation(int linenr, int characternr);
	void SetCursorLocation(int linenr, int characternr);
	void SetCursorLine(int linenr);
	void SetCursorCharacter(int characternr);
};

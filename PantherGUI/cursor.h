#pragma once

#include <string>
#include "utils.h"

#include <vector>

class TextFile;
class TextField;

class Cursor {
	friend class TextFile;
	friend class TextField;
public:
	void OffsetLine(lng offset);
	void OffsetCharacter(lng offset);
	void OffsetSelectionLine(lng offset);
	void OffsetSelectionCharacter(lng offset);
	void OffsetStartOfLine();
	void OffsetEndOfLine();
	void SelectStartOfLine();
	void SelectEndOfLine();
	void OffsetStartOfFile();
	void OffsetEndOfFile();
	void SelectStartOfFile();
	void SelectEndOfFile();
	void OffsetWord(PGDirection direction);
	void OffsetSelectionWord(PGDirection direction);
	void SelectWord();
	void SelectLine();
	int GetSelectedWord(lng& word_start, lng& word_end);

	lng SelectedCharacter() { return start_character; }
	lng SelectedLine() { return start_line; }
	lng BeginCharacter();
	lng BeginLine();
	lng EndCharacter();
	lng EndLine();

	std::string GetText();

	bool SelectionIsEmpty();

	void RestoreCursor(Cursor cursor);
	bool Contains(lng linenr, lng characternr);
	bool OverlapsWith(Cursor& cursor);
	void Merge(Cursor& cursor);

	static void NormalizeCursors(TextField* textfield, std::vector<Cursor*>& cursors, bool scroll_textfield = true);
	static bool CursorOccursFirst (Cursor* a, Cursor* b) { return (a->BeginLine() < b->BeginLine() || (a->BeginLine() == b->BeginLine() && a->BeginCharacter() < b->BeginCharacter())); }
	static void VerifyCursors(TextField* textfield, std::vector<Cursor*>& cursors);

	void SetCursorStartLocation(lng linenr, lng characternr);
	void SetCursorEndLocation(lng linenr, lng characternr);
	void SetCursorLocation(lng linenr, lng characternr);
	void SetCursorLine(lng linenr);
	void SetCursorCharacter(lng characternr);

	Cursor(TextFile* file);
	Cursor(TextFile* file, lng line, lng character);
private:
	TextFile* file;
	lng start_line;
	lng start_character;
	lng end_line;
	lng end_character;
	lng min_character;
	lng min_line;
	lng max_character;
	lng max_line;
};

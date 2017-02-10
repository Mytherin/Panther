#pragma once

#include <string>
#include "utils.h"
#include "textbuffer.h"
#include "textposition.h"

#include <vector>
#include "windowfunctions.h"

class TextFile;
class TextField;

struct CursorData {
	lng start_line;
	lng start_position;
	lng end_line;
	lng end_position;

	CursorData() :
		start_line(0), start_position(0), end_line(0), end_position(0) { 
	}

	CursorData(lng start_line, lng start_position, lng end_line, lng end_position) :
		start_line(start_line), start_position(start_position), end_line(end_line), end_position(end_position) {
	}
};

class Cursor {
	friend class TextFile;
public:
	Cursor(TextFile* file);
	Cursor(TextFile* file, lng start_line, lng start_character);
	Cursor(TextFile* file, lng start_line, lng start_character, lng end_line, lng end_character);
	Cursor(TextFile* file, CursorData data);
	Cursor(TextFile* file, PGTextRange range);

	static std::vector<CursorData> GetCursorData(std::vector<Cursor*> cursors);

	CursorData GetCursorData();
	PGTextRange GetCursorSelection();

	void OffsetLine(lng offset);
	void OffsetCharacter(PGDirection direction);
	void OffsetPosition(lng offset);
	void OffsetSelectionLine(lng offset);
	void OffsetSelectionCharacter(PGDirection direction);
	void OffsetSelectionPosition(lng offset);
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
	std::string GetSelectedWord();

	PGCursorPosition UnselectedPosition();
	PGCursorPosition SelectedPosition();
	PGCursorPosition BeginPosition();
	PGCursorPosition EndPosition();

	PGCharacterPosition SelectedCharacterPosition();
	PGCharacterPosition BeginCharacterPosition();
	PGCharacterPosition EndCharacterPosition();

	PGScalar GetXOffset(PGTextPosition position);
	PGScalar BeginXPosition();
	PGScalar EndXPosition();
	PGScalar SelectedXPosition();

	std::string GetLine();
	std::string GetText();

	bool SelectionIsEmpty();

	bool OverlapsWith(Cursor* cursor);
	void Merge(Cursor* cursor);

	static void NormalizeCursors(TextFile* textfile, std::vector<Cursor*>& cursors, bool scroll_textfield = true);
	static bool CursorOccursFirst(Cursor* a, Cursor* b);

	static void LoadCursors(nlohmann::json& j, std::vector<CursorData>& stored_cursors);
	static void StoreCursors(nlohmann::json& j, std::vector<CursorData>& cursors);

	void SetCursorStartLocation(lng linenr, lng characternr);
	void SetCursorLocation(lng linenr, lng characternr);
	void SetCursorLocation(PGTextRange range);

	void ApplyMinimalSelection(PGTextRange selection);

	PGTextPosition BeginCursorPosition() const;
	PGTextPosition SelectedCursorPosition() const;
	PGTextPosition EndCursorPosition() const;
private:
	void _SetCursorStartLocation(lng linenr, lng characternr);

	static bool CursorPositionOccursFirst(PGTextBuffer* a, lng a_pos, PGTextBuffer* b, lng b_pos);

	TextFile* file;
	PGTextBuffer* start_buffer;
	lng start_buffer_position;
	PGTextBuffer* end_buffer;
	lng end_buffer_position;

	// helper functions so you can loop over both the start_buffer and end_buffer
	PGTextBuffer*& BUF(int i) { assert(i == 0 || i == 1); return i == 0 ? start_buffer : end_buffer; }
	lng& BUFPOS(int i) { assert(i == 0 || i == 1); return i == 0 ? start_buffer_position : end_buffer_position; }

	PGScalar x_position;
};

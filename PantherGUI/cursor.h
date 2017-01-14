#pragma once

#include <string>
#include "utils.h"
#include "textbuffer.h"

#include <vector>
#include "windowfunctions.h"

class TextFile;
class TextField;
struct CursorPosition;

struct CursorPosition {
	PGTextBuffer* buffer;
	lng position;

	friend bool operator< (const CursorPosition& lhs, const CursorPosition& rhs) {
		return (lhs.buffer->start_line < rhs.buffer->start_line ||
			(lhs.buffer->start_line == rhs.buffer->start_line && lhs.position < rhs.position));
	}
	friend bool operator> (const CursorPosition& lhs, const CursorPosition& rhs){ return rhs < lhs; }
	friend bool operator<=(const CursorPosition& lhs, const CursorPosition& rhs){ return !(lhs > rhs); }
	friend bool operator>=(const CursorPosition& lhs, const CursorPosition& rhs){ return !(lhs < rhs); }

	friend bool operator==(const CursorPosition& lhs, const CursorPosition& rhs) {
		return (lhs.buffer->start_line == rhs.buffer->start_line && lhs.position == rhs.position);
	}
	friend bool operator!=(const CursorPosition& lhs, const CursorPosition& rhs){ return !(lhs == rhs); }

	CursorPosition(PGTextBuffer* buffer, lng position) : buffer(buffer), position(position) { }
};

struct CursorData {
	lng start_line;
	lng start_position;
	lng end_line;
	lng end_position;

	CursorData(lng start_line, lng start_position, lng end_line, lng end_position) :
		start_line(start_line), start_position(start_position), end_line(end_line), end_position(end_position) {
	}
};

struct CursorSelection {
	CursorPosition begin;
	CursorPosition end;

	CursorSelection(PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position) :
		begin(start_buffer, start_position), end(end_buffer, end_position) { }
	CursorSelection() : 
		begin(nullptr, 0), end(nullptr, 0) { }
};

class Cursor {
	friend class TextFile;
public:
	Cursor(TextFile* file);
	Cursor(TextFile* file, lng start_line, lng start_character);
	Cursor(TextFile* file, lng start_line, lng start_character, lng end_line, lng end_character);
	Cursor(TextFile* file, CursorData data);

	CursorData GetCursorData();
	CursorSelection GetCursorSelection();

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

	PGScalar SelectedXPosition();

	std::string GetLine();
	std::string GetText();

	bool SelectionIsEmpty();

	bool OverlapsWith(Cursor* cursor);
	void Merge(Cursor* cursor);

	static void NormalizeCursors(TextFile* textfile, std::vector<Cursor*>& cursors, bool scroll_textfield = true);
	static bool CursorOccursFirst(Cursor* a, Cursor* b);

	void SetCursorStartLocation(lng linenr, lng characternr);
	void SetCursorLocation(lng linenr, lng characternr);

	void ApplyMinimalSelection(CursorSelection selection);
private:
	static bool CursorPositionOccursFirst(PGTextBuffer* a, lng a_pos, PGTextBuffer* b, lng b_pos);
	CursorPosition BeginCursorPosition();
	CursorPosition EndCursorPosition();

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

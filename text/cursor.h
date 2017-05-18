#pragma once

#include <string>
#include "utils.h"
#include "textbuffer.h"
#include "textposition.h"

#include <vector>
#include "windowfunctions.h"

class TextView;
class TextField;

class Cursor {
	friend class TextView;
public:
	Cursor() : file(nullptr), x_position(-1), start_buffer(nullptr), start_buffer_position(-1), end_buffer(nullptr), end_buffer_position(-1) { }
	Cursor(TextView* file);
	Cursor(TextView* file, lng start_line, lng start_character);
	Cursor(TextView* file, lng start_line, lng start_character, lng end_line, lng end_character);
	Cursor(TextView* file, PGCursorRange data);
	Cursor(TextView* file, PGTextRange range);

	static std::vector<PGCursorRange> GetCursorData(const std::vector<Cursor>& cursors);

	PGCursorRange GetCursorData() const;
	PGTextRange GetCursorSelection() const;

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
	std::string GetSelectedWord() const;

	PGCursorPosition UnselectedPosition() const;
	PGCursorPosition SelectedPosition() const;
	PGCursorPosition BeginPosition() const;
	PGCursorPosition EndPosition() const;

	PGCharacterPosition SelectedCharacterPosition() const;
	PGCharacterPosition BeginCharacterPosition() const;
	PGCharacterPosition EndCharacterPosition() const;

	PGScalar GetXOffset(PGTextPosition position);
	PGScalar BeginXPosition();
	PGScalar EndXPosition();
	PGScalar SelectedXPosition();

	std::string GetLine();
	std::string GetText();

	bool SelectionIsEmpty() const;

	bool OverlapsWith(const Cursor& cursor) const;
	void Merge(const Cursor& cursor);

	static void NormalizeCursors(TextView* textfile, std::vector<Cursor>& cursors, bool scroll_textfield = true);
	static bool CursorOccursFirst(const Cursor& a, const Cursor& b);
	// returns the index of the first cursor that points to <buffer>
	// <cursors> is assumed to be sorted
	// returns cursors.size() if no cursor points to <buffer>
	static lng FindFirstCursorInBuffer(std::vector<Cursor>& cursors, PGTextBuffer* buffer);

	static void LoadCursors(nlohmann::json& j, std::vector<PGCursorRange>& stored_cursors);
	static void StoreCursors(nlohmann::json& j, std::vector<PGCursorRange>& cursors);

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

	TextView* file;
	PGTextBuffer* start_buffer;
	lng start_buffer_position;
	PGTextBuffer* end_buffer;
	lng end_buffer_position;

	// helper functions so you can loop over both the start_buffer and end_buffer
	PGTextBuffer*& BUF(int i) { assert(i == 0 || i == 1); return i == 0 ? start_buffer : end_buffer; }
	lng& BUFPOS(int i) { assert(i == 0 || i == 1); return i == 0 ? start_buffer_position : end_buffer_position; }

	PGScalar x_position;
};


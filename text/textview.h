#pragma once

#include "cursor.h"
#include "textfile.h"
#include "utils.h"

class BasicTextField;

struct PGTextViewSettings {
	double xoffset = -1;
	PGVerticalScroll yoffset = PGVerticalScroll(-1, -1);
	bool wordwrap;
	std::vector<PGCursorRange> cursor_data;

	PGTextViewSettings() : xoffset(-1), yoffset(-1, -1), wordwrap(false), cursor_data() {}
};

// a textview represents a view into a textfile
// a view includes a scroll position (xoffset, yoffset)
// edit information (cursors)
// visual information (wordwrap, displayed search matches)
struct TextView {
public:
	TextView(BasicTextField* field);

	std::shared_ptr<TextFile> file;

	bool finished_search = false;
	std::vector<PGTextRange> matches;
	lng selected_match = -1;

	lng xoffset = 0;
	PGVerticalScroll yoffset;

	bool wordwrap = false;
	PGScalar wordwrap_width;

	std::vector<Cursor> cursors;
	lng active_cursor;

	BasicTextField* textfield;

	TextLineIterator* GetScrollIterator(BasicTextField* textfield, PGVerticalScroll scroll);
	TextLineIterator* GetLineIterator(BasicTextField* textfield, lng linenumber);

	void ClearExtraCursors();
	void ClearCursors();
	void SetCursorLocation(lng line, lng character);
	void SetCursorLocation(lng start_line, lng start_character, lng end_line, lng end_character);
	void SetCursorLocation(PGTextRange range);
	void AddNewCursor(lng line, lng character);
	void SelectEverything();
	void OffsetLine(lng offset);
	void OffsetSelectionLine(lng offset);
	void OffsetCharacter(PGDirection);
	void OffsetSelectionCharacter(PGDirection);
	void OffsetWord(PGDirection);
	void OffsetSelectionWord(PGDirection);
	void OffsetStartOfLine();
	void SelectStartOfLine();
	void OffsetStartOfFile();
	void SelectStartOfFile();
	void OffsetEndOfLine();
	void SelectEndOfLine();
	void OffsetEndOfFile();
	void SelectEndOfFile();

	int GetLineHeight();

	bool SelectMatches(bool in_selection);
	bool FinishedSearch() { return finished_search; }

	lng GetMaxYScroll();
	PGScalar GetXOffset() { return (PGScalar)xoffset; }
	void SetXOffset(lng offset) { xoffset = offset; }
	PGVerticalScroll GetLineOffset();
	double GetScrollPercentage(PGVerticalScroll scroll);
	double GetScrollPercentage();
	void SetLineOffset(lng offset);
	void SetLineOffset(PGVerticalScroll scroll);
	void SetScrollOffset(lng offset);
	void OffsetLineOffset(double lines);
	PGVerticalScroll GetVerticalScroll(lng linenumber, lng characternr);
	PGVerticalScroll OffsetVerticalScroll(PGVerticalScroll scroll, double offset);
	PGVerticalScroll OffsetVerticalScroll(PGVerticalScroll scroll, double offset, lng& lines_offset);
	Cursor& GetActiveCursor();
	lng GetActiveCursorIndex();
	std::vector<Cursor>& GetCursors() { return cursors; }
	void SetTextField(BasicTextField* textfield) { this->textfield = textfield; }

	bool FindMatch(PGRegexHandle regex_handle, PGDirection direction, bool wrap, bool include_selection);
	void FindAllMatches(PGRegexHandle regex_handle, bool select_first_match, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap);
	void FindAllMatches(PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data);

	void RestoreCursors(std::vector<PGCursorRange>& data);
	Cursor RestoreCursor(PGCursorRange data);
	// same as RestoreCursor, but selections are replaced by the LOWEST value
	Cursor RestoreCursorPartial(PGCursorRange data);

	void ClearMatches();
	const std::vector<PGTextRange>& GetFindMatches() { return matches; }
	void SetSelectedMatch(lng selected_match) { this->selected_match = selected_match; }

	void SetWordWrap(bool wordwrap, PGScalar wrap_width);
	bool GetWordWrap() { return wordwrap; }

	void ApplySettings(PGTextViewSettings& settings);

	void InvalidateTextView();

	void VerifyTextView();
};
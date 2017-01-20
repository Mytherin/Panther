#pragma once

#include "cursor.h"
#include "encoding.h"
#include "textdelta.h"
#include "mmap.h"
#include "utils.h"
#include "scheduler.h"
#include "syntaxhighlighter.h"
#include "language.h"
#include "regex.h"

#include <string>
#include <vector>


#include "assert.h"

typedef enum {
	PGLineEndingWindows,
	PGLineEndingMacOS,
	PGLineEndingUnix,
	PGLineEndingMixed,
	PGLineEndingUnknown
} PGLineEnding;

struct RedoStruct {
	std::vector<CursorData> cursors;
	TextDelta* delta;

	RedoStruct(TextDelta* delta, std::vector<CursorData> cursors) : 
		delta(delta), cursors(cursors) { }
};

PGLineEnding GetSystemLineEnding();
char GetSystemPathSeparator();

typedef enum {
	PGIndentionSpaces,
	PGIndentionTabs,
	PGIndentionMixed
} PGLineIndentation;

class BasicTextField;
struct Interval;

struct PGBufferFindMatch {
	lng start_buffer_pos;
	lng end_buffer_pos;

	static bool MatchOccursFirst(PGBufferFindMatch a, PGBufferFindMatch b);

	PGBufferFindMatch() : start_buffer_pos(-1), end_buffer_pos(-1) { }
	PGBufferFindMatch(lng start_buffer_pos, lng end_buffer_pos) : 
		start_buffer_pos(start_buffer_pos), end_buffer_pos(end_buffer_pos) { }
};

struct PGFindMatch {
	lng start_character;
	lng start_line;
	lng end_character;
	lng end_line;

	static std::vector<PGFindMatch> FromBufferMatches(PGTextBuffer* buffer, std::vector<PGBufferFindMatch> matches);

	PGFindMatch(PGTextBuffer*, PGBufferFindMatch*);
	PGFindMatch() : start_character(-1), start_line(-1), end_character(-1), end_line(-1) { }
	PGFindMatch(lng start_character, lng start_line, lng end_character, lng end_line) : 
		start_character(start_character), start_line(start_line), end_character(end_character), end_line(end_line) { }
};

enum PGLockType {
	PGReadLock,
	PGWriteLock
};

class TextFile {
	friend class Cursor;
	friend class TextLineIterator;
	friend class WrappedTextLineIterator;
	friend class TextField;
	friend class TabControl;
public:
	// create an in-memory textfile with currently unspecified path
	TextFile(BasicTextField* textfield);
	~TextFile();

	static TextFile* OpenTextFile(BasicTextField* textfield, std::string filename, bool immediate_load = false);

	TextLineIterator* GetScrollIterator(TextField* textfield, lng scroll_offset);
	TextLineIterator* GetLineIterator(TextField* textfield, lng linenumber);
	TextLine GetLine(lng linenumber);
	void InsertText(char character);
	void InsertText(PGUTF8Character character);
	void InsertText(std::string text);
	void InsertLines(const std::vector<std::string>& lines);
	void InsertLines(std::vector<std::string> lines, size_t cursor);
	std::vector<std::string> SplitLines(const std::string& text);
	void DeleteCharacter(PGDirection direction);
	void DeleteWord(PGDirection direction);
	void AddNewLine();
	void AddNewLine(std::string text);
	void DeleteLines();
	void DeleteLine(PGDirection direction);
	void AddEmptyLine(PGDirection direction);
	void MoveLines(int offset);

	std::string CutText();
	std::string CopyText();
	void PasteText(std::string& text);

	void ChangeLineEnding(PGLineEnding lineending);
	void ChangeFileEncoding(PGFileEncoding encoding);
	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo();
	void Redo();

	void SaveChanges();
	void SaveAs(std::string path);

	PGFileEncoding GetFileEncoding() { return encoding; }
	PGLineEnding GetLineEnding() { return lineending; }
	PGLineIndentation GetLineIndentation() { return indentation; }
	PGLanguage* GetLanguage() { return language; }

	lng GetLineCount();
	lng GetMaxYScroll() { return wordwrap ? maxscroll : GetLineCount() - 1; }

	void Lock(PGLockType type);
	void Unlock(PGLockType type);

	bool IsLoaded() { return is_loaded; }
	double LoadPercentage() { return loaded; }

	void ClearExtraCursors();
	void ClearCursors();
	void SetCursorLocation(lng line, lng character);
	void SetCursorLocation(lng start_line, lng start_character, lng end_line, lng end_character);
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

	void SelectMatches();
	bool FinishedSearch() { return finished_search; }

	bool FindMatch(std::string text, PGDirection direction, char** error_message, bool match_case, bool wrap, bool regex, bool include_selection);
	void FindAllMatches(std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex);

	int GetLineHeight();

	void VerifyTextfile();

	std::vector<CursorData> BackupCursors();
	CursorData BackupCursor(int i);

	void RestoreCursors(std::vector<CursorData>& data);
	Cursor* RestoreCursor(CursorData data);
	// same as RestoreCursor, but selections are replaced by the LOWEST value
	Cursor* RestoreCursorPartial(CursorData data);

	lng GetLineFromScrollPosition(PGFontHandle font, PGScalar wrap_width, lng scroll_offset);
	lng GetScrollPositionFromLine(PGFontHandle font, PGScalar wrap_width, lng linenr);

	PGScalar GetMaxLineWidth(PGFontHandle font);
	PGScalar GetXOffset() { return (PGScalar) xoffset; }
	void SetXOffset(lng offset) { xoffset = offset; }
	lng GetLineOffset() { return (lng) yoffset; }
	void SetLineOffset(double offset);
	void OffsetLineOffset(double offset);
	Cursor*& GetActiveCursor();
	std::vector<Cursor*>& GetCursors() { return cursors; }
	void SetTextField(BasicTextField* textfield) { this->textfield = textfield; }
	std::string GetFullPath() { return path; }
	std::string GetName() { return name; }
	std::string GetExtension() { return ext; }
	bool HasUnsavedChanges() { return unsaved_changes; }
	bool FileInMemory() { return path.size() == 0; }

	void ClearMatches();
	const std::vector<PGFindMatch>& GetFindMatches() { return matches; }
	void SetSelectedMatch(lng selected_match) { selected_match = selected_match; }

	void AddRef() { refcount++; }
	bool DecRef() { return --refcount == 0; }

	void SetWordWrap(bool wordwrap, PGScalar wrap_width);
	bool GetWordWrap() { return wordwrap; }
private:
	// load textfile from a file
	TextFile(BasicTextField* textfield, std::string filename, char* base_data, lng size, bool immediate_load = false);

	// insert text at the specified cursor number, text must not include newlines
	void InsertText(std::string text, size_t cursornr);
	void DeleteCharacter(PGDirection direction, size_t i);

	void DeleteSelection(int cursornr);

	PGBufferFindMatch FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng begin_position, bool match_case, PGTextBuffer* buffer, std::string& line);
	PGBufferFindMatch FindMatch(std::string pattern, PGDirection direction, lng begin_position, bool match_case, PGTextBuffer* buffer, std::string& line);
	PGFindMatch FindMatch(std::string text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex, Task* current_task);
	PGFindMatch FindMatch(std::string text, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, char** error_message, bool match_case, bool wrap, bool regex, Task* current_task);

	bool finished_search = false;
	std::vector<PGFindMatch> matches;
	lng selected_match = -1;

	void SetUnsavedChanges(bool changes);

	bool unsaved_changes = false;
	bool pending_delete = false;

	std::string path;
	std::string name;
	std::string ext;
	
	BasicTextField* textfield;

	lng longest_line = 0;
	lng xoffset = 0;
	double yoffset = 0;
	lng current_linenumber = -1;

	void ClearCurrentLinenumber();

	bool wordwrap = false;
	PGScalar wordwrap_width;

	std::vector<Cursor*> cursors;
	Cursor* active_cursor;

	void OpenFile(char* base_data, lng size);

	void AddDelta(TextDelta* delta);
	void Undo(TextDelta* delta);

	void Undo(AddText& delta, int i);
	void Undo(RemoveText& delta, std::string& text, int i);

	void PerformOperation(TextDelta* delta);
	bool PerformOperation(TextDelta* delta, bool redo);
	std::vector<Interval> GetCursorIntervals();

	Task* current_task = nullptr;
	static void RunHighlighter(Task* task, TextFile* textfile);
	static void OpenFileAsync(Task* task, void* info);

	Task* find_task = nullptr;
	static void RunTextFinder(Task* task, TextFile* textfile, std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool match_case, bool wrap, bool regex);


	void InvalidateBuffer(PGTextBuffer* buffer);
	void InvalidateParsing();

	bool is_loaded;
	double loaded;

	PGTextBuffer* GetBuffer(lng line);

	lng linecount = 0;
	lng maxscroll = 0;
	lng max_line_length = 0;

	std::vector<PGScalar> line_lengths;
	std::vector<PGTextBuffer*> buffers;
	std::vector<TextDelta*> deltas;
	std::vector<RedoStruct> redos;
	PGLineEnding lineending;
	PGLineIndentation indentation;
	PGFileEncoding encoding;

	PGFontHandle default_font = nullptr;

	PGLanguage* language = nullptr;
	SyntaxHighlighter* highlighter = nullptr;

	PGMutexHandle text_lock;
	int shared_counter = 0;

	lng refcount = 0;
};

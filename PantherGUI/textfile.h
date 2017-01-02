#pragma once

#include "cursor.h"
#include "encoding.h"
#include "textdelta.h"
#include "mmap.h"
#include "utils.h"
#include "scheduler.h"
#include "syntaxhighlighter.h"
#include "language.h"

#include <string>
#include <vector>

typedef enum {
	PGLineEndingWindows,
	PGLineEndingMacOS,
	PGLineEndingUnix,
	PGLineEndingMixed,
	PGLineEndingUnknown
} PGLineEnding;

PGLineEnding GetSystemLineEnding();
char GetSystemPathSeparator();

typedef enum {
	PGIndentionSpaces,
	PGIndentionTabs,
	PGIndentionMixed
} PGLineIndentation;

class BasicTextField;
struct Interval;

struct PGFindMatch {
	lng start_character;
	lng start_line;
	lng end_character;
	lng end_line;

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
public:
	// create an in-memory textfile with currently unspecified path
	TextFile(BasicTextField* textfield);
	// load textfile from a file
	TextFile(BasicTextField* textfield, std::string filename, bool immediate_load = false);
	~TextFile();

	TextLineIterator GetIterator(lng linenumber);
	TextLine GetLine(lng linenumber);
	void InsertText(char character);
	void InsertText(PGUTF8Character character);
	void InsertText(std::string text);
	void InsertLines(std::vector<std::string>& lines);
	void InsertLines(std::vector<std::string>& lines, size_t cursor);
	void DeleteCharacter(PGDirection direction);
	void DeleteWord(PGDirection direction);
	void AddNewLine();
	void AddNewLine(std::string text);
	void AddNewLines(std::vector<std::string>& lines, bool first_is_newline);
	void DeleteLines();
	void DeleteLine(PGDirection direction);
	void AddEmptyLine(PGDirection direction);
	void MoveLines(int offset);

	std::string CopyText();
	void PasteText(std::string& text);

	void ChangeLineEnding(PGLineEnding lineending);
	void ChangeFileEncoding(PGFileEncoding encoding);
	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo();
	void Redo();

	void SaveChanges();

	PGFileEncoding GetFileEncoding() { return encoding; }
	PGLineEnding GetLineEnding() { return lineending; }
	PGLineIndentation GetLineIndentation() { return indentation; }
	PGLanguage* GetLanguage() { return language; }

	lng GetLineCount();

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
	bool FindMatch(std::string text, PGDirection direction, char** error_message, bool match_case, bool wrap, bool regex, lng& selected_match, bool include_selection);
	void FindAllMatches(std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex);

	void RefreshCursors();
	int GetLineHeight();

	void VerifyTextfile();

	std::vector<Cursor> BackupCursors();
	void RestoreCursors(std::vector<Cursor>& cursors);

	lng GetMaxLineWidth() { return longest_line; }
	void SetMaxLineWidth(lng new_width = -1);
	PGScalar GetXOffset() { return (PGScalar) xoffset; }
	void SetXOffset(lng offset) { xoffset = offset; }
	lng GetLineOffset() { return lineoffset_y; }
	void SetLineOffset(lng offset) { lineoffset_y = offset; }
	void OffsetLineOffset(lng offset);
	Cursor*& GetActiveCursor();
	std::vector<Cursor*>& GetCursors() { return cursors; }
	void SetTextField(BasicTextField* textfield) { this->textfield = textfield; }
	std::string GetFullPath() { return path; }
	std::string GetName() { return name; }
	bool HasUnsavedChanges() { return unsaved_changes; }
	bool FileInMemory() { return path.size() == 0; }

	void ClearMatches();
	const std::vector<PGFindMatch>& GetFindMatches() { return matches; }
private:
	// insert text at the specified cursor number, text must not include newlines
	void InsertText(std::string text, size_t cursornr);
	void DeleteCharacter(PGDirection direction, size_t i);

	void DeleteSelection(int cursornr);

	PGFindMatch FindMatch(std::string text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex, Task* current_task);

	bool finished_search = false;
	std::vector<PGFindMatch> matches;

	void SetUnsavedChanges(bool changes);

	bool unsaved_changes = false;
	bool pending_delete = false;

	std::string path;
	std::string name;
	std::string ext;
	
	BasicTextField* textfield;

	lng longest_line = 0;
	lng xoffset = 0;
	lng lineoffset_y = 0;
	std::vector<Cursor*> cursors;
	Cursor* active_cursor;

	void OpenFile(std::string filename);

	void AddDelta(TextDelta* delta);

	void PerformOperation(TextDelta* delta, bool adjust_delta = true);
	void PerformOperation(TextDelta* delta, std::vector<lng>& invalidated_lines, bool adjust_delta = true);
	std::vector<Interval> GetCursorIntervals();

	Task* current_task = nullptr;
	static void RunHighlighter(Task* task, TextFile* textfile);
	static void OpenFileAsync(Task* task, void* info);

	Task* find_task = nullptr;
	static void RunTextFinder(Task* task, TextFile* textfile, std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool match_case, bool wrap, bool regex);


	void InvalidateParsing(lng line);
	void InvalidateParsing(std::vector<lng>& lines);

	bool is_loaded;
	double loaded;

	PGTextBuffer* GetBuffer(lng line);

	lng linecount = 0;
	std::vector<PGTextBuffer*> buffers;
	std::vector<TextDelta*> deltas;
	std::vector<TextDelta*> redos;
	PGLineEnding lineending;
	PGLineIndentation indentation;
	PGFileEncoding encoding;

	PGLanguage* language = nullptr;
	SyntaxHighlighter* highlighter = nullptr;

	PGMutexHandle text_lock;
	int shared_counter = 0;
};

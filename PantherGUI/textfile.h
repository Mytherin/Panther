#pragma once

#include "cursor.h"
#include "encoding.h"
#include "textdelta.h"
#include "mmap.h"
#include "utils.h"
#include "textfile.h"
#include "textblock.h"
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

class TextFile {
	friend class Cursor;
public:
	// create an in-memory textfile with currently unspecified path
	TextFile(BasicTextField* textfield);
	// load textfile from a file
	TextFile(BasicTextField* textfield, std::string filename, bool immediate_load = false);
	~TextFile();

	TextLine* GetLine(lng linenumber);
	void InsertText(char character);
	void InsertText(PGUTF8Character character);
	void InsertText(std::string text);
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
	void PasteText(std::string text);

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

	void Lock();
	void Unlock();
	lng GetBlock(lng linenr) { return linenr / TEXTBLOCK_SIZE; }
	lng GetMaximumBlocks() { return lines.size() % TEXTBLOCK_SIZE == 0 ? GetBlock(lines.size()) : GetBlock(lines.size()) + 1; }
	bool BlockIsParsed(lng block) { return highlighter && parsed_blocks[block].parsed; }

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

	std::vector<PGFindMatch> matches;

	PGFindMatch FindMatch(std::string text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex);
	std::vector<PGFindMatch> FindAllMatches(std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex);

	void RefreshCursors();
	int GetLineHeight();

	std::vector<Cursor> BackupCursors();
	void RestoreCursors(std::vector<Cursor>& cursors);

	lng GetMaxLineWidth() { return longest_line; }
	void SetMaxLineWidth(lng new_width = -1);
	PGScalar GetXOffset() { return xoffset; }
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
private:
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

	void DeleteCharacter(MultipleDelta* delta, PGDirection direction);
	void DeleteCharacter(MultipleDelta* delta, Cursor* cursor, PGDirection direction, bool delete_selection, bool include_cursor = true);

	void OpenFile(std::string filename);

	void AddDelta(TextDelta* delta);

	void PerformOperation(TextDelta* delta, bool adjust_delta = true);
	void PerformOperation(TextDelta* delta, std::vector<lng>& invalidated_lines, bool adjust_delta = true);
	void Undo(TextDelta* delta, std::vector<lng>& invalidated_lines);
	void Redo(TextDelta* delta);
	std::vector<Interval> GetCursorIntervals();

	Task* current_task;
	static void RunHighlighter(Task* task, TextFile* textfile);
	static void OpenFileAsync(Task* task, void* info);

	void InvalidateParsing(lng line);
	void InvalidateParsing(std::vector<lng>& lines);

	bool is_loaded;
	double loaded;

	std::vector<TextLine*> lines;
	std::vector<TextDelta*> deltas;
	std::vector<TextDelta*> redos;
	std::vector<TextBlock> parsed_blocks;
	PGMutexHandle text_lock;
	PGLineEnding lineending;
	PGLineIndentation indentation;
	PGFileEncoding encoding;

	PGLanguage* language = nullptr;
	SyntaxHighlighter* highlighter = nullptr;
};

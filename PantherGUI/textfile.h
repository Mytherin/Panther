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
#include "textline.h"
#include "textiterator.h"

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

struct PGTextFileSettings {
	PGLineEnding line_ending = PGLineEndingUnknown;
	double xoffset = -1;
	PGVerticalScroll yoffset = PGVerticalScroll(-1, -1);
	bool wordwrap;
	std::vector<CursorData> cursor_data;

	PGTextFileSettings() : line_ending(PGLineEndingUnknown), xoffset(-1), yoffset(-1, -1), wordwrap(false), cursor_data() { }
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

	static TextFile* OpenTextFile(BasicTextField* textfield, std::string filename, PGFileError& error, bool immediate_load = false);

	TextLineIterator* GetScrollIterator(BasicTextField* textfield, PGVerticalScroll scroll);
	TextLineIterator* GetLineIterator(BasicTextField* textfield, lng linenumber);
	TextLine GetLine(lng linenumber);
	void InsertText(char character);
	void InsertText(PGUTF8Character character);
	void InsertText(std::string text);
	void InsertLines(const std::vector<std::string>& lines);
	void InsertLines(std::vector<std::string> lines, size_t cursor);
	bool SplitLines(const std::string& text, std::vector<std::string>&);
	std::vector<std::string> SplitLines(const std::string& text);
	void DeleteCharacter(PGDirection direction);
	void DeleteWord(PGDirection direction);
	void AddNewLine();
	void AddNewLine(std::string text);
	void DeleteLines();
	void DeleteLine(PGDirection direction);
	void AddEmptyLine(PGDirection direction);
	void MoveLines(int offset);

	std::string GetText();
	std::string CutText();
	std::string CopyText();
	void PasteText(std::string& text);

	bool Reload(PGFileError& error);

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
	lng GetMaxYScroll();

	void Lock(PGLockType type);
	void Unlock(PGLockType type);

	bool IsLoaded() { return is_loaded; }
	double LoadPercentage() { return (double) bytes / (double) total_bytes; }

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

	PGScalar GetMaxLineWidth(PGFontHandle font);
	PGScalar GetXOffset() { return (PGScalar) xoffset; }
	void SetXOffset(lng offset) { xoffset = offset; }
	PGVerticalScroll GetLineOffset();
	double GetScrollPercentage();
	void SetLineOffset(lng offset);
	void SetLineOffset(PGVerticalScroll scroll);
	void SetScrollOffset(lng offset);
	void OffsetLineOffset(lng lines);
	PGVerticalScroll GetVerticalScroll(lng linenumber, lng characternr);
	PGVerticalScroll OffsetVerticalScroll(PGVerticalScroll scroll, lng offset);
	PGVerticalScroll OffsetVerticalScroll(PGVerticalScroll scroll, lng offset, lng& lines_offset);
	Cursor*& GetActiveCursor();
	std::vector<Cursor*>& GetCursors() { return cursors; }
	void SetTextField(BasicTextField* textfield) { this->textfield = textfield; }
	std::string GetFullPath() { return path; }
	std::string GetName() { return name; }
	std::string GetExtension() { return ext; }

	enum PGStoreFileType {
		PGStoreFileBuffer,
		PGStoreFileDeltas,
		PGFileTooLarge
	};

	PGStoreFileType WorkspaceFileStorage();

	bool HasUnsavedChanges() { return unsaved_changes; }
	bool FileInMemory() { return path.size() == 0; }

	void ClearMatches();
	const std::vector<PGFindMatch>& GetFindMatches() { return matches; }
	void SetSelectedMatch(lng selected_match) { selected_match = selected_match; }

	void AddRef() { refcount++; }
	bool DecRef() { return --refcount == 0; }

	void SetWordWrap(bool wordwrap, PGScalar wrap_width);
	bool GetWordWrap() { return wordwrap; }

	void SetSettings(PGTextFileSettings settings);
private:
	// load textfile from a file
	TextFile(BasicTextField* textfield, std::string filename, char* base_data, lng size, bool immediate_load = false, bool delete_file = true);

	bool WriteToFile(PGFileHandle file, PGEncoderHandle encoder, const char* text, lng size, char** output_text, lng* output_size, char** intermediate_buffer, lng* intermediate_size);

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

	lng last_modified_time = -1;
	lng last_modified_notification = -1;
	bool last_modified_deletion = false;

	lng saved_undo_count = 0;

	std::string path;
	std::string name;
	std::string ext;
	
	BasicTextField* textfield;

	double total_width = 0;
	lng longest_line = 0;
	lng xoffset = 0;
	PGVerticalScroll yoffset;

	bool wordwrap = false;
	PGScalar wordwrap_width;

	std::vector<Cursor*> cursors;
	Cursor* active_cursor;

	void OpenFile(char* base_data, lng size, bool delete_file);

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
	static void RunTextFinder(Task* task, TextFile* textfile, PGRegexHandle regex_handle, lng start_line, lng start_character);

	void InvalidateBuffer(PGTextBuffer* buffer);
	void InvalidateBuffers();
	void InvalidateParsing();

	bool is_loaded;
	lng bytes = 0;
	lng total_bytes = 1;

	PGTextBuffer* GetBuffer(lng line);

	lng linecount = 0;
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

	void ApplySettings(PGTextFileSettings& settings);
	PGTextFileSettings settings;

	lng refcount = 0;
};

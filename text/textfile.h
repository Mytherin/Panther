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
	std::vector<PGCursorRange> cursors;
	std::unique_ptr<TextDelta> delta;

	RedoStruct(std::vector<PGCursorRange> cursors) :
		delta(nullptr), cursors(cursors) { }
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

enum PGLockType {
	PGReadLock,
	PGWriteLock
};

struct PGTextFileSettings {
	PGLineEnding line_ending = PGLineEndingUnknown;
	double xoffset = -1;
	PGVerticalScroll yoffset = PGVerticalScroll(-1, -1);
	bool wordwrap;
	std::vector<PGCursorRange> cursor_data;
	PGLanguage* language;
	PGFileEncoding encoding = PGEncodingUnknown;

	PGTextFileSettings() : line_ending(PGLineEndingUnknown), xoffset(-1), yoffset(-1, -1), wordwrap(false), cursor_data(), language(nullptr), encoding(PGEncodingUnknown) { }
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
	void InsertLines(std::string text, size_t cursor);
	void ReplaceText(std::string replacement_text, size_t i);
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
	void RegexReplace(PGRegexHandle regex, std::string& replacement);

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
	void SetLanguage(PGLanguage* language);

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

	void IndentText(PGDirection direction);

	bool SelectMatches(bool in_selection);
	bool FinishedSearch() { return finished_search; }

	bool FindMatch(PGRegexHandle regex_handle, PGDirection direction, bool wrap, bool include_selection);
	void FindAllMatches(PGRegexHandle regex_handle, bool select_first_match, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap);
	void FindAllMatches(PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data);
	int GetLineHeight();

	void VerifyPartialTextfile();
	void VerifyTextfile();

	std::vector<PGCursorRange> BackupCursors();
	PGCursorRange BackupCursor(int i);

	void RestoreCursors(std::vector<PGCursorRange>& data);
	Cursor RestoreCursor(PGCursorRange data);
	// same as RestoreCursor, but selections are replaced by the LOWEST value
	Cursor RestoreCursorPartial(PGCursorRange data);

	PGScalar GetMaxLineWidth(PGFontHandle font);
	PGScalar GetXOffset() { return (PGScalar) xoffset; }
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
	void SetFilePath(std::string path);
	std::string GetFullPath() { return path; }
	std::string GetName() { return name; }
	void SetName(std::string name) { this->name = name; }
	std::string GetExtension() { return ext; }
	void SetExtension(std::string extension) { this->ext = extension; }
	bool GetReadOnly() { return read_only; }
	void SetReadOnly(bool read_only) { this->read_only = read_only; }

	void UpdateModificationTime();

	void SetTabWidth(int tabwidth);
	int GetTabWidth() { return tabwidth; }

	enum PGStoreFileType {
		PGStoreFileBuffer,
		PGStoreFileDeltas,
		PGFileTooLarge
	};

	PGStoreFileType WorkspaceFileStorage();

	bool HasUnsavedChanges() { return FileInMemory() || unsaved_changes; }
	bool FileInMemory() { return path.size() == 0; }

	void ClearMatches();
	const std::vector<PGTextRange>& GetFindMatches() { return matches; }
	void SetSelectedMatch(lng selected_match) { this->selected_match = selected_match; }
	
	void SetWordWrap(bool wordwrap, PGScalar wrap_width);
	bool GetWordWrap() { return wordwrap; }

	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap, std::shared_ptr<Task> current_task);
	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap, std::shared_ptr<Task> current_task);

	void SetSettings(PGTextFileSettings settings);
	PGTextFileSettings GetSettings();

	// only used for "Find Results"
	void AddFindMatches(std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng start_line);
	std::string current_find_file;

	void ConvertToIndentation(PGLineIndentation indentation);

	void FindAllMatchesAsync(std::vector<PGFile>& files, PGRegexHandle regex_handle, int context_lines);
private:
	// load textfile from a file
	TextFile(BasicTextField* textfield, PGFileEncoding encoding, std::string filename, char* base_data, lng size, bool immediate_load = false, bool delete_file = true);

	bool WriteToFile(PGFileHandle file, PGEncoderHandle encoder, const char* text, lng size, char** output_text, lng* output_size, char** intermediate_buffer, lng* intermediate_size);

	// replace text in the specified text range with <replacement_text>
	// neither <range> nor <replacement_text> may not contain newlines
	void ReplaceText(PGTextRange range, std::string replacement_text);
	// insert text at the specified cursor number, text must not include newlines
	void InsertText(std::string text, size_t cursornr);
	// insert text at the specified position, text must not include newlines
	void InsertText(std::string text, PGTextBuffer* buffer, lng position);
	// delete the selection of the specified cursor number, cursor selection must not be empty
	void DeleteSelection(int cursornr);
	// delete the specified text range
	void DeleteText(PGTextRange);

	bool read_only = false;

	bool finished_search = false;
	std::vector<PGTextRange> matches;
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

	std::vector<Cursor> cursors;
	lng active_cursor;

	void OpenFile(char* base_data, lng size, bool delete_file);

	void AddDelta(TextDelta* delta);
	void Undo(TextDelta* delta);

	void Undo(PGReplaceText& delta, int i);
	void Undo(PGRegexReplace& delta, int i);
	void Undo(RemoveText& delta, std::string& text, int i);

	void PerformOperation(TextDelta* delta);
	bool PerformOperation(TextDelta* delta, bool redo);
	std::vector<Interval> GetCursorIntervals();

	std::shared_ptr<Task> current_task = nullptr;
	static void RunHighlighter(std::shared_ptr<Task> task, TextFile* textfile);
	static void OpenFileAsync(std::shared_ptr<Task> task, void* info);

	std::shared_ptr<Task> find_task = nullptr;
	static void RunTextFinder(std::shared_ptr<Task> task, TextFile* textfile, PGRegexHandle regex_handle, lng start_line, lng start_character, bool select_first_match);

	void InvalidateBuffer(PGTextBuffer* buffer);
	void InvalidateBuffers();
	void InvalidateParsing();

	bool reload_on_changed = true;
	bool is_loaded;
	lng bytes = 0;
	lng total_bytes = 1;

	PGTextBuffer* GetBuffer(lng line);

	lng linecount = 0;
	PGTextPosition max_line_length;

	std::vector<PGTextBuffer*> buffers;
	std::vector<std::unique_ptr<TextDelta>> deltas;
	std::vector<RedoStruct> redos;
	PGLineEnding lineending;
	PGLineIndentation indentation;
	PGFileEncoding encoding;
	int tabwidth = 0;
	
	PGFontHandle default_font = nullptr;

	PGLanguage* language = nullptr;
	std::unique_ptr<SyntaxHighlighter> highlighter = nullptr;

	std::unique_ptr<PGMutex> text_lock;
	int shared_counter = 0;

	void ApplySettings(PGTextFileSettings& settings);
	PGTextFileSettings settings;
	
	void _InsertLine(char* ptr, size_t prev, int& offset, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr);
};

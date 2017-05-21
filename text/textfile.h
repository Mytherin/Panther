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

#include "rust/globset.h"

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

class ProjectExplorer;
struct Interval;

enum PGLockType {
	PGReadLock,
	PGWriteLock
};

struct PGTextFileSettings {
	PGLineEnding line_ending = PGLineEndingUnknown;
	PGLanguage* language;
	PGFileEncoding encoding = PGEncodingUnknown;
	std::string name;

	PGTextFileSettings() : line_ending(PGLineEndingUnknown), language(nullptr), encoding(PGEncodingUnknown), name("") { }
};

struct FindAllInformation;

class TextFile {
	friend class Cursor;
	friend class FileManager;
	friend class TextLineIterator;
	friend class WrappedTextLineIterator;
	friend class TextField;
	friend class TextFile;
	friend class TabControl;
	friend class TextView;
public:
	// create an in-memory textfile with currently unspecified path
	TextFile();
	~TextFile();

	static TextFile* OpenTextFile(std::string filename, PGFileError& error, bool immediate_load = false, bool ignore_binary = false);

	TextLine GetLine(lng linenumber);

	void InsertText(std::vector<Cursor>& cursors, char character);
	void InsertText(std::vector<Cursor>& cursors, PGUTF8Character character);
	void InsertText(std::vector<Cursor>& cursors, std::string text);

	bool SplitLines(const std::string& text, std::vector<std::string>&);
	std::vector<std::string> SplitLines(const std::string& text);

	void DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction);
	void DeleteWord(std::vector<Cursor>& cursors, PGDirection direction);
	void AddNewLine(std::vector<Cursor>& cursors);
	void AddNewLine(std::vector<Cursor>& cursors, std::string text);
	void DeleteLines(std::vector<Cursor>& cursors);
	void DeleteLine(std::vector<Cursor>& cursors, PGDirection direction);
	void AddEmptyLine(std::vector<Cursor>& cursors, PGDirection direction);
	void MoveLines(std::vector<Cursor>& cursors, int offset);

	std::string GetText();
	std::string CutText(std::vector<Cursor>& cursors);
	std::string CopyText(std::vector<Cursor>& cursors);
	void PasteText(std::vector<Cursor>& cursors, std::string& text);
	void RegexReplace(std::vector<Cursor>& cursors, PGRegexHandle regex, std::string& replacement);

	bool Reload(PGFileError& error);

	void ChangeLineEnding(PGLineEnding lineending);
	void ChangeFileEncoding(PGFileEncoding encoding);
	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo(TextView* view);
	void Redo(TextView* view);

	void AddTextView(std::shared_ptr<TextView> view);

	void SaveChanges();
	void SaveAs(std::string path);

	PGFileEncoding GetFileEncoding() { return encoding; }
	PGLineEnding GetLineEnding() { return lineending; }
	PGLineIndentation GetLineIndentation() { return indentation; }
	PGLanguage* GetLanguage() { return language; }
	void SetLanguage(PGLanguage* language);

	lng GetLineCount();

	void Lock(PGLockType type);
	void Unlock(PGLockType type);

	bool IsLoaded() { return is_loaded; }
	double LoadPercentage() { return (double) bytes / (double) total_bytes; }

	void IndentText(std::vector<Cursor>& cursors, PGDirection direction);

	void VerifyPartialTextfile();
	void VerifyTextfile();

	PGScalar GetMaxLineWidth(PGFontHandle font);

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

	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap, std::shared_ptr<Task> current_task);
	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap, std::shared_ptr<Task> current_task);

	void SetSettings(PGTextFileSettings settings);
	PGTextFileSettings GetSettings();

	// only used for "Find Results"
	// FIXME: this should not be in the TextFile class
	void FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data);
	std::shared_ptr<Task> find_task = nullptr;
	void AddFindMatches(std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng start_line);
	std::string current_find_file;

	void ConvertToIndentation(PGLineIndentation indentation);

	void FindAllMatchesAsync(PGGlobSet whitelist, ProjectExplorer* explorer, PGRegexHandle regex_handle, int context_lines, bool ignore_binary);
private:
	// load textfile from a file
	TextFile(PGFileEncoding encoding, std::string filename, char* base_data, lng size, bool immediate_load = false, bool delete_file = true);

	bool WriteToFile(PGFileHandle file, PGEncoderHandle encoder, const char* text, lng size, char** output_text, lng* output_size, char** intermediate_buffer, lng* intermediate_size);

	void InsertLines(std::vector<Cursor>& cursors, std::string text, size_t cursor);
	void ReplaceText(std::vector<Cursor>& cursors, std::string replacement_text, size_t i);
	// replace text in the specified text range with <replacement_text>
	// neither <range> nor <replacement_text> may not contain newlines
	void ReplaceText(std::vector<Cursor>& cursors, PGTextRange range, std::string replacement_text);
	// insert text at the specified cursor number, text must not include newlines
	void InsertText(std::vector<Cursor>& cursors, std::string text, size_t cursornr);
	// insert text at the specified position, text must not include newlines
	void InsertText(std::vector<Cursor>& cursors, std::string text, PGTextBuffer* buffer, lng position);
	// delete the selection of the specified cursor number, cursor selection must not be empty
	void DeleteSelection(std::vector<Cursor>& cursors, size_t cursornr);
	// delete the specified text range
	void DeleteText(std::vector<Cursor>& cursors, PGTextRange);

	bool read_only = false;

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
	
	double total_width = 0;
	lng longest_line = 0;

	void OpenFile(char* base_data, lng size, bool delete_file);

	void AddDelta(TextDelta* delta);
	void Undo(TextView* view, TextDelta* delta);

	void Undo(std::vector<Cursor>& cursors, PGReplaceText& delta, size_t i);
	void Undo(std::vector<Cursor>& cursors, PGRegexReplace& delta, size_t i);
	void Undo(std::vector<Cursor>& cursors, RemoveText& delta, std::string& text, size_t i);

	void PerformOperation(std::vector<Cursor>& cursors, TextDelta* delta);
	bool PerformOperation(std::vector<Cursor>& cursors, TextDelta* delta, bool redo);
	std::vector<Interval> GetCursorIntervals(std::vector<Cursor>& cursors);

	std::shared_ptr<Task> current_task = nullptr;
	static void RunHighlighter(std::shared_ptr<Task> task, TextFile* textfile);
	static void OpenFileAsync(std::shared_ptr<Task> task, void* info);

	void InvalidateBuffer(PGTextBuffer* buffer);
	void InvalidateBuffers(TextView* responsible_view);
	void InvalidateParsing();

	bool reload_on_changed = true;
	bool is_loaded;
	lng bytes = 0;
	lng total_bytes = 1;

	PGTextBuffer* GetBuffer(lng line);

	lng linecount = 0;
	PGTextPosition max_line_length;

	std::vector<std::weak_ptr<TextView>> views;
	std::vector<PGTextBuffer*> buffers;
	std::vector<std::unique_ptr<TextDelta>> deltas;
	std::vector<RedoStruct> redos;
	PGLineEnding lineending;
	PGLineIndentation indentation;
	PGFileEncoding encoding;
	int tabwidth = 0;
	
	PGLanguage* language = nullptr;
	std::unique_ptr<SyntaxHighlighter> highlighter = nullptr;

	std::unique_ptr<PGMutex> text_lock;
	int shared_counter = 0;

	void ApplySettings(PGTextFileSettings& settings);
	PGTextFileSettings settings;
	
	void _InsertLine(char* ptr, size_t prev, int& offset, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr);
};

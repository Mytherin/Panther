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

struct Interval {
	lng start_line;
	lng end_line;
	std::vector<Cursor*> cursors;
	Interval(lng start, lng end, Cursor* cursor) : start_line(start), end_line(end) { cursors.push_back(cursor); }
};

struct FindAllInformation;

typedef void (*PGTextFileLoadedCallback)(std::shared_ptr<TextFile> file, void* data);
typedef void (*PGTextFileDestructorCallback)(void* data);

class TextFile : public std::enable_shared_from_this<TextFile> {
	friend class InMemoryTextFile;
	friend class StreamingTextFile;
public:
	// create an in-memory textfile with currently unspecified path
	TextFile();
	// load textfile from a file
	TextFile(std::string filename);
	virtual ~TextFile();

	virtual TextLine GetLine(lng linenumber) = 0;

	virtual void InsertText(std::vector<Cursor>& cursors, char character) = 0;
	virtual void InsertText(std::vector<Cursor>& cursors, PGUTF8Character character) = 0;
	virtual void InsertText(std::vector<Cursor>& cursors, std::string text) = 0;

	static std::vector<Interval> GetCursorIntervals(std::vector<Cursor>& cursors);
	static bool SplitLines(const std::string& text, std::vector<std::string>&);
	static std::vector<std::string> SplitLines(const std::string& text);
	static void InvalidateBuffer(PGTextBuffer* buffer);

	virtual void DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction) = 0;
	virtual void DeleteWord(std::vector<Cursor>& cursors, PGDirection direction) = 0;
	virtual void AddNewLine(std::vector<Cursor>& cursors) = 0;
	virtual void AddNewLine(std::vector<Cursor>& cursors, std::string text) = 0;
	virtual void DeleteLines(std::vector<Cursor>& cursors) = 0;
	virtual void DeleteLine(std::vector<Cursor>& cursors, PGDirection direction) = 0;
	virtual void AddEmptyLine(std::vector<Cursor>& cursors, PGDirection direction) = 0;
	virtual void MoveLines(std::vector<Cursor>& cursors, int offset) = 0;

	virtual std::string GetText() = 0;
	virtual std::string CutText(std::vector<Cursor>& cursors) = 0;
	virtual std::string CopyText(std::vector<Cursor>& cursors) = 0;
	virtual void PasteText(std::vector<Cursor>& cursors, std::string& text) = 0;
	virtual void RegexReplace(std::vector<Cursor>& cursors, PGRegexHandle regex, std::string& replacement) = 0;

	virtual bool Reload(PGFileError& error) = 0;

	void ChangeLineEnding(PGLineEnding lineending);
	void ChangeFileEncoding(PGFileEncoding encoding);
	virtual void ChangeIndentation(PGLineIndentation indentation) = 0;
	virtual void RemoveTrailingWhitespace() = 0;

	virtual void Undo(TextView* view) = 0;
	virtual void Redo(TextView* view) = 0;

	void AddTextView(std::shared_ptr<TextView> view);

	virtual void SaveChanges() = 0;
	void SaveAs(std::string path);

	PGFileEncoding GetFileEncoding() { return encoding; }
	PGLineEnding GetLineEnding() { return lineending; }
	PGLineIndentation GetLineIndentation() { return indentation; }
	PGLanguage* GetLanguage() { return language; }
	
	virtual void SetLanguage(PGLanguage* language) = 0;

	virtual lng GetLineCount() = 0;

	void Lock(PGLockType type);
	void Unlock(PGLockType type);

	bool IsLoaded() { return is_loaded; }
	double LoadPercentage() { return (double) bytes / (double) total_bytes; }

	virtual void IndentText(std::vector<Cursor>& cursors, PGDirection direction) = 0;

	void VerifyPartialTextfile();
	void VerifyTextfile();

	virtual PGScalar GetMaxLineWidth(PGFontHandle font) = 0;

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

	virtual PGStoreFileType WorkspaceFileStorage() = 0;

	bool HasUnsavedChanges() { return FileInMemory() || unsaved_changes; }
	bool FileInMemory() { return path.size() == 0; }

	virtual PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap) = 0;
	virtual PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap) = 0;

	void SetSettings(PGTextFileSettings settings);
	PGTextFileSettings GetSettings();

	// only used for "Find Results"
	// FIXME: this should not be in the TextFile class
	virtual void FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data) = 0;

	virtual void ConvertToIndentation(PGLineIndentation indentation) = 0;

	void PendDelete();

	void OnLoaded(PGTextFileLoadedCallback callback, PGTextFileDestructorCallback destructor, void* data);

	virtual PGTextBuffer* GetBuffer(lng line) = 0;
	virtual PGTextBuffer* GetBufferFromWidth(double width) = 0;
	virtual PGTextBuffer* GetFirstBuffer() = 0;
	virtual PGTextBuffer* GetLastBuffer() = 0;

	double GetTotalWidth() { return total_width; }

	void SetUnsavedChanges(bool changes);

	bool FileHasErrors() { return bytes < 0; }

	lng last_modified_time = -1;
	lng last_modified_notification = -1;
	bool last_modified_deletion = false;
	bool reload_on_changed = true;
protected:
	bool read_only = false;

	bool unsaved_changes = false;
	bool pending_delete = false;

	lng saved_undo_count = 0;

	std::string path;
	std::string name;
	std::string ext;

	PGFileError error = PGFileSuccess;

	double total_width = 0;
	lng longest_line = 0;

	bool is_loaded;
	lng bytes = 0;
	lng total_bytes = 1;

	std::vector<PGTextBuffer*> buffers;

	PGLanguage* language = nullptr;
	std::unique_ptr<SyntaxHighlighter> highlighter = nullptr;

	std::unique_ptr<PGMutex> text_lock;
	int shared_counter = 0;

	std::unique_ptr<PGMutex> loading_lock;
	struct LoadCallbackData {
		PGTextFileLoadedCallback callback = nullptr;
		PGTextFileDestructorCallback destructor = nullptr;
		void* data = nullptr;

		LoadCallbackData() : callback(nullptr), destructor(nullptr), data(nullptr) {}
		~LoadCallbackData() {
			if (data && destructor) {
				destructor(data);
			}
		}
	};
	std::vector<std::unique_ptr<LoadCallbackData>> loading_data;

	void FinalizeLoading();
	virtual void ApplySettings(PGTextFileSettings settings);

	std::vector<std::weak_ptr<TextView>> views;

	PGLineEnding lineending;
	PGLineIndentation indentation;
	PGFileEncoding encoding;
	int tabwidth = 0;

	std::shared_ptr<Task> current_task = nullptr;

	lng linecount = 0;
	PGTextPosition max_line_length;

	void ConsumeBytes(const char* buffer, size_t buffer_size, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr, char& prev_character);
	void ConsumeBytes(const char* buffer, size_t buffer_size, size_t& prev, int& offset, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr);
private:
	std::shared_ptr<Task> find_task = nullptr;
	std::string current_find_file;
	
	void _InsertLine(const char* ptr, size_t current, size_t prev, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr);
	void _InsertText(const char* ptr, size_t current, size_t prev, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr);
};

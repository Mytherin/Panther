
#include "textfile.h"

class InMemoryTextFile : public TextFile {
public:
	static std::shared_ptr<TextFile> OpenTextFile(std::string filename, PGFileError& error, bool immediate_load = false, bool ignore_binary = false);
	static std::shared_ptr<TextFile> OpenTextFile(PGFileEncoding encoding, std::string path, char* buffer, size_t buffer_size, bool immediate_load = false);

	InMemoryTextFile();
	InMemoryTextFile(std::string filename);
	~InMemoryTextFile();

	TextLine GetLine(lng linenumber);

	void InsertText(std::vector<Cursor>& cursors, char character);
	void InsertText(std::vector<Cursor>& cursors, PGUTF8Character character);
	void InsertText(std::vector<Cursor>& cursors, std::string text);

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

	void SaveChanges();
	void SaveAs(std::string path);

	void SetLanguage(PGLanguage* language);

	lng GetLineCount();

	void IndentText(std::vector<Cursor>& cursors, PGDirection direction);

	void VerifyPartialTextfile();
	void VerifyTextfile();

	PGScalar GetMaxLineWidth(PGFontHandle font);

	PGStoreFileType WorkspaceFileStorage();

	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap);
	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap);

	void ConvertToIndentation(PGLineIndentation indentation);

	void FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data);
	void FindAllMatchesAsync(PGGlobSet whitelist, ProjectExplorer* explorer, PGRegexHandle regex_handle, int context_lines, bool ignore_binary);

	PGTextBuffer* GetBuffer(lng line);
	PGTextBuffer* GetBufferFromWidth(double width);
	PGTextBuffer* GetFirstBuffer();
	PGTextBuffer* GetLastBuffer();
protected:
	void ApplySettings(PGTextFileSettings settings);
private:
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

	void OpenFile(std::shared_ptr<TextFile> file, PGFileEncoding encoding, char* base, size_t size, bool immediate_load);
	void OpenFile(char* base_data, lng size, bool delete_file);
	void ReadFile(std::shared_ptr<TextFile> file, bool immediate_load, bool ignore_binary);
	void ActuallyReadFile(std::shared_ptr<TextFile> file, bool ignore_binary);

	void AddDelta(TextDelta* delta);
	void Undo(TextView* view, TextDelta* delta);

	void Undo(std::vector<Cursor>& cursors, PGReplaceText& delta, size_t i);
	void Undo(std::vector<Cursor>& cursors, PGRegexReplace& delta, size_t i);
	void Undo(std::vector<Cursor>& cursors, RemoveText& delta, std::string& text, size_t i);

	void PerformOperation(std::vector<Cursor>& cursors, TextDelta* delta);
	bool PerformOperation(std::vector<Cursor>& cursors, TextDelta* delta, bool redo);

	void HighlightText();
	
	void AddFindMatches(std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng start_line);

	void InvalidateBuffers(TextView* responsible_view);
	void InvalidateParsing();

	std::vector<std::unique_ptr<TextDelta>> deltas;
	std::vector<RedoStruct> redos;
};
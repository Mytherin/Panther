
#include "textfile.h"


class StreamingTextFile : public TextFile {
public:
	~StreamingTextFile();

	static std::shared_ptr<TextFile> OpenTextFile(std::string filename, PGFileError& error, bool ignore_binary = false);

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

	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo(TextView* view);
	void Redo(TextView* view);

	void SaveChanges();
	void SetLanguage(PGLanguage* language);

	lng GetLineCount();
	void IndentText(std::vector<Cursor>& cursors, PGDirection direction);

	void VerifyPartialTextfile();
	void VerifyTextfile();

	PGScalar GetMaxLineWidth(PGFontHandle font);
	PGStoreFileType WorkspaceFileStorage();

	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap);
	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap);

	void FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data);
	void ConvertToIndentation(PGLineIndentation indentation);
	void FindAllMatchesAsync(PGGlobSet whitelist, ProjectExplorer* explorer, PGRegexHandle regex_handle, int context_lines, bool ignore_binary);

	PGTextBuffer* GetBuffer(lng line);

	PGTextBuffer* GetBufferFromWidth(double width);
	PGTextBuffer* GetFirstBuffer();
	PGTextBuffer* GetLastBuffer();
private:
	StreamingTextFile(PGFileHandle handle, std::string filename);

	lng linecount = 0;
	PGTextPosition max_line_length;

	PGFileHandle handle;

	std::vector<PGTextBuffer*> buffers;
};
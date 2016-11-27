#pragma once

#include "cursor.h"
#include "encoding.h"
#include "textdelta.h"
#include "mmap.h"
#include "utils.h"
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

typedef enum {
	PGIndentionSpaces,
	PGIndentionTabs,
	PGIndentionMixed
} PGLineIndentation;

class TextField;

class TextFile {
public:
	TextFile(std::string filename);
	TextFile(std::string filename, TextField* textfield);
	~TextFile();

	std::string GetText();

	TextLine* GetLine(int linenumber);
	void InsertText(char character, std::vector<Cursor>& cursors);
	void InsertText(std::string text, std::vector<Cursor>& cursors);
	void DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction);
	void DeleteWord(std::vector<Cursor>& cursors, PGDirection direction);
	void AddNewLine(std::vector<Cursor>& cursors);
	void AddNewLine(std::vector<Cursor>& cursors, std::string text);
	void AddNewLines(std::vector<Cursor>& cursors, std::vector<std::string>& lines, bool first_is_newline);

	std::string CopyText(std::vector<Cursor>& cursors);
	void PasteText(std::vector<Cursor>& cursors, std::string text);

	void ChangeLineEnding(PGLineEnding lineending);
	void ChangeFileEncoding(PGFileEncoding encoding);
	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo();
	void Redo();

	void SaveChanges();

	ssize_t GetLineCount();
private:
	void DeleteCharacter(MultipleDelta* delta, std::vector<Cursor>& cursors, PGDirection direction);
	void DeleteCharacter(MultipleDelta* delta, Cursor* cursor, PGDirection direction, bool delete_selection);

	void OpenFile(std::string filename);

	void AddDelta(TextDelta* delta);

	void Undo(TextDelta* delta);
	void Redo(TextDelta* delta);

	TextField* textfield;
	//PGMemoryMappedFileHandle file;
	std::vector<TextLine> lines;
	std::vector<TextDelta*> deltas;
	std::vector<TextDelta*> redos;
	std::string path;
	char* base;
	PGLineEnding lineending;
	PGLineIndentation indentation;
};

#pragma once

#include "cursor.h"
#include "encoding.h"
#include "textdelta.h"
#include "mmap.h"
#include "utils.h"
#include "textfile.h"
#include "textblock.h"
#include "scheduler.h"
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
struct Interval;

class TextFile {
public:
	TextFile(std::string filename);
	TextFile(std::string filename, TextField* textfield);
	~TextFile();

	std::string GetText();

	TextLine* GetLine(ssize_t linenumber);
	void InsertText(char character, std::vector<Cursor*>& cursors);
	void InsertText(std::string text, std::vector<Cursor*>& cursors);
	void DeleteCharacter(std::vector<Cursor*>& cursors, PGDirection direction);
	void DeleteWord(std::vector<Cursor*>& cursors, PGDirection direction);
	void AddNewLine(std::vector<Cursor*>& cursors);
	void AddNewLine(std::vector<Cursor*>& cursors, std::string text);
	void AddNewLines(std::vector<Cursor*>& cursors, std::vector<std::string>& lines, bool first_is_newline);
	void DeleteLines(std::vector<Cursor*>& cursors);
	void DeleteLine(std::vector<Cursor*>& cursors, PGDirection direction);
	void AddEmptyLine(std::vector<Cursor*>& cursors, PGDirection direction);
	void MoveLines(std::vector<Cursor*>& cursors, int offset);

	std::string CopyText(std::vector<Cursor*>& cursors);
	void PasteText(std::vector<Cursor*>& cursors, std::string text);

	void ChangeLineEnding(PGLineEnding lineending);
	void ChangeFileEncoding(PGFileEncoding encoding);
	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo(std::vector<Cursor*>& cursors);
	void Redo(std::vector<Cursor*>& cursors);

	void SaveChanges();

	PGLineEnding GetLineEnding() { return lineending; }
	ssize_t GetLineCount();

	void InvalidateParsing(ssize_t line);
	void InvalidateParsing(std::vector<ssize_t>& lines);
	void LockBlock(ssize_t block);
	void UnlockBlock(ssize_t block);
	ssize_t GetBlock(ssize_t linenr) { return linenr / TEXTBLOCK_SIZE; }
	ssize_t GetMaximumBlocks() { return lines.size() % TEXTBLOCK_SIZE == 0 ? GetBlock(lines.size()) : GetBlock(lines.size()) + 1; }
	bool BlockIsParsed(ssize_t block) { return parsed_blocks[block].parsed; }
private:
	void DeleteCharacter(MultipleDelta* delta, std::vector<Cursor*>& cursors, PGDirection direction);
	void DeleteCharacter(MultipleDelta* delta, Cursor* cursor, PGDirection direction, bool delete_selection, bool include_cursor = true);

	void OpenFile(std::string filename);

	void AddDelta(TextDelta* delta);

	void PerformOperation(TextDelta* delta, bool adjust_delta = true);
	void PerformOperation(TextDelta* delta, std::vector<ssize_t>& invalidated_lines, bool adjust_delta = true);
	void Undo(TextDelta* delta, std::vector<ssize_t>& invalidated_lines, std::vector<Cursor*>& cursors);
	void Redo(TextDelta* delta, std::vector<Cursor*>& cursors);
	std::vector<Interval> GetCursorIntervals(std::vector<Cursor*>& cursors);

	Task* current_task;
	static void TextFile::RunHighlighter(Task* task, TextFile* textfile);

	TextField* textfield;
	//PGMemoryMappedFileHandle file;
	std::vector<TextLine> lines;
	std::vector<TextDelta*> deltas;
	std::vector<TextDelta*> redos;
	std::vector<TextBlock> parsed_blocks;
	std::vector<PGMutexHandle> block_locks;
	std::string path;
	char* base;
	PGLineEnding lineending;
	PGLineIndentation indentation;

	SyntaxHighlighter* highlighter;
};

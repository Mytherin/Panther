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
	TextFile(std::string filename, TextField* textfield, bool immediate_load = false);
	~TextFile();

	TextLine* GetLine(lng linenumber);
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
	lng GetLineCount();

	void LockBlock(lng block);
	void UnlockBlock(lng block);
	lng GetBlock(lng linenr) { return linenr / TEXTBLOCK_SIZE; }
	lng GetMaximumBlocks() { return lines.size() % TEXTBLOCK_SIZE == 0 ? GetBlock(lines.size()) : GetBlock(lines.size()) + 1; }
	bool BlockIsParsed(lng block) { return parsed_blocks[block].parsed; }

	bool IsLoaded() { return is_loaded; }
	double LoadPercentage() { return loaded; }
private:
	void DeleteCharacter(MultipleDelta* delta, std::vector<Cursor*>& cursors, PGDirection direction);
	void DeleteCharacter(MultipleDelta* delta, Cursor* cursor, PGDirection direction, bool delete_selection, bool include_cursor = true);

	void OpenFile(std::string filename);

	void AddDelta(TextDelta* delta);

	void PerformOperation(TextDelta* delta, bool adjust_delta = true);
	void PerformOperation(TextDelta* delta, std::vector<lng>& invalidated_lines, bool adjust_delta = true);
	void Undo(TextDelta* delta, std::vector<lng>& invalidated_lines, std::vector<Cursor*>& cursors);
	void Redo(TextDelta* delta, std::vector<Cursor*>& cursors);
	std::vector<Interval> GetCursorIntervals(std::vector<Cursor*>& cursors);

	Task* current_task;
	static void RunHighlighter(Task* task, TextFile* textfile);
	static void OpenFileAsync(Task* task, void* info);

	void InvalidateParsing(lng line);
	void InvalidateParsing(std::vector<lng>& lines);

	bool is_loaded;
	double loaded;

	TextField* textfield;
	//PGMemoryMappedFileHandle file;
	std::vector<TextLine> lines;
	std::vector<TextDelta*> deltas;
	std::vector<TextDelta*> redos;
	std::vector<TextBlock> parsed_blocks;
	std::vector<PGMutexHandle> block_locks;
	std::string path;
	PGLineEnding lineending;
	PGLineIndentation indentation;

	SyntaxHighlighter* highlighter;
};

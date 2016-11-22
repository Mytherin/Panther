#pragma once

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

class TextFile {
public:
	TextFile(std::string filename);
	~TextFile();

	std::string GetText();

	TextLine* GetLine(int linenumber);
	void InsertText(char character, int linenumber, int characternumber);
	void DeleteCharacter(int linenumber, int characternumber);
	void AddNewLine(int linenumber, int characternumber);

	void SetLineEnding(PGLineEnding lineending);

	void Undo();
	void Redo();

	void Flush();

	ssize_t GetLineCount();
private:
	void AddDelta(TextDelta* delta);

	void Undo(TextDelta* delta);
	void Redo(TextDelta* delta);

	PGMemoryMappedFileHandle file;
	std::vector<TextLine> lines;
	std::vector<TextDelta*> deltas;
	std::vector<TextDelta*> redos;
	char* base;
	PGLineEnding lineending;
};
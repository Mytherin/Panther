#pragma once

#include "mmap.h"
#include "utils.h"
#include <string>
#include <vector>

typedef enum {
	PGWindows,
	PGMacOS,
	PGUnix,
	PGMixed,
	PGUnknown
} PGLineEnding;

class TextLine {
	friend class TextFile;
public:
	size_t GetLength();
	char* GetLine();
	TextLine(char* line, int length) : line(line), length(length) { }
private:
	size_t length;
	char* line;

};

class TextFile {
public:
	TextFile(std::string filename);
	~TextFile();

	std::string GetText();

	TextLine* GetLine(int linenumber);
	void InsertText(char character, int linenumber, int characternumber);
	void DeleteCharacter(int linenumber, int characternumber);


	void Flush();

	ssize_t GetLineCount();
private:
	PGMemoryMappedFileHandle file;
	std::vector<TextLine> lines;
	char* base;
	PGLineEnding lineending;
};
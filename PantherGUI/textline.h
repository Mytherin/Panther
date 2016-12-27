#pragma once

#include "syntax.h"
#include "utils.h"
#include <string>

class TextDelta;
class TextFile;

struct TextLine {
	friend class SyntaxHighlighter;
	friend class TextFile;
public:
	lng GetLength(void);
	char* GetLine(void);
	std::string& GetString(void) { return line; }
	void AddDelta(TextFile* textfile, TextDelta* delta);
	void RemoveDelta(TextFile* textfile, TextDelta* delta);
	TextDelta* PopDelta(TextFile* textfile);
	
	TextLine(TextFile* textfile, char* line, lng length);
	TextLine(const TextLine&);
	~TextLine();

	PGSyntax syntax;
private:
	void UpdateTextFile(TextFile* textfile, lng oldsize);

	void UndoDelta(TextFile* textfile, TextDelta* delta);

	std::string line;
	TextDelta* applied_deltas;
};
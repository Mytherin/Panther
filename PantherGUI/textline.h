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
	void AddDelta(TextDelta* delta);
	void RemoveDelta(TextDelta* delta);
	TextDelta* PopDelta();
	
	TextLine(char* line, lng length) : line(line, length), syntax(), applied_deltas(nullptr) {}
	TextLine(const TextLine&);
	~TextLine();

	PGSyntax syntax;
private:
	void UndoDelta(TextDelta* delta);

	std::string line;
	TextDelta* applied_deltas;
};
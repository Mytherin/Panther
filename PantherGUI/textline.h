#pragma once

#include "syntax.h"
#include "utils.h"

class TextDelta;
class TextFile;

struct TextLine {
	friend class SyntaxHighlighter;
	friend class TextFile;
public:
	ssize_t GetLength(void);
	char* GetLine(void);
	void AddDelta(TextDelta* delta);
	void RemoveDelta(TextDelta* delta);
	void PopDelta();

	void ApplyDeltas();

	TextLine(char* line, ssize_t length) : line(line), length(length), deltas(nullptr), modified_line(nullptr), syntax() { syntax.next = nullptr; }

	PGParserState state;
	PGSyntax syntax;
private:
	ssize_t length;
	char* line;
	char *modified_line;
	TextDelta* deltas;
};
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
	ssize_t GetLength(void);
	char* GetLine(void); // FIXME std::string&
	void AddDelta(TextDelta* delta);
	void RemoveDelta(TextDelta* delta);
	TextDelta* PopDelta();

	void ApplyDeltas();

	TextLine(char* line, ssize_t length) : line(line, length), deltas(nullptr), syntax(), applied_deltas(nullptr) {}
	TextLine(const TextLine&);
	~TextLine();

	PGSyntax syntax;
private:
	void UndoDelta(TextDelta* delta);

	std::string line;
	TextDelta* deltas;
	TextDelta* applied_deltas;
};
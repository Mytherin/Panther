#pragma once

#include "utils.h"

class TextDelta;

struct TextLine {
	friend class TextFile;
public:
	size_t GetLength(void);
	char* GetLine(void);
	void AddDelta(TextDelta* delta);

	void ApplyDeltas();

	TextLine(char* line, size_t length) : line(line), length(length), deltas(NULL), modified_line(NULL) { }

private:
	size_t length;
	char* line;
	char *modified_line;
	TextDelta* deltas;
};
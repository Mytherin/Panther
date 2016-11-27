#pragma once

#include "utils.h"

class TextDelta;

struct TextLine {
	friend class TextFile;
public:
	ssize_t GetLength(void);
	char* GetLine(void);
	void AddDelta(TextDelta* delta);
	void RemoveDelta(TextDelta* delta);
	void PopDelta();

	void ApplyDeltas();

	TextLine(char* line, ssize_t length) : line(line), length(length), deltas(NULL), modified_line(NULL) { }

private:
	ssize_t length;
	char* line;
	char *modified_line;
	TextDelta* deltas;
};
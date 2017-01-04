#pragma once

#include "cursor.h"
#include "textbuffer.h"
#include "utils.h"

#include <string>
#include <vector>

class TextDelta;
class TextFile;

typedef enum {
	PGDeltaAddText,
	PGDeltaAddLines,
	PGDeltaRemoveText,
	PGDeltaRemoveLine,
	PGDeltaRemoveWord,
	PGDeltaMultiple,
	PGDeltaUnknown
} PGTextType;

class TextDelta {
public:
	PGTextType type;
	std::vector<CursorData> stored_cursors;
	TextDelta* next = nullptr;

	TextDelta(PGTextType type) : 
		type(type), next(nullptr) { }
	PGTextType TextDeltaType() { return type; }
};

class AddText : public TextDelta {
public:
	std::string text;

	AddText(std::string text) : 
		TextDelta(PGDeltaAddText), text(text) { }
};

class AddLines : public TextDelta {
public:
	std::vector<std::string> lines;

	AddLines(std::vector<std::string> lines) : 
		TextDelta(PGDeltaAddLines), lines(lines) { }
};

class RemoveText : public TextDelta {
public:
	PGDirection direction;
	std::vector<std::string> removed_text;

	RemoveText(PGDirection direction, PGTextType type) : 
		TextDelta(type), direction(direction) { }
};

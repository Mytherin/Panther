#pragma once

#include "cursor.h"
#include "textbuffer.h"
#include "utils.h"

#include <string>
#include <vector>

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

	TextDelta(std::vector<CursorData> stored_cursors, PGTextType type) : 
		stored_cursors(stored_cursors), type(type) { }
	PGTextType TextDeltaType() { return type; }
};

class AddText : public TextDelta {
public:
	std::string text;

	AddText(std::vector<CursorData> stored_cursors, std::string text) : 
		TextDelta(stored_cursors, PGDeltaAddText), text(text) { }
};

class AddLines : public TextDelta {
public:
	std::vector<std::string> lines;

	AddLines(std::vector<CursorData> stored_cursors, std::vector<std::string> lines) : 
		TextDelta(stored_cursors, PGDeltaAddLines), lines(lines) { }
};

class RemoveText : public TextDelta {
public:
	PGDirection direction;
	std::vector<std::string> removed_text;

	RemoveText(PGDirection direction, std::vector<CursorData> stored_cursors, PGTextType type) : 
		TextDelta(stored_cursors, type), direction(direction) { }
};

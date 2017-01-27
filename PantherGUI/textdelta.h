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
	PGDeltaRemoveText,
	PGDeltaCursor,
	PGDeltaRemoveLine,
	PGDeltaRemoveCharacter,
	PGDeltaRemoveWord,
	PGDeltaAddEmptyLine,
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
	virtual std::string ToString() { assert(0); return ""; }
	static TextDelta* FromString(std::string);
};

class AddText : public TextDelta {
public:
	std::vector<std::string> lines;

	AddText(std::string line) : 
		TextDelta(PGDeltaAddText) {
			lines.push_back(line);
		}
	AddText(std::vector<std::string> lines) : 
		TextDelta(PGDeltaAddText), lines(lines) { }
	std::string ToString();
};

class RemoveText : public TextDelta {
public:
	std::vector<std::string> removed_text;

	RemoveText() : 
		TextDelta(PGDeltaRemoveText) { }
	std::string ToString();
};

class RemoveSelection : public TextDelta {
public:
	PGDirection direction;
	RemoveSelection(PGDirection direction, PGTextType type) : 
		TextDelta(type), direction(direction) { }
	std::string ToString();
};

class InsertLineBefore : public TextDelta {
public:
	PGDirection direction;
	InsertLineBefore(PGDirection direction) : 
		TextDelta(PGDeltaAddEmptyLine), direction(direction) { }
	std::string ToString();
};

#pragma once

#include "textline.h"
#include "utils.h"

#include <string>

typedef enum {
	PGDeltaAddText,
	PGDeltaRemoveText,
	PGDeltaRemoveLine,
	PGTextUnknown
} PGTextType;

class TextDelta {
public:
	TextDelta* next;

	TextDelta() : next(NULL) { }

	virtual PGTextType TextDeltaType() { return PGTextUnknown; }
};

class AddText : public TextDelta {
public:
	int linenr;
	int characternr;
	std::string text;

	PGTextType TextDeltaType() { return PGDeltaAddText; }
	AddText(int linenr, int characternr, std::string text) : linenr(linenr), characternr(characternr), text(text) { }
};

class RemoveText : public TextDelta {
public:
	int linenr;
	int characternr;
	int charactercount;

	PGTextType TextDeltaType() { return PGDeltaRemoveText; }
	RemoveText(int linenr, int characternr, int charactercount) : linenr(linenr), characternr(characternr), charactercount(charactercount) { }
};

class RemoveLine : public TextDelta {
public:
	int linenr;
	TextLine line;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	RemoveLine(int linenr, TextLine line) : linenr(linenr), line(line) { }

};

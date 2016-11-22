#pragma once

#include "textline.h"
#include "utils.h"

#include <string>
#include <vector>

typedef enum {
	PGDeltaAddText,
	PGDeltaRemoveText,
	PGDeltaAddLine,
	PGDeltaRemoveLine,
	PGDeltaMultiple,
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
	TextLine *line;
	int linenr;
	int characternr;
	std::string text;

	PGTextType TextDeltaType() { return PGDeltaAddText; }
	AddText(int linenr, int characternr, std::string text) : linenr(linenr), characternr(characternr), text(text), line(NULL) { }
};

class RemoveText : public TextDelta {
public:
	TextLine *line;
	int linenr;
	int characternr;
	int charactercount;

	PGTextType TextDeltaType() { return PGDeltaRemoveText; }
	RemoveText(int linenr, int characternr, int charactercount) : linenr(linenr), characternr(characternr), charactercount(charactercount), line(NULL) { }
};

class RemoveLine : public TextDelta {
public:
	int linenr;
	TextLine line;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	RemoveLine(int linenr, TextLine line) : linenr(linenr), line(line) { }

};

class AddLine : public TextDelta {
public:
	int linenr;
	int characternr;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	AddLine(int linenr, int characternr) : linenr(linenr), characternr(characternr) { }

};

class MultipleDelta : public TextDelta {
public:
	std::vector<TextDelta*> deltas;

	PGTextType TextDeltaType() { return PGDeltaMultiple; }
	MultipleDelta() : deltas(NULL) { }

	void AddDelta(TextDelta* delta) {
		deltas.push_back(delta);
	}

};


#pragma once

#include "cursor.h"
#include "textline.h"
#include "utils.h"

#include <string>
#include <vector>

typedef enum {
	PGDeltaAddText,
	PGDeltaRemoveText,
	PGDeltaAddLine,
	PGDeltaRemoveLine,
	PGDeltaRemoveManyLines,
	PGDeltaMultiple,
	PGTextUnknown
} PGTextType;

class TextDelta {
public:
	TextDelta* next;

	TextDelta() : next(NULL) { }

	virtual PGTextType TextDeltaType() { return PGTextUnknown; }
};

class CursorDelta : public TextDelta {
public:
	Cursor* cursor;
	Cursor stored_cursor;

	CursorDelta(Cursor* cursor) : cursor(cursor), stored_cursor(NULL) { 
		if (cursor) stored_cursor = Cursor(*cursor);
	}

	virtual PGTextType TextDeltaType() { return PGTextUnknown; }
};

class AddText : public CursorDelta {
public:
	int linenr;
	int characternr;
	std::string text;

	PGTextType TextDeltaType() { return PGDeltaAddText; }
	AddText(Cursor* cursor, int linenr, int characternr, std::string text) : CursorDelta(cursor), linenr(linenr), characternr(characternr), text(text) { }
};

class RemoveText : public CursorDelta {
public:
	int linenr;
	int characternr;
	int charactercount;

	PGTextType TextDeltaType() { return PGDeltaRemoveText; }
	RemoveText(Cursor* cursor, int linenr, int characternr, int charactercount) : CursorDelta(cursor), linenr(linenr), characternr(characternr), charactercount(charactercount) { }
};

class RemoveLine : public CursorDelta {
public:
	int linenr;
	TextLine line;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	RemoveLine(Cursor* cursor, int linenr, TextLine line) : CursorDelta(cursor), linenr(linenr), line(line) { }
};

class RemoveLines : public CursorDelta {
public:
	int start;
	std::vector<TextLine> lines;

	PGTextType TextDeltaType() { return PGDeltaRemoveManyLines; }
	RemoveLines(Cursor* cursor, int start) : CursorDelta(cursor), start(start) { }
	void AddLine(TextLine line) {
		lines.push_back(line);
	}
};

class AddLine : public CursorDelta {
public:
	int linenr;
	int characternr;

	PGTextType TextDeltaType() { return PGDeltaAddLine; }
	AddLine(Cursor* cursor, int linenr, int characternr) : CursorDelta(cursor), linenr(linenr), characternr(characternr) { }

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


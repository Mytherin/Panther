#pragma once

#include "cursor.h"
#include "textline.h"
#include "utils.h"

#include <string>
#include <vector>

class TextFile;

typedef enum {
	PGDeltaAddText,
	PGDeltaRemoveText,
	PGDeltaAddLine,
	PGDeltaAddManyLines,
	PGDeltaRemoveLine,
	PGDeltaRemoveManyLines,
	PGDeltaMultiple,
	PGDeltaCursorMovement,
	PGDeltaUnknown
} PGTextType;

class TextDelta {
public:
	int linenr;
	int characternr;
	TextDelta* next;

	TextDelta() : next(NULL) {}

	virtual PGTextType TextDeltaType() { return PGDeltaUnknown; }
	TextDelta(int linenr, int characternr) : linenr(linenr), characternr(characternr) { }
};

class CursorDelta : public TextDelta {
public:
	Cursor* cursor;
	Cursor stored_cursor;

	CursorDelta(Cursor* cursor, int linenr, int characternr) : cursor(cursor), stored_cursor(NULL), TextDelta(linenr, characternr) {
		if (cursor) stored_cursor = Cursor(*cursor);
	}

	virtual PGTextType TextDeltaType() { return PGDeltaCursorMovement; }
};

class AddText : public CursorDelta {
public:
	std::string text;

	PGTextType TextDeltaType() { return PGDeltaAddText; }
	AddText(Cursor* cursor, int linenr, int characternr, std::string text) : CursorDelta(cursor, linenr, characternr), text(text) {}
};

class RemoveText : public CursorDelta {
public:
	int charactercount;

	PGTextType TextDeltaType() { return PGDeltaRemoveText; }
	RemoveText(Cursor* cursor, int linenr, int characternr, int charactercount) : CursorDelta(cursor, linenr, characternr), charactercount(charactercount) {}
};

class RemoveLine : public CursorDelta {
public:
	TextLine line;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	RemoveLine(Cursor* cursor, int linenr, TextLine line) : CursorDelta(cursor, linenr, characternr), line(line) {}
};

class RemoveLines : public CursorDelta {
public:
	std::vector<TextLine> lines;

	PGTextType TextDeltaType() { return PGDeltaRemoveManyLines; }
	RemoveLines(Cursor* cursor, int linenr) : CursorDelta(cursor, linenr, 0) {}
	void AddLine(TextLine line) {
		lines.push_back(line);
	}
};

class AddLine : public CursorDelta {
public:
	int cursor_position;
	std::string line;
	std::string extra_text;
	RemoveText* remove_text;


	PGTextType TextDeltaType() { return PGDeltaAddLine; }
	AddLine(Cursor* cursor, int linenr, int characternr, std::string text) : CursorDelta(cursor, linenr, characternr), line(text), cursor_position(0), remove_text(NULL) {}
	AddLine(Cursor* cursor, int linenr, int characternr, std::string text, int cursor_position) : CursorDelta(cursor, linenr, characternr), line(text), cursor_position(cursor_position), remove_text(NULL) {}
};

class AddLines : public CursorDelta {
public:
	std::vector<std::string> lines;
	std::string extra_text;
	RemoveText* remove_text;


	PGTextType TextDeltaType() { return PGDeltaAddManyLines; }
	AddLines(Cursor* cursor, int linenr, int characternr, std::vector<std::string> lines) : CursorDelta(cursor, linenr, characternr), lines(lines), remove_text(NULL) {}
};

class MultipleDelta : public TextDelta {
public:
	std::vector<TextDelta*> deltas;

	PGTextType TextDeltaType() { return PGDeltaMultiple; }
	MultipleDelta() : deltas(NULL) {}

	void AddDelta(TextDelta* delta) {
		deltas.push_back(delta);
	}

	void FinalizeDeltas(TextFile* file);
};


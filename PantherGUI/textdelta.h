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
	PGDeltaAddManyLines,
	PGDeltaRemoveLine,
	PGDeltaRemoveManyLines,
	PGDeltaMultiple,
	PGTextUnknown
} PGTextType;

class TextDelta {
public:
	TextDelta* next;

	TextDelta() : next(NULL) {}

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
	AddText(Cursor* cursor, int linenr, int characternr, std::string text) : CursorDelta(cursor), linenr(linenr), characternr(characternr), text(text) {}
};

class RemoveText : public CursorDelta {
public:
	int linenr;
	int characternr;
	int charactercount;

	PGTextType TextDeltaType() { return PGDeltaRemoveText; }
	RemoveText(Cursor* cursor, int linenr, int characternr, int charactercount) : CursorDelta(cursor), linenr(linenr), characternr(characternr), charactercount(charactercount) {}
};

class RemoveLine : public CursorDelta {
public:
	int linenr;
	TextLine line;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	RemoveLine(Cursor* cursor, int linenr, TextLine line) : CursorDelta(cursor), linenr(linenr), line(line) {}
};

class RemoveLines : public CursorDelta {
public:
	int start;
	std::vector<TextLine> lines;

	PGTextType TextDeltaType() { return PGDeltaRemoveManyLines; }
	RemoveLines(Cursor* cursor, int start) : CursorDelta(cursor), start(start) {}
	void AddLine(TextLine line) {
		lines.push_back(line);
	}
};

class AddLine : public CursorDelta {
public:
	int linenr;
	int characternr;
	int cursor_position;
	std::string line;
	std::string extra_text;
	RemoveText* remove_text;


	PGTextType TextDeltaType() { return PGDeltaAddLine; }
	AddLine(Cursor* cursor, int linenr, int characternr, std::string text) : CursorDelta(cursor), linenr(linenr), characternr(characternr), line(text), cursor_position(0), remove_text(NULL) {}
	AddLine(Cursor* cursor, int linenr, int characternr, std::string text, int cursor_position) : CursorDelta(cursor), linenr(linenr), characternr(characternr), line(text), cursor_position(cursor_position), remove_text(NULL) {}
};

class AddLines : public CursorDelta {
public:
	int linenr;
	int characternr;
	std::vector<std::string> lines;
	std::string extra_text;
	RemoveText* remove_text;


	PGTextType TextDeltaType() { return PGDeltaAddManyLines; }
	AddLines(Cursor* cursor, int linenr, int characternr, std::vector<std::string> lines) : CursorDelta(cursor), linenr(linenr), characternr(characternr), lines(lines), remove_text(NULL) {}
};

class MultipleDelta : public TextDelta {
public:
	std::vector<TextDelta*> deltas;

	PGTextType TextDeltaType() { return PGDeltaMultiple; }
	MultipleDelta() : deltas(NULL) {}

	void AddDelta(TextDelta* delta) {
		deltas.push_back(delta);
	}

	void FinalizeDeltas() {
		for (auto it = deltas.begin(); it != deltas.end(); it++) {
			ssize_t shift_lines = 0;
			ssize_t shift_lines_start = 0;
			ssize_t shift_characters = 0;
			ssize_t shift_characters_line = 0;
			ssize_t shift_characters_character = 0;
			ssize_t deleted_lines_start = -1;
			ssize_t deleted_lines_end = -1;
			switch ((*it)->TextDeltaType()) {
			case PGDeltaRemoveLine:
				shift_lines = -1;
				shift_lines_start = ((RemoveLine*)(*it))->linenr;
				deleted_lines_start = ((RemoveLine*)(*it))->linenr;
				deleted_lines_end = ((RemoveLine*)(*it))->linenr;
				break;
			case PGDeltaRemoveManyLines:
				shift_lines = -((ssize_t) ((RemoveLines*)(*it))->lines.size());
				shift_lines_start = ((RemoveLines*)(*it))->start;
				deleted_lines_start = ((RemoveLines*)(*it))->start;
				deleted_lines_end = ((RemoveLines*)(*it))->start + ((RemoveLines*)(*it))->lines.size();
				break;
			case PGDeltaAddLine:
				shift_lines = 1;
				shift_lines_start = ((AddLine*)(*it))->linenr;
				break;
			case PGDeltaAddText:
				shift_characters = (ssize_t) ((AddText*)(*it))->text.size();
				shift_characters_line = ((AddText*)(*it))->linenr;
				shift_characters_character = ((AddText*)(*it))->characternr;
				break;
			case PGDeltaRemoveText:
				shift_characters = -((ssize_t) ((RemoveText*)(*it))->charactercount);
				shift_characters_line = ((RemoveText*)(*it))->linenr;
				shift_characters_character = ((RemoveText*)(*it))->characternr + shift_characters;
				break;
			}

			for (auto it2 = it + 1; it2 != deltas.end(); it2++) {
				TextDelta* delta = *it2;
				switch (delta->TextDeltaType()) {
				case PGDeltaRemoveLine:
					//assert(((RemoveLine*)delta)->linenr < deleted_lines_start || ((RemoveLine*)delta)->linenr > deleted_lines_end);
					if (((RemoveLine*)delta)->linenr >= shift_lines_start) {
						((RemoveLine*)delta)->linenr += shift_lines;
					}
					break;
				case PGDeltaRemoveManyLines:
					//assert(((RemoveLines*)delta)->start < deleted_lines_start || ((RemoveLines*)delta)->start > deleted_lines_end);
					if (((RemoveLines*)delta)->start >= shift_lines_start) {
						((RemoveLines*)delta)->start += shift_lines;
					}
					break;
				case PGDeltaAddLine:
					//assert(((RemoveLine*)delta)->linenr < deleted_lines_start || ((RemoveLine*)delta)->linenr > deleted_lines_end);
					if (((AddLine*)delta)->linenr >= shift_lines_start) {
						((AddLine*)delta)->linenr += shift_lines;
					}
					break;
				case PGDeltaAddText:
					//assert(((AddText*)delta)->linenr < deleted_lines_start || ((AddText*)delta)->linenr > deleted_lines_end);
					if (((AddText*)delta)->linenr >= shift_lines_start) {
						((AddText*)delta)->linenr += shift_lines;
					}
					if (((AddText*)delta)->linenr == shift_characters_line &&
						((AddText*)delta)->characternr > shift_characters_character) {
						((AddText*)delta)->characternr += shift_characters;
					}
					break;
				case PGDeltaRemoveText:
					//assert(((RemoveText*)delta)->linenr < deleted_lines_start || ((RemoveText*)delta)->linenr > deleted_lines_end);
					if (((RemoveText*)delta)->linenr >= shift_lines_start) {
						((RemoveText*)delta)->linenr += shift_lines;
					}
					if (((RemoveText*)delta)->linenr == shift_characters_line &&
						((RemoveText*)delta)->characternr > shift_characters_character) {
						((RemoveText*)delta)->characternr += shift_characters;
					}
					break;
				}
			}
		}
	}

};


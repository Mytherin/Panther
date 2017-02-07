#pragma once

#include "syntax.h"
#include "utils.h"
#include <string>
#include <vector>

#include "windowfunctions.h"

extern lng TEXT_BUFFER_SIZE;

class TextFile;
struct TextLine;
struct PGTextBuffer;

struct PGCursorPosition {
	lng line;
	lng position;
};

struct PGCharacterPosition {
	lng line;
	lng position;
	lng character;
};

struct PGLineWrap {
	std::vector<lng> lines;

	PGLineWrap() { }
	~PGLineWrap() { }
};

struct PGBufferUpdate {
	lng split_point = 0;
	PGTextBuffer* new_buffer = nullptr;
	PGBufferUpdate(lng offset) : split_point(offset), new_buffer(nullptr) { }
	PGBufferUpdate(lng split_point, PGTextBuffer* new_buffer) : split_point(split_point), new_buffer(new_buffer) { }
};

struct PGTextBuffer {
public:
	PGTextBuffer();
	PGTextBuffer(const char* text, lng size, lng start_line);
	~PGTextBuffer();

	char* buffer = nullptr;
	ulng buffer_size = 0;
	ulng current_size = 0;
	ulng start_line = 0;

	ulng syntax_count = 0;
	PGSyntax* syntax = nullptr;
	PGParserState state = nullptr;
	bool parsed = false;

	PGTextBuffer* prev = nullptr;
	PGTextBuffer* next = nullptr;

	double cumulative_width = -1;
	PGScalar wrap_width;
	std::vector<PGLineWrap> line_wraps;

	std::vector<PGCharacterPosition> cached_positions;


	void ClearWrappedLines();

	void Extend(ulng new_size);

	std::string GetString() { return std::string(buffer, next ? current_size : current_size - 1); }

	static lng GetBufferFromWidth(std::vector<PGTextBuffer*>& buffers, double percentage);
	static lng GetBuffer(std::vector<PGTextBuffer*>& buffers, lng line);

	// insert text into the specified buffer, "text" should not contain newlines
	// this function accounts for extending buffers and creating new buffers
	static PGBufferUpdate InsertText(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer, ulng position, std::string text, ulng linecount);

	// delete text from the specified buffer, text is deleted rightwards =>
	// this function can merge adjacent buffers together, if two adjacent buffers
	// are almost empty
	static PGBufferUpdate DeleteText(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer, ulng position, ulng size);


	// insert text into the current buffer, this should only be called if the text fits within the buffer
	void InsertText(ulng position, std::string text);
	// delete text from the current buffer without newlines
	void DeleteText(ulng position, ulng size);
	// delete text from the current buffer with potential newlines
	// returns the amount of deleted lines
	lng DeleteLines(ulng position, ulng end_position);
	lng DeleteLines(ulng start);

	// gets the total amount of lines in the buffer, parameter should be total amount of lines in the text file
	lng GetLineCount(lng total_lines);
	// gets the total width in the buffer; parameter should be total width of the text file
	double GetTotalWidth(double total_width);
	// gets all the lines in the buffer as TextLines
	std::vector<TextLine> GetLines();
	ulng GetBufferLocationFromCursor(lng line, lng character);
	void GetCursorFromBufferLocation(lng position, lng& line, lng& character);

	PGCharacterPosition GetCharacterFromPosition(ulng position);
	PGCursorPosition GetCursorFromPosition(ulng position, lng total_lines);
	TextLine GetLineFromPosition(ulng position);
};

void SetTextBufferSize(lng bufsiz);

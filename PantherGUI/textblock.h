#pragma once

#include "utils.h"
#include "syntax.h"

#define TEXTBLOCK_SIZE 64

struct TextBlock {
	ssize_t line_start;
	PGParserState state;
	bool parsed;

	TextBlock(ssize_t line) : line_start(line), state(PGParserDefaultState), parsed(false) { }
};
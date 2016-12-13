#pragma once

#include "utils.h"
#include "syntax.h"
#include "thread.h"

#define TEXTBLOCK_SIZE 64

struct TextBlock {
	lng line_start;
	PGParserState state;
	bool parsed;

	TextBlock(lng line) : line_start(line), state(PGParserErrorState), parsed(false) { }
};
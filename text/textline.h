#pragma once

#include "textbuffer.h"
#include "windowfunctions.h"

struct TextLine {
	friend class TextLineIterator;
	friend class WrappedTextLineIterator;
public:
	TextLine() : line(nullptr), length(0) { }
	TextLine(char* line, lng length) : line(line), length(length), syntax(nullptr) { }
	TextLine(char* line, lng length, PGSyntax* syntax) : line(line), length(length), syntax(syntax) { }
	TextLine(PGTextBuffer* buffer, lng line);

	lng GetLength(void) { return length; }
	char* GetLine(void) { return line; }
	bool IsValid() { return line != nullptr; }

	lng* WrapLine(TextView* view, lng linenr, PGFontHandle font, PGScalar wrap_width);
	lng RenderedLines(TextView* view, lng linenr, PGFontHandle font, PGScalar wrap_width);

	static lng* WrapLine(TextView* view, lng linenr, char* line, lng length, PGFontHandle font, PGScalar wrap_width);
	static lng RenderedLines(TextView* view, lng linenr, char* line, lng length, PGFontHandle font, PGScalar wrap_width);

	PGSyntax* syntax;

	// computes how to wrap the line starting at start_wrap
	// returns true if the line has to be wrapped, false if it fits entirely within wrap_width
	// if the function returns true, end_wrap is set to a value < length
	// returns the amount of lines this one line is rendered at with the given wrap width
	static bool WrapLine(char* line, lng length, PGFontHandle font, PGScalar wrap_width, lng start_wrap, lng& end_wrap);
	static lng RenderedLines(char* line, lng length, PGFontHandle font, PGScalar wrap_width);

	bool WrapLine(PGFontHandle font, PGScalar wrap_width, lng start_wrap, lng& end_wrap);

private:
	char* line;
	lng length;
};

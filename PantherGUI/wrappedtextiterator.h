#pragma once

#include "textbuffer.h"
#include "textiterator.h"
#include "textline.h"

class WrappedTextLineIterator : public TextLineIterator {
public:
	TextLine GetLine();

	lng GetCurrentLineNumber() { return current_line; }
	lng GetCurrentCharacterNumber() { return start_wrap; }
	PGTextRange GetCurrentRange() { return PGTextRange(buffer, start_position + start_wrap, buffer, start_position + end_wrap); }

	PGVerticalScroll GetCurrentScrollOffset();

	WrappedTextLineIterator(PGFontHandle font, TextFile* textfile, PGVerticalScroll scroll, PGScalar wrap_width);
protected:
	void PrevLine();
	void NextLine();

	PGFontHandle font;
	PGScalar wrap_width;
	lng start_wrap = 0;
	lng end_wrap;
	lng inner_line;
	lng max_inner_line;
	lng* wrap_positions;
	TextLine wrapped_line;
	PGSyntax syntax;

private:
	void SetCurrentScrollOffset(PGVerticalScroll scroll);

	void SetLineFromOffsets();
};

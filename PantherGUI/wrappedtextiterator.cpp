
#include "textbuffer.h"
#include "textdelta.h"
#include "textfile.h"
#include "unicode.h"

WrappedTextLineIterator::WrappedTextLineIterator(PGFontHandle font, TextFile* textfile, lng scroll_offset, PGScalar wrap_width, bool is_scroll_offset) :
	font(font), wrap_width(wrap_width) {
	lng current_line = scroll_offset;
	if (is_scroll_offset) {
		current_line = textfile->GetLineFromScrollPosition(font, wrap_width, scroll_offset);
	}
	this->Initialize(textfile, current_line);
	delete_syntax = false;
	start_wrap = 0;
	DetermineEndWrap();
}

void WrappedTextLineIterator::PrevLine() {
	assert(0);
}

void WrappedTextLineIterator::NextLine() {
	if (end_wrap >= textline.length) {
		// have to get the next line
		TextLineIterator::NextLine();
		start_wrap = 0;
	} else {
		start_wrap = end_wrap;
	}
	if (textline.line != nullptr) {
		DetermineEndWrap();
	} else {
		wrapped_line.line = nullptr;
	}
}

TextLine WrappedTextLineIterator::GetLine() {
	return wrapped_line;
}

bool TextLine::WrapLine(char* line, lng length, PGFontHandle font, PGScalar wrap_width, lng start_wrap, lng& end_wrap) {
	PGScalar current_width = 0;
	lng i = start_wrap;
	lng last_space = -1;
	end_wrap = length;
	for (; i < length; ) {
		int offset = utf8_character_length(line[i]);
		current_width += MeasureTextWidth(font, line + i, offset);
		if (current_width > wrap_width) {
			// we have to wrap now
			// try to wrap at the last space
			if (last_space < 0 || last_space <= start_wrap) {
				// there is no space
				end_wrap = i;
			} else {
				end_wrap = last_space;
			}
			return true;
		}
		if (offset == 1) {
			if (line[i] == '\t' || line[i] == ' ') {
				last_space = i;
			}
		}
		i += offset;
	}
	return false;
}

lng TextLine::RenderedLines(char* line, lng length, PGFontHandle font, PGScalar wrap_width) {
	lng lines = 1;
	lng start_wrap = 0;
	lng end_wrap;
	while (WrapLine(line, length, font, wrap_width, start_wrap, end_wrap)) {
		lines++;
		start_wrap = end_wrap;
	}
	return lines;
}

bool TextLine::WrapLine(PGFontHandle font, PGScalar wrap_width, lng start_wrap, lng& end_wrap) {
	return TextLine::WrapLine(line, length, font, wrap_width, start_wrap, end_wrap);
}

void WrappedTextLineIterator::DetermineEndWrap() {
	textline.WrapLine(font, wrap_width, start_wrap, end_wrap);
	if (delete_syntax) {
		wrapped_line.syntax.Delete();
		wrapped_line.syntax.next = nullptr;
		delete_syntax = false;
	}
	wrapped_line.line = textline.line + start_wrap;
	wrapped_line.length = end_wrap - start_wrap;
	wrapped_line.syntax.end = -1;
	if (start_wrap == 0 && end_wrap == textline.length) {
		// the wrapped line is the entire line, we can directly use the textline syntax
		delete_syntax = false;
		wrapped_line.syntax = textline.syntax;
	} else if (textline.syntax.end > 0) {
		delete_syntax = true;
		// we have to split the syntax into separate parts
		if (start_wrap > 0) {
			// we have to find the start syntax
			PGSyntax* syntax = &textline.syntax;
			while (syntax->next && syntax->end >= 0 && syntax->end < start_wrap) {
				syntax = syntax->next;
			}
			wrapped_line.syntax = *syntax;
		} else {
			wrapped_line.syntax = textline.syntax;
		}
		PGSyntax* syntax = &wrapped_line.syntax;
		while (syntax->next) {
			syntax->end -= start_wrap;
			if (syntax->end >= end_wrap) break;
			syntax->next = new PGSyntax(*syntax->next);
			syntax = syntax->next;
		}
		syntax->end = std::min(syntax->end, wrapped_line.length);
		syntax->next = nullptr;
	}
}
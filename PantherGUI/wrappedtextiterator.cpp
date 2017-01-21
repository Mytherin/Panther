
#include "textbuffer.h"
#include "textdelta.h"
#include "textfile.h"
#include "unicode.h"

WrappedTextLineIterator::WrappedTextLineIterator(PGFontHandle font, TextFile* textfile, PGVerticalScroll scroll, PGScalar wrap_width) :
	font(font), wrap_width(wrap_width) {
	lng current_line = scroll.linenumber;
	this->Initialize(textfile, current_line);
	delete_syntax = false;
	if (scroll.inner_line > 0) {
		SetCurrentScrollOffset(scroll);
	} else {
		start_wrap = 0;
	}
	DetermineEndWrap();
}

void WrappedTextLineIterator::PrevLine() {
	if (start_wrap == 0) {
		// have to get the previous line
		TextLineIterator::PrevLine();
		end_wrap = textline.length;
	} else {
		end_wrap = start_wrap;
	}
	if (textline.line != nullptr) {
		DetermineStartWrap();
	} else {
		wrapped_line.line = nullptr;
	}
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

void WrappedTextLineIterator::DetermineStartWrap() {
	lng start_wrap = 0, end_wrap;
	while (textline.WrapLine(font, wrap_width, start_wrap, end_wrap)) {
		if (end_wrap >= this->end_wrap) {
			this->start_wrap = start_wrap;
			SetLineFromOffsets();
			return;
		}
		start_wrap = end_wrap;
	}
	this->start_wrap = start_wrap;
	SetLineFromOffsets();
	return;
}

void WrappedTextLineIterator::DetermineEndWrap() {
	textline.WrapLine(font, wrap_width, start_wrap, end_wrap);
	SetLineFromOffsets();
}

PGVerticalScroll WrappedTextLineIterator::GetCurrentScrollOffset() {
	PGVerticalScroll scroll = PGVerticalScroll(current_line, 0);
	lng lines = 0;
	lng total_lines = 1;
	lng start_wrap = 0;
	lng end_wrap;
	while (textline.WrapLine(font, wrap_width, start_wrap, end_wrap)) {
		if (this->start_wrap >= end_wrap) {
			lines = total_lines;
		} else {
			break;
		}
		total_lines++;
		start_wrap = end_wrap;
	}
	scroll.inner_line = lines;
	return scroll;
}

void WrappedTextLineIterator::SetCurrentScrollOffset(PGVerticalScroll scroll) {
	lng start_wrap = 0;
	lng end_wrap;

	lng total_lines = textline.RenderedLines(textline.GetLine(), textline.GetLength(), font, wrap_width);
	lng line = scroll.inner_line;
	this->start_wrap = 0;
	if (line == 0) return;
	while (textline.WrapLine(font, wrap_width, start_wrap, end_wrap)) {
		line--;
		this->start_wrap = end_wrap;
		if (line == 0) {
			return;
		}
		start_wrap = end_wrap;
	}
}

void WrappedTextLineIterator::SetLineFromOffsets() {
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
			wrapped_line.syntax.next = syntax->next;
			wrapped_line.syntax.end = syntax->end;
			wrapped_line.syntax.type = syntax->type;
		} else {
			wrapped_line.syntax.next = textline.syntax.next;
			wrapped_line.syntax.end = textline.syntax.end;
			wrapped_line.syntax.type = textline.syntax.type;
		}
		PGSyntax* syntax = &wrapped_line.syntax;
		while (syntax->next) {
			syntax->end -= start_wrap;
			syntax->end = std::min(syntax->end, wrapped_line.length);
			if (syntax->end >= end_wrap) break;
			syntax->next = new PGSyntax(*syntax->next);
			syntax = syntax->next;
		}
		syntax->end = std::min(syntax->end, wrapped_line.length);
		syntax->next = nullptr;
	}
}


#include "textbuffer.h"
#include "textdelta.h"
#include "textfile.h"
#include "unicode.h"
#include "wrappedtextiterator.h"

WrappedTextLineIterator::WrappedTextLineIterator(PGFontHandle font, TextFile* textfile, PGVerticalScroll scroll, PGScalar wrap_width) :
	font(font), wrap_width(wrap_width), start_wrap(0) {
	lng current_line = scroll.linenumber;
	this->Initialize(textfile, current_line);
	delete_syntax = false;

	SetCurrentScrollOffset(scroll);
}

void WrappedTextLineIterator::PrevLine() {
	if (inner_line == 0) {
		if (current_line == 0) {
			wrapped_line.line = nullptr;
			wrapped_line.length = 0;
			textline.line = nullptr;
			textline.length = 0;
		} else {
			// have to get the previous line
			TextLineIterator::PrevLine();
			max_inner_line = textline.RenderedLines(this->buffer, this->current_line, textfile->GetLineCount(), font, wrap_width);
			inner_line = max_inner_line - 1;
			this->wrap_positions = textline.WrapLine(this->buffer, this->current_line, textfile->GetLineCount(), font, wrap_width);
			start_wrap = inner_line > 0 ? wrap_positions[inner_line - 1] : 0;
			end_wrap = wrap_positions[inner_line];
		}
	} else {
		end_wrap = wrap_positions[inner_line];
		start_wrap = wrap_positions[--inner_line];
	}
	this->SetLineFromOffsets();
}

void WrappedTextLineIterator::NextLine() {
	if (inner_line < max_inner_line - 1) {
		start_wrap = wrap_positions[inner_line];
		end_wrap = wrap_positions[++inner_line];
	} else {
		// have to get the next line
		TextLineIterator::NextLine();
		if (textline.line == nullptr) {
			wrapped_line.line = nullptr;
			wrapped_line.length = 0;
		} else {
			start_wrap = 0;
			this->wrap_positions = textline.WrapLine(this->buffer, this->current_line, textfile->GetLineCount(), font, wrap_width);
			end_wrap = wrap_positions[0];
			inner_line = 0;
			max_inner_line = textline.RenderedLines(this->buffer, this->current_line, textfile->GetLineCount(), font, wrap_width);
		}
	}
	this->SetLineFromOffsets();
}

TextLine WrappedTextLineIterator::GetLine() {
	return wrapped_line;
}

PGVerticalScroll WrappedTextLineIterator::GetCurrentScrollOffset() {
	return PGVerticalScroll(current_line, inner_line);
}

void WrappedTextLineIterator::SetCurrentScrollOffset(PGVerticalScroll scroll) {
	max_inner_line = textline.RenderedLines(this->buffer, this->current_line, textfile->GetLineCount(), font, wrap_width);
	if (scroll.inner_line >= max_inner_line) {
		scroll.inner_line = max_inner_line - 1;
	}
	inner_line = scroll.inner_line;
	assert(scroll.inner_line >= 0 && scroll.inner_line < max_inner_line);
	this->wrap_positions = textline.WrapLine(this->buffer, this->current_line, textfile->GetLineCount(), font, wrap_width);
	start_wrap = inner_line > 1 ? wrap_positions[inner_line - 1] : 0;
	end_wrap = wrap_positions[inner_line];
	SetLineFromOffsets();
}

void WrappedTextLineIterator::SetLineFromOffsets() {
	if (textline.line == nullptr) return;
	if (delete_syntax) {
		wrapped_line.syntax.Delete();
		wrapped_line.syntax.next = nullptr;
		delete_syntax = false;
	}
	wrapped_line.line = textline.line + start_wrap;
	wrapped_line.length = end_wrap - start_wrap;
	assert(end_wrap >= start_wrap);
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


#include "textline.h"
#include "unicode.h"

TextLine::TextLine(PGTextBuffer* buffer, lng line, lng max_line) {
	assert(line >= buffer->start_line);
	lng current_line = buffer->start_line;
	lng maximum_line = current_line + buffer->GetLineCount(max_line);
	this->line = buffer->buffer;
	this->length = std::max((lng) buffer->current_size - 1, (lng) 0);
	// check if there is more than one line in the buffer
	if (!(current_line + 1 == maximum_line)) {
		for (lng i = 0; i < buffer->current_size; ) {
			int offset = utf8_character_length(buffer->buffer[i]);
			if (offset == 1 && buffer->buffer[i] == '\n') {
				current_line++;
				if (current_line == line) {
					this->line = buffer->buffer + i + 1;
					if (current_line + 1 == maximum_line) {
						this->length = (buffer->current_size - 1) - (i + 1);
						break;
					}
				} else if (current_line == line + 1) {
					this->length = (buffer->buffer + i) - this->line;
					break;
				}
			}
			i += offset;
		}
	}
	if (buffer->syntax && buffer->parsed) {
		this->syntax = PGSyntax(buffer->syntax[line - buffer->start_line]);
	} else {
		this->syntax.end = -1;
	}
}

lng TextLine::RenderedLines(PGTextBuffer* buffer, lng linenr, lng total_lines, char* line, lng length, PGFontHandle font, PGScalar wrap_width) {
	// first wrap the line, if it has not already been wrapped
	WrapLine(buffer, linenr, total_lines, line, length, font, wrap_width);

	// then search until we find the end of the line
	return buffer->line_wraps[linenr - buffer->start_line].lines.size();
}

lng TextLine::RenderedLines(PGTextBuffer* buffer, lng linenr, lng total_lines, PGFontHandle font, PGScalar wrap_width) {
	return TextLine::RenderedLines(buffer, linenr, total_lines, line, length, font, wrap_width);
}

lng* TextLine::WrapLine(PGTextBuffer* buffer, lng linenr, lng total_lines, PGFontHandle font, PGScalar wrap_width) {
	return WrapLine(buffer, linenr, total_lines, line, length, font, wrap_width);
}

lng* TextLine::WrapLine(PGTextBuffer* buffer, lng linenr, lng total_lines, char* line, lng length, PGFontHandle font, PGScalar wrap_width) {
	lng buffer_lines = buffer->GetLineCount(total_lines);
	assert(linenr >= buffer->start_line && linenr < buffer->start_line + buffer->line_count);
	if (!panther::epsilon_equals(buffer->wrap_width, wrap_width)) {
		// cache is invalidated: wrap width is different
		buffer->line_wraps.clear();
		buffer->wrap_width = wrap_width;
	} else if (buffer->line_wraps.size() == buffer_lines) {
		// check if the cache exists
		if (buffer->line_wraps[linenr - buffer->start_line].lines.size() != 0) {
			// line wrap already cached
			return (lng*) (&buffer->line_wraps[linenr - buffer->start_line].lines[0]);
		}
	}
	assert(linenr >= buffer->start_line);
	assert(!buffer->next || linenr < buffer->next->start_line);
	if (buffer->line_wraps.size() != buffer_lines) {
		buffer->line_wraps.resize(buffer_lines);
	}

	lng index = linenr - buffer->start_line;
	lng start_wrap = 0;
	lng end_wrap = -1;
	std::vector<lng> wrap_positions;
	while (WrapLine(line, length, font, wrap_width, start_wrap, end_wrap)) {
		wrap_positions.push_back(end_wrap);
		start_wrap = end_wrap;
	}
	wrap_positions.push_back(length);
	buffer->line_wraps[index].lines = wrap_positions;
	return (lng*) (&buffer->line_wraps[linenr - buffer->start_line].lines[0]);
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
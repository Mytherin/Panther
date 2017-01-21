
#include "textbuffer.h"
#include "textdelta.h"
#include "textfile.h"
#include "unicode.h"

lng TEXT_BUFFER_SIZE = 4096;

PGTextBuffer::PGTextBuffer(const char* text, lng size, lng start_line) :
	current_size(size), start_line(start_line), state(nullptr), syntax(nullptr) {
	if (size + 1 < TEXT_BUFFER_SIZE) {
		buffer_size = TEXT_BUFFER_SIZE;
	} else {
		buffer_size = size + size / 5 + 2;
	}
	buffer = (char*)malloc(buffer_size);
	if (text) {
		memcpy(buffer, text, size);
	}
}

PGTextBuffer::~PGTextBuffer() {
	delete buffer;
}

lng PGTextBuffer::GetLineCount(lng total_lines) {
	return (this->next ? this->next->start_line : total_lines) - this->start_line;
}

std::vector<TextLine> PGTextBuffer::GetLines() {
	std::vector<TextLine> lines;
	lng current_position = 0;
	lng line = 0;
	for (lng i = 0; i < current_size; ) {
		int offset = utf8_character_length(buffer[i]);
		if (offset == 1 && buffer[i] == '\n') {
			PGSyntax syntax = parsed ? this->syntax[line] : PGSyntax();
			lines.push_back(TextLine(buffer + current_position, i - current_position, syntax));
			current_position = i + 1;
			line++;
		}
		i += offset;
	}
	return lines;
}

TextLine PGTextBuffer::GetLineFromPosition(ulng pos) {
	char* position = buffer + pos;
	if (pos > 0) {
		if (*position == '\n') position--;
		while (position > buffer && *position != '\n') {
			position--;
		}
		if (*position == '\n') position++;
	}
	char* end = buffer + pos;
	while (end < buffer + current_size && *end != '\n') {
		end++;
	}
	// FIXME: do we want to get the correct syntax for the line?
	PGSyntax syntax;
	syntax.end = -1;
	return TextLine(position, end - position, syntax);
}

void PGTextBuffer::GetCursorFromBufferLocation(lng position, lng& line, lng& character) {
	if (position >= current_size) {
		assert(this->next);
		return this->next->GetCursorFromBufferLocation(position - current_size, line, character);
	} else if (position < 0) {
		assert(this->prev);
		return this->prev->GetCursorFromBufferLocation(position + this->prev->current_size, line, character);
	}
	assert(position < current_size);
	line = start_line;
	character = 0;
	char* current_position = buffer;
	for (int i = 0; i < position; i++) {
		if (current_position[i] == '\n') {
			line++;
			character = 0;
		} else {
			character++;
		}
	}
}

ulng PGTextBuffer::GetBufferLocationFromCursor(lng line, lng position) {
	lng current_line = start_line;
	lng current_character = 0;
	for (ulng i = 0; i < current_size; ) {
		if (current_line == line && current_character == position) {
			return i;
		}
		int offset = utf8_character_length(buffer[i]);
		if (offset == 1 && buffer[i] == '\n') {
			current_character = 0;
			current_line++;
		} else {
			current_character += offset;
		}
		i += offset;
	}
	assert(0);
	return 0;
}

PGCursorPosition PGTextBuffer::GetCursorFromPosition(ulng position) {
	assert(position <= current_size);
	PGCursorPosition pos;
	pos.line = start_line;
	pos.position = 0;
	pos.character = 0;
	for (ulng i = 0; i < position; ) {
		int offset = utf8_character_length(buffer[i]);
		if (offset == 1 && buffer[i] == '\n') {
			pos.line++;
			pos.character = 0;
			pos.position = 0;
		} else {
			pos.character++;
			pos.position += offset;
		}
		i += offset;
	}
	return pos;
}

void PGTextBuffer::Extend(ulng new_size) {
	assert(new_size > buffer_size);
	char* new_buffer = (char*)malloc(new_size);
	assert(new_buffer);
	memcpy(new_buffer, buffer, current_size);
	free(buffer);
	buffer = new_buffer;
	buffer_size = new_size;
}

PGBufferUpdate PGTextBuffer::InsertText(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer, ulng position, std::string text, ulng linecount) {
	if (buffer->current_size + text.size() >= buffer->buffer_size) {
		// data does not fit within the current buffer
		if ((buffer->next != nullptr && buffer->start_line + 1 == buffer->next->start_line) ||
			(buffer->next == nullptr && buffer->start_line + 1 >= linecount)) {
			// there is only one line in the current buffer
			// that means we can't move lines out of the buffer to the next buffer
			// thus we extend the current buffer instead of creating new buffers
			lng new_size = std::max(buffer->buffer_size + buffer->buffer_size / 5, buffer->buffer_size + text.size() + 1);
			buffer->Extend(new_size);
			buffer->InsertText(position, text);
			return PGBufferUpdate(text.size());
		} else {
			lng buffer_position = PGTextBuffer::GetBuffer(buffers, buffer->start_line);

			// there are multiple lines in the current buffer
			// in this case, we can make room in the buffer by splitting the buffer
			// we want to make two buffers: [old_buffer] [new_buffer]
			// depending on [position] within the buffer, [position] is either in 
			// [old_buffer] or [new_buffer]

			// note that because "text" contains no newlines,
			// text is appended entirely to either [old_buffer] or [new_buffer]

			// every buffer ends with a newline except for the last buffer
			assert(buffer_position == buffers.size() - 1 || buffer->buffer[buffer->current_size - 1] == '\n');
			lng current_position = buffer->current_size - 1;
			lng current_line = 0;
			lng halfway_point = buffer->buffer_size / 2;
			for (lng i = current_position - 1; i >= 0; i--) {
				if (i == 0 && buffer->buffer[i] != '\n') {
					// only a single line in the buffer -> something went wrong
					assert(current_position != buffer->current_size - 1);
				}
				if (buffer->buffer[i] == '\n' || i == 0) {
					// found a line
					// this is a potential splitting point
					if (i <= halfway_point) {
						// we are past the halfway point
						// as a heuristic, we try to split the buffer in two parts
						// lets split either here or at the previous location
						lng split_point = current_position;
						if (std::abs(i - halfway_point) < std::abs(current_position - halfway_point) ||
							current_position == buffer->current_size - 1) {
							// this point is closer to the halfway point than the previous one
							// split here
							split_point = i;
							current_line++;
						}
						split_point++;
						assert(split_point < buffer->current_size);
						// create the new buffer and insert it to the right of the current buffer
						lng line = (buffer->next == nullptr ? linecount : buffer->next->start_line) - current_line;
						PGTextBuffer* new_buffer = new PGTextBuffer(buffer->buffer + split_point, buffer->current_size - split_point, line);
						if (buffer->next != nullptr) buffer->next->prev = new_buffer;
						new_buffer->next = buffer->next;
						new_buffer->prev = buffer;
						assert(new_buffer->start_line > buffer->start_line);
						assert(!new_buffer->next || new_buffer->next->start_line != new_buffer->start_line);
						buffer->current_size = split_point;
						buffer->next = new_buffer;
						buffers.insert(buffers.begin() + buffer_position + 1, new_buffer);

						PGTextBuffer* text_buffer = buffer;
						if (split_point <= position) {
							// split point is before the insert point
							// text ends up in the new (right) bucket
							position -= split_point;
							text_buffer = new_buffer;
						}
						if (text_buffer->buffer_size - text_buffer->current_size <= (lng)text.size()) {
							// even after splitting, the text does not fit within the buffer
							// extend the current buffer
							lng new_size = std::max((lng)(buffer->buffer_size + buffer->buffer_size / 5), (lng)(buffer->buffer_size + text.size() + 1));
							text_buffer->Extend(new_size);
						}
						text_buffer->InsertText(position, text);
						return PGBufferUpdate(split_point, new_buffer);
					}
					current_position = i;
					current_line++;
				}
			}
		}
	} else {
		// there is room in the buffer; simply insert the text
		buffer->InsertText(position, text);
		return PGBufferUpdate(text.size());
	}
	assert(0);
	return PGBufferUpdate(-1);
}

PGBufferUpdate PGTextBuffer::DeleteText(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer, ulng position, ulng size) {
	// first delete the text from the current buffer
	buffer->DeleteText(position, size);
	// now check if we want to merge this buffer to any adjacent buffers
	// we only merge forwards for simplicity when dealing with cursors
	PGTextBuffer* merge_buffer = nullptr;
	if (buffer->next && (buffer->current_size + buffer->next->current_size <= buffer->buffer_size / 2)) {
		merge_buffer = buffer->next;
	}
	if (merge_buffer) {
		assert(merge_buffer->start_line > buffer->start_line);
		assert(merge_buffer->current_size + buffer->current_size < buffer->buffer_size);
		// merge the next buffer into this buffer
		// update next/prev pointers
		buffer->next = merge_buffer->next;
		if (buffer->next) buffer->next->prev = buffer;
		// store the old buffer size for cursor updates
		lng old_size = buffer->current_size;
		// copy the content of merge_buffer into this buffer and update the size
		memcpy(buffer->buffer + buffer->current_size, merge_buffer->buffer, merge_buffer->current_size);
		buffer->current_size += merge_buffer->current_size;
		// finally delete merge_buffer from the buffer list
		lng bufpos = GetBuffer(buffers, merge_buffer->start_line);
		buffers.erase(buffers.begin() + bufpos);
		delete merge_buffer;
		return PGBufferUpdate(old_size, merge_buffer);
	} else {
		// no merges
		return PGBufferUpdate(size);
	}
}

lng PGTextBuffer::GetBuffer(std::vector<PGTextBuffer*>& buffers, lng line) {
	lng limit = buffers.size() - 1;
	lng first = 0;
	lng last = limit;
	lng middle = (first + last) / 2;
	// binary search to find the block that belongs to the buffer
	while (first <= last) {
		if (middle != limit && line >= buffers[middle + 1]->start_line) {
			first = middle + 1;
		} else if (line >= buffers[middle]->start_line &&
			(middle == limit || line < buffers[middle + 1]->start_line)) {
			return middle;
		} else {
			last = middle - 1;
		}
		middle = (first + last) / 2;
	}
	assert(0);
	return 0;
}

void PGTextBuffer::InsertText(ulng position, std::string text) {
	// this method can only be called if the text fits into the buffer
	assert(current_size + (lng)text.size() < buffer_size);
	assert(text.size() > 0);
	assert(position <= current_size);
	// first move the edge of the buffer to the right so we can fit the text into the buffer
	memmove(buffer + position + text.size(), buffer + position, current_size - position);
	// now insert the text into the buffer
	memcpy(buffer + position, text.c_str(), text.size());
	// increase the size of the buffer
	current_size += text.size();
}

void PGTextBuffer::DeleteText(ulng position, ulng size) {
	// text deletion is a right => deletion
	assert(position + size < current_size);
	assert(position >= 0);
	// move the text after the deletion backwards
	memmove(buffer + position, buffer + position + size, current_size - (position + size));
	// decrement the size
	current_size -= size;
}

lng PGTextBuffer::DeleteLines(ulng position, ulng end_position) {
	ulng size = end_position - position;
	lng lines = 0;
	for (lng i = position; i < position + size; i++) {
		if (buffer[i] == '\n') lines++;
	}
	this->DeleteText(position, size);
	return lines;
}

lng PGTextBuffer::DeleteLines(ulng position) {
	return DeleteLines(position, current_size - 1);
}

TextLine::TextLine(PGTextBuffer* buffer, lng line) {
	assert(line >= buffer->start_line);
	lng current_line = buffer->start_line;
	this->line = buffer->buffer;
	this->length = buffer->current_size;
	for (lng i = 0; i < buffer->current_size; ) {
		int offset = utf8_character_length(buffer->buffer[i]);
		if (offset == 1 && buffer->buffer[i] == '\n') {
			current_line++;
			if (current_line == line) {
				this->line = buffer->buffer + i + 1;
			} else if (current_line == line + 1) {
				this->length = (buffer->buffer + i) - this->line;
				break;
			}
		}
		i += offset;
	}
	if (buffer->syntax && buffer->parsed) {
		this->syntax = PGSyntax(buffer->syntax[line - buffer->start_line]);
	} else {
		this->syntax.end = -1;
	}
}

TextLineIterator::TextLineIterator(TextFile* textfile, lng line) {
	Initialize(textfile, line);
}

TextLineIterator::TextLineIterator() {

}

void TextLineIterator::Initialize(TextFile* textfile, lng line) {
	this->textfile = textfile;
	this->current_line = line;

	buffer = textfile->buffers[PGTextBuffer::GetBuffer(textfile->buffers, line)];

	lng current_line = buffer->start_line;
	assert(line >= current_line);
	textline.line = buffer->buffer;
	textline.length = buffer->current_size;
	for (lng i = 0; i < buffer->current_size; ) {
		int offset = utf8_character_length(buffer->buffer[i]);
		if (offset == 1 && buffer->buffer[i] == '\n') {
			current_line++;
			if (current_line == line) {
				start_position = i + 1;
				textline.line = buffer->buffer + i + 1;
			} else if (current_line == line + 1) {
				end_position = i;
				textline.length = (buffer->buffer + i) - textline.line;
				break;
			}
		}
		i += offset;
	}
	if (buffer->syntax && buffer->parsed) {
		textline.syntax = PGSyntax(buffer->syntax[line - buffer->start_line]);
	} else {
		textline.syntax.end = -1;
	}
}

TextLineIterator::TextLineIterator(TextFile* textfile, PGTextBuffer* buffer) :
	textfile(textfile) {
	this->buffer = buffer;
	this->current_line = buffer->start_line - 1;
	this->end_position = -1;
	this->start_position = 0;
	NextLine();
}


void TextLineIterator::PrevLine() {
	current_line--;
	if (current_line < 0) {
		current_line = 0;
		textline = TextLine();
		return;
	}
	if (current_line < buffer->start_line) {
		// have to look into previous buffer
		buffer = buffer->prev;
		assert(buffer);
		start_position = buffer->current_size - 1;
		end_position = buffer->current_size - 1;
	} else {
		end_position = start_position - 1;
	}
	// start at the current line and look for the previous newline character
	for (lng i = start_position - 2; i >= 0; i--) {
		if (buffer->buffer[i] == '\n') {
			start_position = i + 1;
			textline.line = buffer->buffer + start_position;
			textline.length = end_position - start_position;
			textline.syntax = buffer->parsed ? buffer->syntax[current_line - buffer->start_line] : PGSyntax();
			assert(textfile->GetLine(current_line).GetLine() == textline.line);
			assert(textfile->GetLine(current_line).GetLength() == textline.length);
			return;
		}
	}
	start_position = 0;

	textline.line = buffer->buffer + start_position;
	textline.length = end_position - start_position;
	textline.syntax = buffer->parsed ? buffer->syntax[current_line - buffer->start_line] : PGSyntax();

	assert(textfile->GetLine(current_line).GetLine() == textline.line);
	assert(textfile->GetLine(current_line).GetLength() == textline.length);
	// no newline in the buffer
	//assert(0);
}

void TextLineIterator::NextLine() {
	current_line++;
	if (current_line >= textfile->GetLineCount()) {
		current_line--;
		textline = TextLine();
		return;
	}
	if (buffer->next && current_line >= buffer->next->start_line) {
		// have to look into next buffer
		buffer = buffer->next;
		start_position = 0;
		end_position = 0;
	} else {
		end_position++;
		start_position = end_position;
	}
	// start at the current line and look for the next newline character
	for (lng i = end_position; i < buffer->current_size; i++) {
		if (buffer->buffer[i] == '\n') {
			end_position = i;
			textline.line = buffer->buffer + start_position;
			textline.length = end_position - start_position;
			textline.syntax = buffer->parsed ? buffer->syntax[current_line - buffer->start_line] : PGSyntax();
			return;
		}
	}
	assert(0);
}

TextLine TextLineIterator::GetLine() {
	return textline;
}

void SetTextBufferSize(lng bufsiz) {
	TEXT_BUFFER_SIZE = bufsiz;
}

lng PGTextBuffer::GetRenderedLines(TextFile* textfile, PGFontHandle font, PGScalar size) {
	lng rendered_lines = 0;
	for (auto it = TextLineIterator(textfile, this); it.CurrentBuffer() == this; it++) {
		TextLine line = it.GetLine();
		if (!line.IsValid()) break;
		rendered_lines += TextLine::RenderedLines(line.GetLine(), line.GetLength(), font, size);
	}
	return rendered_lines;
}
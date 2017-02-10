
#include "textbuffer.h"
#include "textdelta.h"
#include "textfile.h"
#include "unicode.h"

lng TEXT_BUFFER_SIZE = 250;


PGTextBuffer::PGTextBuffer() : 
	buffer(nullptr), buffer_size(0), current_size(0), start_line(0), 
	state(nullptr), syntax(nullptr), cumulative_width(0),
	width(0), line_count(0), index(0), wrap_width(0) {

}

PGTextBuffer::PGTextBuffer(const char* text, lng size, lng start_line) :
	current_size(size), start_line(start_line), state(nullptr), syntax(nullptr), 
	cumulative_width(0), width(0), line_count(0), index(0), wrap_width(0) {
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
	if (buffer) {
		delete buffer;
	}
}

lng PGTextBuffer::GetLineCount(lng total_lines) {
	return line_count;
}

double PGTextBuffer::GetTotalWidth(double total_width) {
	return width;
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
	if (current_line == line && current_character == position) {
		return 0;
	}
	ulng i = 0;
	for (i = 0; i < current_size; ) {
		if (current_line == line && current_character == position) {
			return i;
		}
		int offset = utf8_character_length(buffer[i]);
		if (offset == 1 && buffer[i] == '\n') {
			if (current_line == line) {
				return i;
			}
			current_character = 0;
			current_line++;
		} else {
			current_character += offset;
		}
		i += offset;
	}
	if (current_line == line) {
		return i;
	}
	assert(0);
	return 0;
}

PGCursorPosition PGTextBuffer::GetCursorFromPosition(ulng position, lng total_lines) {
	assert(position <= current_size);
	PGCursorPosition pos;
	pos.line = start_line;
	pos.position = 0;

	lng next_line = next ? next->start_line : total_lines;

	if (pos.line + 1 == next_line) {
		pos.position = position;
		return pos;
	}

	for (ulng i = 0; i < position; ) {
		int offset = utf8_character_length(buffer[i]);
		if (buffer[i] == '\n') {
			pos.line++;
			pos.position = 0;
			if (pos.line + 1 == next_line) {
				pos.position = position - i - 1;
				return pos;
			}
		} else {
			pos.position += offset;
		}
		i += offset;
	}
	return pos;
}

PGCharacterPosition PGTextBuffer::GetCharacterFromPosition(ulng position) {
	assert(position <= current_size);
	PGCharacterPosition pos;
	pos.line = start_line;
	pos.position = 0;
	pos.character = 0;
	ulng i = 0;

	lng cache_entry = TEXT_BUFFER_SIZE == 0 ? 0 : (position / TEXT_BUFFER_SIZE) - 1;
	if (cache_entry >= 0) {
		if (cache_entry < cached_positions.size()) {
			// cache entry exists
			pos = cached_positions[cache_entry];
			i = pos.position;
		} else if (cached_positions.size() > 0) {
			// no exact entry exists, but we can use the closest one
			cache_entry = cached_positions.size() - 1;
			pos = cached_positions.back();
			i = pos.position;
		} else {
			cache_entry = 0;
		}
	}

	for (; i < position; ) {
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
		if (TEXT_BUFFER_SIZE > 0 && i / TEXT_BUFFER_SIZE > cache_entry) {
			cache_entry++;
			if (cached_positions.size() <= cache_entry) {
				cached_positions.push_back(pos);	
			}
		}
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
		if (buffer->line_count == 1) {
			// there is only one line in the current buffer
			// that means we can't move lines out of the buffer to the next buffer
			// thus we extend the current buffer instead of creating new buffers
			lng new_size = std::max(buffer->buffer_size + buffer->buffer_size / 5, buffer->buffer_size + text.size() + 1);
			buffer->Extend(new_size);
			buffer->InsertText(position, text);
			return PGBufferUpdate(text.size());
		} else {
			lng buffer_position = PGTextBuffer::GetBuffer(buffers, buffer);

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
						// current_line is the amount of lines that will be in the new buffer
						// and hence also the amount of lines that will be removed from the current buffer
						PGTextBuffer* new_buffer = new PGTextBuffer(buffer->buffer + split_point, buffer->current_size - split_point, -1);
						if (buffer->next != nullptr) buffer->next->prev = new_buffer;
						new_buffer->next = buffer->next;
						new_buffer->prev = buffer;
						new_buffer->line_count = current_line;
						new_buffer->cumulative_width = -1;
						buffer->line_count -= current_line;
						buffer->current_size = split_point;
						buffer->next = new_buffer;
						buffers.insert(buffers.begin() + buffer_position + 1, new_buffer);
						new_buffer->start_line = buffer->start_line + buffer->line_count;
						for (lng k = buffer->line_count; k < buffer->line_start.size(); k++) {
							new_buffer->line_start.push_back(buffer->line_start[k] - split_point);
						}
						buffer->line_start.erase(buffer->line_start.begin() + buffer->line_count - 1, buffer->line_start.end());

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
	// this should never get used
	assert(0);
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
		lng bufpos = GetBuffer(buffers, merge_buffer);
		buffers.erase(buffers.begin() + bufpos);
		delete merge_buffer;
		return PGBufferUpdate(old_size, merge_buffer);
	} else {
		// no merges
		return PGBufferUpdate(size);
	}
}

lng PGTextBuffer::GetBufferFromWidth(std::vector<PGTextBuffer*>& buffers, double width) {
	lng limit = buffers.size() - 1;
	lng first = 0;
	lng last = limit;
	lng middle = (first + last) / 2;

	// binary search to find the block that belongs to the buffer
	while (first <= last) {
		if (middle != limit && width >= buffers[middle + 1]->cumulative_width) {
			first = middle + 1;
		} else if (width >= buffers[middle]->cumulative_width &&
			(middle == limit || width < buffers[middle + 1]->cumulative_width)) {
			return middle;
		} else {
			last = middle - 1;
		}
		middle = (first + last) / 2;
	}
	assert(0);
	return limit;
}

lng PGTextBuffer::GetBuffer(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer) {
	return buffer->index;
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
	lng start_line = GetStartLine(position);
	// have to update line_start for all subsequent lines
	for (size_t i = start_line; i < line_start.size(); i++) {
		line_start[i] += text.size();
	}
	// first move the edge of the buffer to the right so we can fit the text into the buffer
	memmove(buffer + position + text.size(), buffer + position, current_size - position);
	// now insert the text into the buffer
	memcpy(buffer + position, text.c_str(), text.size());
	// increase the size of the buffer
	current_size += text.size();
}

void PGTextBuffer::DeleteText(ulng position, ulng size) {
	lng start_line = GetStartLine(position);
	// have to update line_start for all subsequent lines
	for(size_t i = start_line; i < line_start.size(); i++) {
		line_start[i] -= size;
	}
	this->_DeleteText(position, size);
}

lng PGTextBuffer::DeleteLines(ulng position, ulng end_position) {
	lng start_line = GetStartLine(position);
	lng end_line = GetStartLine(end_position);
	if (end_line != start_line) {
		// remove any erased lines from line_start
		line_start.erase(line_start.begin() + start_line, line_start.begin() + end_line);	
	}
	lng size = end_position - position;
	// have to update line_start for all subsequent lines
	for(size_t i = start_line; i < line_start.size(); i++) {
		line_start[i] -= size;
	}
	this->_DeleteText(position, size);
	return end_line - start_line;
}

void PGTextBuffer::_DeleteText(ulng position, ulng size) {
	// text deletion is a right => deletion
	assert(position + size < current_size);
	assert(position >= 0);
	// move the text after the deletion backwards
	memmove(buffer + position, buffer + position + size, current_size - (position + size));
	// decrement the size
	current_size -= size;
}

lng PGTextBuffer::DeleteLines(ulng position) {
	return DeleteLines(position, current_size - 1);
}

lng PGTextBuffer::GetStartLine(lng position) {
	// FIXME: either binary search or non-branching loop here
	for(size_t i = 0; i < line_start.size(); i++) {
		if (line_start[i] > position) {
			return i;
		}
	}
	return line_start.size();
}

void PGTextBuffer::ClearWrappedLines() {
	line_wraps.clear();
	cached_positions.clear();
}

void PGTextBuffer::VerifyBuffer() {
#ifdef PANTHER_DEBUG
	assert(this->line_lengths.size() == this->line_count || this->cumulative_width < 0);
	lng current_line = 0;
	for (int i = 0; i < current_size - 1; i++) {
		if (buffer[i] == '\n') {
			assert(line_start.size() > current_line);
			assert(line_start[current_line] == i + 1);
			current_line++;
		}
	}
#endif
}

void SetTextBufferSize(lng bufsiz) {
	TEXT_BUFFER_SIZE = bufsiz;
}

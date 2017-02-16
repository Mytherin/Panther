
#include "textiterator.h"
#include "textfile.h"
#include "unicode.h"

TextLineIterator::TextLineIterator(TextFile* textfile, lng line) : 
	start_position(0) {
	Initialize(textfile, line);
}

TextLineIterator::TextLineIterator() : 
	start_position(0) {

}

void TextLineIterator::Initialize(TextFile* textfile, lng line) {
	this->textfile = textfile;
	this->current_line = line;
	this->start_position = 0;
	
	buffer = textfile->buffers[PGTextBuffer::GetBuffer(textfile->buffers, line)];
	end_position = buffer->current_size - 1;

	lng current_line = buffer->start_line;
	lng last_line = current_line + buffer->GetLineCount(textfile->GetLineCount());
	assert(line >= current_line);
	textline.line = buffer->buffer;
	textline.length = buffer->current_size - 1;
	// check if the buffer holds more than one line
	if (!(line == current_line && current_line + 1 == last_line)) {
		for (lng i = 0; i < buffer->current_size; ) {
			int offset = utf8_character_length(buffer->buffer[i]);
			if (offset == 1 && buffer->buffer[i] == '\n') {
				current_line++;
				if (current_line == line) {
					start_position = i + 1;
					textline.line = buffer->buffer + i + 1;
					if (current_line + 1 == last_line) {
						end_position = buffer->current_size - 1;
						textline.length = (buffer->buffer + end_position) - textline.line;
						// this is the last line in the buffer
						break;
					}
				} else if (current_line == line + 1) {
					end_position = i;
					textline.length = (buffer->buffer + i) - textline.line;
					break;
				}
			}
			i += offset;
		}
	}
	if (buffer->syntax.size() > 0 && buffer->parsed) {
		textline.syntax = &buffer->syntax[line - buffer->start_line];
	} else {
		textline.syntax = nullptr;
	}
}

TextLineIterator::TextLineIterator(TextFile* textfile, PGTextBuffer* buffer) :
	textfile(textfile) {
	this->buffer = buffer;
	this->current_line = (lng) buffer->start_line - 1;
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
	for (lng i = end_position - 1; i >= 0; i--) {
		if (buffer->buffer[i] == '\n') {
			start_position = i + 1;
			textline.line = buffer->buffer + start_position;
			textline.length = end_position - start_position;
			textline.syntax = buffer->parsed ? &buffer->syntax[current_line - buffer->start_line] : nullptr;
			assert(textfile->GetLine(current_line).GetLine() == textline.line);
			assert(textfile->GetLine(current_line).GetLength() == textline.length);
			return;
		}
	}
	start_position = 0;

	textline.line = buffer->buffer + start_position;
	textline.length = end_position - start_position;
	textline.syntax = buffer->parsed ? &buffer->syntax[current_line - buffer->start_line] : nullptr;

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
			textline.syntax = buffer->parsed ? &buffer->syntax[current_line - buffer->start_line] : nullptr;
			return;
		}
	}
	assert(0);
}

TextLine TextLineIterator::GetLine() {
	return textline;
}

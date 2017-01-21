
#include "cursor.h"
#include "textfile.h"
#include "textfield.h"
#include "text.h"
#include "unicode.h"

#include <algorithm>

Cursor::Cursor(TextFile* file) :
	file(file), x_position(-1) {
	start_buffer = file->GetBuffer(0);
	start_buffer_position = 0;
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}

Cursor::Cursor(TextFile* file, lng line, lng character) :
	file(file), x_position(-1) {
	start_buffer = file->GetBuffer(line);
	start_buffer_position = start_buffer->GetBufferLocationFromCursor(line, character);
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
	assert(start_buffer_position >= 0);
}

Cursor::Cursor(TextFile* file, lng start_line, lng start_character, lng end_line, lng end_character) :
	file(file), x_position(-1) {
	start_buffer = file->GetBuffer(start_line);
	start_buffer_position = start_buffer->GetBufferLocationFromCursor(start_line, start_character);
	end_buffer = file->GetBuffer(end_line);
	end_buffer_position = end_buffer->GetBufferLocationFromCursor(end_line, end_character);
	assert(start_buffer_position >= 0);
	assert(end_buffer_position >= 0);
}

Cursor::Cursor(TextFile* file, CursorData data) : 
   Cursor(file, data.start_line, data.start_position, data.end_line, data.end_position) {
}

CursorData Cursor::GetCursorData() {
	PGCursorPosition start = this->SelectedPosition();
	PGCursorPosition end = this->UnselectedPosition();
	return CursorData(start.line, start.position, end.line, end.position);
}

void Cursor::OffsetLine(lng offset) {
	OffsetSelectionLine(offset);
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}


void Cursor::OffsetSelectionLine(lng offset) {
	PGFontHandle textfield_font;
	if (!file->textfield) {
		textfield_font = PGCreateFont();
		SetTextFontSize(textfield_font, 15);
	} else {
		textfield_font = file->textfield->GetTextfieldFont();
	}
	PGCursorPosition sel = SelectedPosition();
	lng start_line = sel.line;
	lng start_character = sel.position;

	if (file->GetWordWrap()) {
		// the file has word wrapping enabled
		// when we press up/down, we might not go to the next line
		// but stay on the same line
		TextLine line = TextLine(start_buffer, start_line);
		lng start_wrap = 0;
		lng end_wrap = -1;
		bool wrap = false;
		if (offset > 0) {
			// we have the cursor down
			while (line.WrapLine(textfield_font, file->textfield->GetTextfieldWidth(), start_wrap, end_wrap)) {
				if (end_wrap > start_character) {
					// there is a wrap AFTER this cursor
					wrap = true;
					if (x_position < 0) {
						x_position = MeasureTextWidth(textfield_font, line.GetLine() + start_wrap, start_character - start_wrap);
					}
					break;
				}
				start_wrap = end_wrap;
			}
			if (wrap) {
				// if there is a wrap after this cursor
				// we move to a position WITHIN the current line (but in the next wrapped line)
				start_character = end_wrap + GetPositionInLine(textfield_font, x_position, line.GetLine() + end_wrap, line.GetLength() - end_wrap);
				_SetCursorStartLocation(start_line, start_character);
				return;
			} else {
				// if there is no wrap after this cursor
				// record the x_position and move to the next line
				if (x_position < 0) {
					x_position = MeasureTextWidth(textfield_font, line.GetLine() + start_wrap, start_character - start_wrap);
				}
			}
		} else {
			lng prev_wrap = -1;
			// we have to move the cursor up
			while (true) {
				bool found = line.WrapLine(textfield_font, file->textfield->GetTextfieldWidth(), start_wrap, end_wrap);
				if (end_wrap >= start_character) {
					wrap = true;
					break;
				}
				prev_wrap = start_wrap;
				start_wrap = end_wrap;
				if (!found) break;
			}
			if (wrap && start_wrap > 0) {
				// there is a wrap before this line
				// first measure the x-position of the cursor
				if (x_position < 0) {
					x_position = MeasureTextWidth(textfield_font, line.GetLine() + start_wrap, start_character - start_wrap);
				}
				start_character = prev_wrap + GetPositionInLine(textfield_font, x_position, line.GetLine() + prev_wrap, start_wrap - prev_wrap);
				_SetCursorStartLocation(start_line, start_character);
				return;
			} else {
				// we are on the first part of the currently wrapped line
				// however, the line ABOVE us might be wrapped as well
				// we want to move the cursor to the LAST part of that line
				if (x_position < 0) {
					x_position = MeasureTextWidth(textfield_font, TextLine(start_buffer, start_line).GetLine(), start_character);
				}

				lng new_line = std::min(std::max(start_line + offset, (lng)0), this->file->GetLineCount() - 1);
				if (new_line != start_line) {
					start_line = new_line;
					start_wrap = 0;
					line = this->file->GetLine(start_line);
					while (line.WrapLine(textfield_font, file->textfield->GetTextfieldWidth(), start_wrap, end_wrap)) {
						start_wrap = end_wrap;
					}
					start_character = GetPositionInLine(textfield_font, x_position, line.GetLine() + start_wrap, end_wrap - start_wrap);
					_SetCursorStartLocation(start_line, start_wrap + start_character);
				}
				return;
			}
		}
	}

	if (x_position < 0) {
		x_position = MeasureTextWidth(textfield_font, TextLine(start_buffer, start_line).GetLine(), start_character);
	}

	lng new_line = std::min(std::max(start_line + offset, (lng)0), this->file->GetLineCount() - 1);
	if (new_line != start_line) {
		start_line = new_line;
		TextLine line = this->file->GetLine(start_line);
		start_character = GetPositionInLine(textfield_font, x_position, line.GetLine(), line.GetLength());
		_SetCursorStartLocation(start_line, start_character);
	}
}

void Cursor::OffsetCharacter(PGDirection direction) {
	OffsetSelectionCharacter(direction);
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
	x_position = -1;
}

void Cursor::OffsetSelectionCharacter(PGDirection direction) {
	if (direction == PGDirectionLeft) {
		if (start_buffer_position <= 0) {
			// select previous buffer
			if (start_buffer->prev != nullptr) {
				start_buffer = start_buffer->prev;
				start_buffer_position = start_buffer->current_size - 1;
			}
		} else {
			start_buffer_position = utf8_prev_character(start_buffer->buffer, start_buffer_position);
		}
	} else if (direction == PGDirectionRight) {
		start_buffer_position += utf8_character_length(start_buffer->buffer[start_buffer_position]);
		if (start_buffer_position >= start_buffer->current_size) {
			// select next buffer
			if (start_buffer->next != nullptr) {
				start_buffer = start_buffer->next;
				start_buffer_position = 0;
			} else {
				start_buffer_position--;
			}
		}
	}
}


void Cursor::OffsetPosition(lng offset) {
	OffsetSelectionPosition(offset);
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
	x_position = -1;
}

void Cursor::OffsetSelectionPosition(lng offset) {
	start_buffer_position += offset;
	while (start_buffer_position < 0) {
		if (start_buffer->prev) {
			start_buffer = start_buffer->prev;
			start_buffer_position += start_buffer->current_size;
		} else {
			start_buffer_position = 0;
		}
	}
	while (start_buffer_position >= start_buffer->current_size) {
		if (start_buffer->next) {
			start_buffer_position -= start_buffer->current_size - 1;
			start_buffer = start_buffer->next;
		} else {
			start_buffer_position = start_buffer->current_size - 1;
		}
	}
}

void Cursor::OffsetStartOfLine() {
	SelectStartOfLine();
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}

void Cursor::OffsetEndOfLine() {
	SelectEndOfLine();
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}

void Cursor::OffsetSelectionWord(PGDirection direction) {
	if (direction == PGDirectionLeft && (start_buffer_position == 0 || start_buffer->buffer[start_buffer_position - 1] == '\n')) {
		OffsetSelectionCharacter(direction);
	} else if (direction == PGDirectionRight && (start_buffer_position == start_buffer->current_size - 1 || start_buffer->buffer[start_buffer_position] == '\n')) {
		OffsetSelectionCharacter(direction);
	} else {
		char* text = start_buffer->buffer + start_buffer_position;
		int offset = direction == PGDirectionLeft ? -1 : 1;
		if (direction == PGDirectionLeft && start_buffer_position > 0) text += offset;
		PGCharacterClass type = GetCharacterClass(*text);
		for (text += offset; (text > start_buffer->buffer && text < start_buffer->buffer + start_buffer->current_size); text += offset) {
			if (*text == '\n') {
				if (direction == PGDirectionLeft)
					text++;
				break;
			}
			if (GetCharacterClass(*text) != type) {
				text -= direction == PGDirectionLeft ? offset : 0;
				break;
			}
		}
		start_buffer_position = std::min((lng)start_buffer->current_size - 1, std::max((lng)0, (lng)(text - start_buffer->buffer)));
		assert(start_buffer_position >= 0);
	}
}

void Cursor::SelectWord() {
	char* start = start_buffer->buffer + start_buffer_position - (start_buffer_position > 0 ? 1 : 0);
	char* end = start_buffer->buffer + start_buffer_position;
	PGCharacterClass left_type = GetCharacterClass(*start);
	PGCharacterClass right_type = GetCharacterClass(*end);
	PGCharacterClass type = left_type == PGCharacterTypeText || right_type == PGCharacterTypeText ? PGCharacterTypeText : left_type;
	for (; start > start_buffer->buffer; start--) {
		if (*start == '\n') {
			start++;
			break;
		}
		if (GetCharacterClass(*start) != type) {
			start++;
			break;
		}
	}
	if (start_buffer_position < start_buffer->current_size) {
		for (; end < start_buffer->buffer + start_buffer->current_size; end++) {
			if (*end == '\n') break;
			if (GetCharacterClass(*end) != type) {
				break;
			}
		}
	}
	end_buffer = start_buffer;
	start_buffer_position = end - start_buffer->buffer;
	end_buffer_position = start - start_buffer->buffer;
	assert(start_buffer_position >= 0);
	assert(end_buffer_position >= 0);
}

std::string Cursor::GetSelectedWord() {
	std::string word;
	if (start_buffer != end_buffer) return word;
	if (start_buffer_position == end_buffer_position) return word;
	lng minpos = std::min(start_buffer_position, end_buffer_position);
	lng maxpos = std::max(start_buffer_position, end_buffer_position);
	for (lng i = minpos; i < maxpos; i++) {
		if (GetCharacterClass(start_buffer->buffer[i]) != PGCharacterTypeText) {
			return word;
		}
	}
	if (!(minpos == 0 || GetCharacterClass(start_buffer->buffer[minpos - 1]) != PGCharacterTypeText))
		return word;
	if (!(maxpos == start_buffer->current_size - 1 || GetCharacterClass(start_buffer->buffer[maxpos]) != PGCharacterTypeText))
		return word;

	return std::string(start_buffer->buffer + minpos, maxpos - minpos);
}

void Cursor::SelectLine() {
	TextLine line = start_buffer->GetLineFromPosition(start_buffer_position);
	end_buffer = start_buffer;
	end_buffer_position = line.GetLine() - end_buffer->buffer;
	start_buffer_position = line.GetLine() + line.GetLength() - start_buffer->buffer;
	OffsetSelectionCharacter(PGDirectionRight);
	x_position = -1;
}

void Cursor::OffsetWord(PGDirection direction) {
	OffsetSelectionWord(direction);
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}

void Cursor::SelectStartOfLine() {
	TextLine line = start_buffer->GetLineFromPosition(start_buffer_position);
	start_buffer_position = line.GetLine() - start_buffer->buffer;
}

void Cursor::SelectEndOfLine() {
	TextLine line = start_buffer->GetLineFromPosition(start_buffer_position);
	start_buffer_position = line.GetLine() + line.GetLength() - start_buffer->buffer;
}

void Cursor::OffsetStartOfFile() {
	SelectStartOfFile();
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}

void Cursor::OffsetEndOfFile() {
	SelectEndOfFile();
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
}

void Cursor::SelectStartOfFile() {
	start_buffer = file->buffers.front();
	start_buffer_position = 0;
}

void Cursor::SelectEndOfFile() {
	start_buffer = file->buffers.back();
	start_buffer_position = start_buffer->current_size - 1;
}

void Cursor::SetCursorLocation(lng linenr, lng characternr) {
	SetCursorStartLocation(linenr, characternr);
	end_buffer = start_buffer;
	end_buffer_position = start_buffer_position;
	x_position = -1;
}

void Cursor::_SetCursorStartLocation(lng linenr, lng characternr) {
	start_buffer = file->GetBuffer(linenr);
	start_buffer_position = start_buffer->GetBufferLocationFromCursor(linenr, characternr);
}

void Cursor::SetCursorStartLocation(lng linenr, lng characternr) {
	_SetCursorStartLocation(linenr, characternr);
	x_position = -1;
}

bool Cursor::SelectionIsEmpty() {
	return this->start_buffer == this->end_buffer && this->start_buffer_position == this->end_buffer_position;
}

PGCursorPosition Cursor::SelectedPosition() {
	return start_buffer->GetCursorFromPosition(start_buffer_position);
}

PGCursorPosition Cursor::UnselectedPosition() {
	return end_buffer->GetCursorFromPosition(end_buffer_position);
}

bool Cursor::CursorOccursFirst(Cursor* a, Cursor* b) {
	return (a->start_buffer->start_line < b->start_buffer->start_line ||
		(a->start_buffer->start_line == b->start_buffer->start_line && a->start_buffer_position < b->start_buffer_position));
}

PGScalar Cursor::GetXOffset(CursorPosition position) {
	TextLine line = position.buffer->GetLineFromPosition(position.position);
	lng line_position = position.buffer->buffer + position.position - line.GetLine();
	auto textfield_font = file->textfield->GetTextfieldFont();
	if (file->GetWordWrap()) {
		assert(0);
		return -1;
	} else {
		return MeasureTextWidth(textfield_font, line.GetLine(), line_position);
	}
}

PGCursorPosition Cursor::BeginPosition() {
	CursorPosition begin = BeginCursorPosition();
	return begin.buffer->GetCursorFromPosition(begin.position);
}

PGCursorPosition Cursor::EndPosition() {
	CursorPosition end = EndCursorPosition();
	return end.buffer->GetCursorFromPosition(end.position);
}

PGScalar Cursor::SelectedXPosition() {
	return GetXOffset(CursorPosition(start_buffer, start_buffer_position));
}

PGScalar Cursor::BeginXPosition() {
	return GetXOffset(BeginCursorPosition());
}

PGScalar Cursor::EndXPosition() {
	return GetXOffset(EndCursorPosition());
}

std::string Cursor::GetLine() {
	TextLine line = start_buffer->GetLineFromPosition(start_buffer_position);
	return std::string(line.GetLine(), line.GetLength());
}

std::string Cursor::GetText() {
	std::string text = "";
	auto start = BeginCursorPosition();
	auto end = EndCursorPosition();
	PGTextBuffer* begin_buffer = start.buffer;
	lng begin_pos = start.position;
	PGTextBuffer* final_buffer = end.buffer;
	lng final_pos = end.position;
	if (begin_buffer == final_buffer) {
		text = text + std::string(begin_buffer->buffer + begin_pos, final_pos - begin_pos);
	} else {
		text += std::string(begin_buffer->buffer + begin_pos, begin_buffer->current_size - begin_pos);
		begin_buffer = begin_buffer->next;
		while (begin_buffer != final_buffer) {
			text += std::string(begin_buffer->buffer, begin_buffer->current_size);
			begin_buffer = begin_buffer->next;
		}
		text += std::string(final_buffer->buffer, final_pos);
	}
	return text;
}

bool Cursor::CursorPositionOccursFirst(PGTextBuffer* a, lng a_pos, PGTextBuffer* b, lng b_pos) {
	return a->start_line < b->start_line ||
		(a->start_line == b->start_line && a_pos < b_pos);
}

CursorPosition Cursor::BeginCursorPosition() {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		return CursorPosition(start_buffer, start_buffer_position);
	} else {
		return CursorPosition(end_buffer, end_buffer_position);
	}
}

CursorPosition Cursor::EndCursorPosition() {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		return CursorPosition(end_buffer, end_buffer_position);
	} else {
		return CursorPosition(start_buffer, start_buffer_position);
	}
}


bool Cursor::OverlapsWith(Cursor* cursor) {
	CursorPosition begin_position = BeginCursorPosition();
	CursorPosition cursor_begin_position = cursor->BeginCursorPosition();
	CursorPosition end_position = EndCursorPosition();
	CursorPosition cursor_end_position = cursor->EndCursorPosition();

	if (begin_position <= cursor_end_position && end_position >= cursor_begin_position) {
		return true;
	}
	if (cursor_begin_position <= end_position && cursor_end_position >= begin_position) {
		return true;
	}
	return false;
}

void Cursor::Merge(Cursor* cursor) {
	CursorPosition begin_position = BeginCursorPosition();
	CursorPosition cursor_begin_position = cursor->BeginCursorPosition();
	if (begin_position.buffer == cursor_begin_position.buffer) {
		begin_position.position = std::min(begin_position.position, cursor_begin_position.position);
	} else if (begin_position.buffer->start_line > cursor_begin_position.buffer->start_line) {
		begin_position.buffer = cursor_begin_position.buffer;
		begin_position.position = cursor_begin_position.position;
	}

	CursorPosition end_position = EndCursorPosition();
	CursorPosition cursor_end_position = cursor->EndCursorPosition();
	if (end_position.buffer == cursor_end_position.buffer) {
		end_position.position = std::max(end_position.position, cursor_end_position.position);
	} else if (end_position.buffer->start_line < cursor_end_position.buffer->start_line) {
		end_position.buffer = cursor_end_position.buffer;
		end_position.position = cursor_end_position.position;
	}

	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		start_buffer = begin_position.buffer;
		start_buffer_position = begin_position.position;
		end_buffer = end_position.buffer;
		end_buffer_position = end_position.position;
	} else {
		start_buffer = end_position.buffer;
		start_buffer_position = end_position.position;
		end_buffer = begin_position.buffer;
		end_buffer_position = begin_position.position;
	}
}

void Cursor::NormalizeCursors(TextFile* textfile, std::vector<Cursor*>& cursors, bool scroll_textfield) {
	for (size_t i = 0; i < cursors.size() - 1; i++) {
		int j = i + 1;
		if (cursors[i]->OverlapsWith(cursors[j])) {
			cursors[i]->Merge(cursors[j]);
			delete cursors[j];
			cursors.erase(cursors.begin() + j);
			i--;
		}
	}
	if (scroll_textfield && textfile->textfield) {
		// FIXME: cursor_line should now be double as well for scrolling purposes
		/*
		lng line_offset = textfile->GetLineOffset();
		lng line_height = textfile->GetLineHeight();
		lng line_start = line_offset;
		lng line_end = line_start + line_height;
		lng cursor_min = INT_MAX, cursor_max = 0;
		bool word_wrap = textfile->GetWordWrap();
		PGScalar cursor_min_character = INT_MAX, cursor_max_character = 0;
		for (int i = 0; i < cursors.size(); i++) {
			lng cursor_line = cursors[i]->SelectedPosition().line;
			cursor_min = std::min(cursor_min, cursor_line);
			cursor_max = std::max(cursor_max, cursor_line);
			if (!word_wrap) {
				PGScalar cursor_position = cursors[i]->SelectedXPosition();
				cursor_min_character = std::min(cursor_min_character, cursor_position);
				cursor_max_character = std::max(cursor_max_character, cursor_position);
			}
		}

		if (cursor_min >= line_end) {
			// cursor is located past the end of what is visible, offset the view
			line_offset = cursor_min - line_height;
		} else if (cursor_max < line_start) {
			// cursor is located before the start of what is visible, offset the view
			line_offset = cursor_max;
		}
		textfile->SetLineOffset(line_offset);

		if (!word_wrap) {
			PGScalar xoffset = textfile->GetXOffset();
			PGScalar max_textwidth = textfile->textfield->GetTextfieldWidth() - 20;
			if (cursor_max_character < xoffset) {
				xoffset = cursor_max_character;
			} else if (cursor_min_character > xoffset + max_textwidth) {
				xoffset = cursor_min_character - max_textwidth;
			}
			textfile->SetXOffset(std::max(0.0f, std::min(xoffset, textfile->textfield->GetMaxXOffset())));
		}*/
		if (textfile->textfield) {
			textfile->textfield->SelectionChanged();
		}
	}
}

CursorSelection Cursor::GetCursorSelection() {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		return CursorSelection(start_buffer, start_buffer_position, end_buffer, end_buffer_position);
	} else {
		return CursorSelection(end_buffer, end_buffer_position, start_buffer, start_buffer_position);
	}
}

void Cursor::ApplyMinimalSelection(CursorSelection selection) {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		if (!CursorPositionOccursFirst(start_buffer, start_buffer_position, 
			selection.begin.buffer, selection.begin.position)) {
			start_buffer = selection.begin.buffer;
			start_buffer_position = selection.begin.position;
		}
		if (!CursorPositionOccursFirst(selection.end.buffer, selection.end.position,
				end_buffer, end_buffer_position)) {
			end_buffer = selection.end.buffer;
			end_buffer_position = selection.end.position;
		}
	} else {
		if (!CursorPositionOccursFirst(end_buffer, end_buffer_position, 
			selection.begin.buffer, selection.begin.position)) {
			end_buffer = selection.begin.buffer;
			end_buffer_position = selection.begin.position;
		}
		if (!CursorPositionOccursFirst(selection.end.buffer, selection.end.position,
			start_buffer, start_buffer_position)) {
			start_buffer = selection.end.buffer;
			start_buffer_position = selection.end.position;
		}
	}
}
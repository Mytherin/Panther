
#include "cursor.h"
#include "textfile.h"
#include "textfield.h"
#include "text.h"
#include "unicode.h"
#include "textiterator.h"

#include <algorithm>

void Cursor::StoreCursors(nlohmann::json& j, std::vector<CursorData>& cursors) {
	j["cursors"] = nlohmann::json::array();
	nlohmann::json& cursor = j["cursors"];
	lng index = 0;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		cursor[index] = nlohmann::json::array();
		nlohmann::json& current_cursor = cursor[index];
		current_cursor[0] = it->start_line;
		current_cursor[1] = it->start_position;
		current_cursor[2] = it->end_line;
		current_cursor[3] = it->end_position;
		index++;
	}
}

void Cursor::LoadCursors(nlohmann::json& j, std::vector<CursorData>& stored_cursors) {
	if (j.count("cursors") > 0) {
		nlohmann::json& cursors = j["cursors"];
		if (cursors.is_array()) {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				int size = it->size();
				if (it->is_array() && it->size() == 4 &&
					(*it)[0].is_number() && (*it)[1].is_number() && (*it)[2].is_number() && (*it)[3].is_number()) {

					int start_line = (*it)[0];
					int start_pos = (*it)[1];
					int end_line = (*it)[2];
					int end_pos = (*it)[3];

					stored_cursors.push_back(CursorData(start_line, start_pos, end_line, end_pos));
				}
			}
		}
	}
}

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

CursorData Cursor::GetCursorData() const {
	PGCursorPosition start = this->SelectedPosition();
	PGCursorPosition end = this->UnselectedPosition();
	return CursorData(start.line, start.position, end.line, end.position);
}

std::vector<CursorData> Cursor::GetCursorData(const std::vector<Cursor>& cursors) {
	std::vector<CursorData> data;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		data.push_back(it->GetCursorData());
	}
	return data;
}

Cursor::Cursor(TextFile* file, PGTextRange range) : 
	file(file), x_position(-1), 
	start_buffer(range.start_buffer), 
	start_buffer_position(range.start_position), 
	end_buffer(range.end_buffer),
	end_buffer_position(range.end_position)
{

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
		TextLine line = TextLine(start_buffer, start_line, file->GetLineCount());
		lng start_wrap = 0;
		lng end_wrap = -1;
		bool wrap = false;
		if (offset > 0) {
			lng* lines = line.WrapLine(start_buffer, start_line, file->GetLineCount(), textfield_font, file->wordwrap_width);
			start_wrap = 0;
			lng index = 0;
			// we have the cursor down
			while (lines[index] < line.GetLength()) {
				end_wrap = lines[index];
				if (end_wrap > start_character) {
					// there is a wrap AFTER this cursor
					wrap = true;
					if (x_position < 0) {
						x_position = MeasureTextWidth(textfield_font, line.GetLine() + start_wrap, start_character - start_wrap);
					}
					break;
				}
				start_wrap = end_wrap;
				index++;
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
			lng* lines = line.WrapLine(start_buffer, start_line, file->GetLineCount(), textfield_font, file->wordwrap_width);
			start_wrap = 0;
			lng index = 0;
			lng prev_wrap = -1;
			// we have to move the cursor up
			while (true) {
				end_wrap = lines[index];
				bool found = lines[index] < line.GetLength();
				if (end_wrap >= start_character) {
					wrap = true;
					break;
				}
				prev_wrap = start_wrap;
				start_wrap = end_wrap;
				index++;
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
					x_position = MeasureTextWidth(textfield_font, TextLine(start_buffer, start_line, file->GetLineCount()).GetLine(), start_character);
				}

				lng new_line = std::min(std::max(start_line + offset, (lng)0), this->file->GetLineCount() - 1);
				if (new_line != start_line) {
					start_line = new_line;
					start_wrap = 0;
					auto buffer = this->file->GetBuffer(start_line);
					line = this->file->GetLine(start_line);
					lng* lines = line.WrapLine(buffer, start_line, file->GetLineCount(), textfield_font, file->wordwrap_width);
					lng rendered_lines = line.RenderedLines(buffer, start_line, file->GetLineCount(), textfield_font, file->wordwrap_width);
					start_wrap = rendered_lines > 1 ? lines[rendered_lines - 2] : 0;
					end_wrap = lines[rendered_lines - 1];
					start_character = GetPositionInLine(textfield_font, x_position, line.GetLine() + start_wrap, end_wrap - start_wrap);
					_SetCursorStartLocation(start_line, start_wrap + start_character);
				}
				return;
			}
		}
	}

	if (x_position < 0) {
		x_position = MeasureTextWidth(textfield_font, TextLine(start_buffer, start_line, file->GetLineCount()).GetLine(), start_character);
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

std::string Cursor::GetSelectedWord() const {
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

void Cursor::SetCursorLocation(PGTextRange range) {
	start_buffer = range.start_buffer;
	start_buffer_position = range.start_position;
	end_buffer = range.end_buffer;
	end_buffer_position = range.end_position;
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

bool Cursor::SelectionIsEmpty() const {
	return this->start_buffer == this->end_buffer && this->start_buffer_position == this->end_buffer_position;
}

PGCursorPosition Cursor::SelectedPosition() const {
	return start_buffer->GetCursorFromPosition(start_buffer_position, file->GetLineCount());
}

PGCursorPosition Cursor::UnselectedPosition() const {
	return end_buffer->GetCursorFromPosition(end_buffer_position, file->GetLineCount());
}

PGCursorPosition Cursor::BeginPosition() const {
	PGTextPosition begin = BeginCursorPosition();
	return begin.buffer->GetCursorFromPosition(begin.position, file->GetLineCount());
}

PGCursorPosition Cursor::EndPosition() const {
	PGTextPosition end = EndCursorPosition();
	return end.buffer->GetCursorFromPosition(end.position, file->GetLineCount());
}

PGCharacterPosition Cursor::SelectedCharacterPosition() const {
	return start_buffer->GetCharacterFromPosition(start_buffer_position);
}

PGCharacterPosition Cursor::BeginCharacterPosition() const {
	PGTextPosition begin = BeginCursorPosition();
	return begin.buffer->GetCharacterFromPosition(begin.position);
}

PGCharacterPosition Cursor::EndCharacterPosition() const {
	PGTextPosition end = EndCursorPosition();
	return end.buffer->GetCharacterFromPosition(end.position);
}

bool Cursor::CursorOccursFirst(const Cursor& a, const Cursor& b) {
	return (a.start_buffer->start_line < b.start_buffer->start_line ||
		(a.start_buffer->start_line == b.start_buffer->start_line && a.start_buffer_position < b.start_buffer_position));
}

PGScalar Cursor::GetXOffset(PGTextPosition position) {
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

PGScalar Cursor::SelectedXPosition() {
	return GetXOffset(PGTextPosition(start_buffer, start_buffer_position));
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

PGTextPosition Cursor::BeginCursorPosition() const {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		return PGTextPosition(start_buffer, start_buffer_position);
	} else {
		return PGTextPosition(end_buffer, end_buffer_position);
	}
}

PGTextPosition Cursor::EndCursorPosition() const {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		return PGTextPosition(end_buffer, end_buffer_position);
	} else {
		return PGTextPosition(start_buffer, start_buffer_position);
	}
}

PGTextPosition Cursor::SelectedCursorPosition() const {
	return PGTextPosition(start_buffer, start_buffer_position);
}

bool Cursor::OverlapsWith(const Cursor& cursor) const {
	PGTextPosition begin_position = BeginCursorPosition();
	PGTextPosition cursor_begin_position = cursor.BeginCursorPosition();
	PGTextPosition end_position = EndCursorPosition();
	PGTextPosition cursor_end_position = cursor.EndCursorPosition();

	if (begin_position <= cursor_end_position && end_position >= cursor_begin_position) {
		return true;
	}
	if (cursor_begin_position <= end_position && cursor_end_position >= begin_position) {
		return true;
	}
	return false;
}

void Cursor::Merge(const Cursor& cursor) {
	PGTextPosition begin_position = BeginCursorPosition();
	PGTextPosition cursor_begin_position = cursor.BeginCursorPosition();
	if (begin_position.buffer == cursor_begin_position.buffer) {
		begin_position.position = std::min(begin_position.position, cursor_begin_position.position);
	} else if (begin_position.buffer->start_line > cursor_begin_position.buffer->start_line) {
		begin_position.buffer = cursor_begin_position.buffer;
		begin_position.position = cursor_begin_position.position;
	}

	PGTextPosition end_position = EndCursorPosition();
	PGTextPosition cursor_end_position = cursor.EndCursorPosition();
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

void Cursor::NormalizeCursors(TextFile* textfile, std::vector<Cursor>& cursors, bool scroll_textfield) {
	for (size_t i = 0; i < cursors.size() - 1; i++) {
		int j = i + 1;
		if (cursors[i].OverlapsWith(cursors[j])) {
			cursors[i].Merge(cursors[j]);
			cursors.erase(cursors.begin() + j);
			i--;
		}
	}
	if (scroll_textfield && textfile->textfield) {
		PGVerticalScroll line_offset = textfile->GetLineOffset();
		lng line_height = textfile->GetLineHeight();
		PGVerticalScroll end_scroll = PGVerticalScroll(std::min(textfile->GetLineCount() - 1, line_offset.linenumber + line_height), 0);
		bool word_wrap = textfile->GetWordWrap();
		if (word_wrap) {
			auto it = textfile->GetScrollIterator(textfile->textfield, line_offset);
			for (lng i = 0; i < line_height; i++, (*it)++) {
				if (!(*it).GetLine().IsValid()) {
					break;
				}
			}
			end_scroll = (*it).GetCurrentScrollOffset();
		}
		PGCursorPosition cursor_min_position = cursors[0].SelectedPosition();
		PGCursorPosition cursor_max_position = cursor_min_position;
		PGScalar cursor_min_xoffset = word_wrap ? 0 : cursors[0].SelectedXPosition();
		PGScalar cursor_max_xoffset = cursor_min_xoffset;
		for (int i = 1; i < cursors.size(); i++) {
			auto position = cursors[i].SelectedPosition();
			if (position.line < cursor_min_position.line ||
				(position.line == cursor_min_position.line && position.position < cursor_min_position.position)) {
				cursor_min_position = position;
			}
			if (position.line > cursor_max_position.line ||
				(position.line == cursor_max_position.line && position.position > cursor_max_position.position)) {
				cursor_max_position = position;
			}
			if (!word_wrap) {
				PGScalar cursor_position = cursors[i].SelectedXPosition();
				cursor_min_xoffset = std::min(cursor_min_xoffset, cursor_position);
				cursor_max_xoffset = std::max(cursor_max_xoffset, cursor_position);
			}
		}
		PGVerticalScroll min_scroll = textfile->GetVerticalScroll(cursor_min_position.line, cursor_min_position.position);
		PGVerticalScroll max_scroll = textfile->GetVerticalScroll(cursor_max_position.line, cursor_max_position.position);

		if (max_scroll < line_offset) {
			// cursor is located past the end of what is visible, offset the view
			line_offset = max_scroll;
		} else
		if (min_scroll > end_scroll) {
			// cursor is located before the start of what is visible, offset the view
			if (!word_wrap) {
				line_offset = PGVerticalScroll(std::max((lng) 0, min_scroll.linenumber - line_height), 0);
			} else {
				lng count = line_height;
				auto it = textfile->GetScrollIterator(textfile->textfield, min_scroll);
				for (; ; (*it)--) {
					if (!(*it).GetLine().IsValid()) {
						break;
					}
					if (count == 0)
						break;
					count--;
				}
				line_offset = it->GetCurrentScrollOffset();
			}
		}
		textfile->SetLineOffset(line_offset);

		if (!word_wrap) {
			PGScalar xoffset = textfile->GetXOffset();
			PGScalar max_textwidth = textfile->textfield->GetTextfieldWidth() - 20;
			if (cursor_max_xoffset < xoffset) {
				xoffset = cursor_max_xoffset;
			} else if (cursor_min_xoffset > xoffset + max_textwidth) {
				xoffset = cursor_min_xoffset - max_textwidth;
			}
			textfile->SetXOffset(std::max(0.0f, std::min(xoffset, textfile->textfield->GetMaxXOffset())));
		}
		if (textfile->textfield) {
			textfile->textfield->SelectionChanged();
		}
	}
}

PGTextRange Cursor::GetCursorSelection() const {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		return PGTextRange(start_buffer, start_buffer_position, end_buffer, end_buffer_position);
	} else {
		return PGTextRange(end_buffer, end_buffer_position, start_buffer, start_buffer_position);
	}
}

void Cursor::ApplyMinimalSelection(PGTextRange selection) {
	if (CursorPositionOccursFirst(start_buffer, start_buffer_position, end_buffer, end_buffer_position)) {
		if (!CursorPositionOccursFirst(start_buffer, start_buffer_position,
			selection.start_buffer, selection.start_position)) {
			start_buffer = selection.start_buffer;
			start_buffer_position = selection.start_position;
		}
		if (!CursorPositionOccursFirst(selection.end_buffer, selection.end_position,
			end_buffer, end_buffer_position)) {
			end_buffer = selection.end_buffer;
			end_buffer_position = selection.end_position;
		}
	} else {
		if (!CursorPositionOccursFirst(end_buffer, end_buffer_position,
			selection.start_buffer, selection.start_position)) {
			end_buffer = selection.start_buffer;
			end_buffer_position = selection.start_position;
		}
		if (!CursorPositionOccursFirst(selection.end_buffer, selection.end_position,
			start_buffer, start_buffer_position)) {
			start_buffer = selection.end_buffer;
			start_buffer_position = selection.end_position;
		}
	}
}
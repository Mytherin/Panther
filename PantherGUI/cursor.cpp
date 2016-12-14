
#include "cursor.h"
#include "textfile.h"
#include "textfield.h"
#include "text.h"

#include <algorithm>

Cursor::Cursor(TextFile* file) :
	file(file), start_line(0), start_character(0), end_line(0), end_character(0), min_character(-1) {

}

Cursor::Cursor(TextFile* file, lng line, lng character) :
	file(file), start_line(line), start_character(character), end_line(line), end_character(character), min_character(-1) {
}

void Cursor::OffsetLine(lng offset) {
	OffsetSelectionLine(offset);
	end_line = start_line;
	end_character = start_character;
	min_character = -1;
}

void Cursor::OffsetSelectionLine(lng offset) {
	start_line = std::min(std::max(start_line + offset, (lng)0), this->file->GetLineCount() - 1);
	start_character = std::min(start_character, file->GetLine(start_line)->GetLength());
}

void Cursor::OffsetCharacter(lng offset) {
	OffsetSelectionCharacter(offset);
	end_character = start_character;
	end_line = start_line;
	min_character = -1;
}

void Cursor::OffsetSelectionCharacter(lng offset) {
	start_character += offset;
	while (start_character < 0) {
		if (start_line > 0) {
			start_line--;
			start_character += this->file->GetLine(start_line)->GetLength() + 1;
		} else {
			start_character = 0;
			break;
		}
	}
	while (start_character > this->file->GetLine(start_line)->GetLength()) {
		if (start_line < this->file->GetLineCount() - 1) {
			start_character -= this->file->GetLine(start_line)->GetLength() + 1;
			start_line++;
		} else {
			start_character = this->file->GetLine(start_line)->GetLength();
			break;
		}
	}
}

void Cursor::OffsetStartOfLine() {
	SelectStartOfLine();
	this->end_character = this->start_character;
	this->end_line = this->start_line;
}

void Cursor::OffsetEndOfLine() {
	SetCursorLocation(this->start_line, this->file->GetLine(start_line)->GetLength());
}

void Cursor::OffsetSelectionWord(PGDirection direction) {
	if (direction == PGDirectionLeft && this->SelectedCharacter() == 0) {
		OffsetSelectionCharacter(-1);
	} else if (direction == PGDirectionRight && start_character == this->file->GetLine(start_line)->GetLength()) {
		OffsetSelectionCharacter(1);
	} else {
		TextLine* textline = this->file->GetLine(start_line);
		char* line = textline->GetLine();
		lng length = textline->GetLength();
		int offset = direction == PGDirectionLeft ? -1 : 1;
		lng index = direction == PGDirectionLeft ? start_character + offset : start_character;
		PGCharacterClass type = GetCharacterClass(line[index]);
		for (index += offset; index >= 0 && index < length; index += offset) {
			if (GetCharacterClass(line[index]) != type) {
				index -= direction == PGDirectionLeft ? offset : 0;
				break;
			}
		}
		index = std::min(std::max(index, (lng)0), textline->GetLength());
		start_character = index;
	}
}

void Cursor::SelectWord() {
	TextLine* textline = this->file->GetLine(start_line);
	char* line = textline->GetLine();
	lng start_index = std::max(start_character - 1, (lng)0);
	lng end_index = start_index + 1;
	PGCharacterClass left_type = GetCharacterClass(line[start_index]);
	PGCharacterClass right_type = GetCharacterClass(line[end_index]);
	PGCharacterClass type = left_type == PGCharacterTypeText || right_type == PGCharacterTypeText ? PGCharacterTypeText : left_type;
	for (end_index = start_index + 1; end_index < textline->GetLength(); end_index++) {
		if (GetCharacterClass(line[end_index]) != type) {
			break;
		}
	}
	for (; start_index >= 0; start_index--) {
		if (GetCharacterClass(line[start_index]) != type) {
			start_index++;
			break;
		}
	}
	end_character = std::max(start_index, (lng)0);
	start_character = std::min(end_index, textline->GetLength());
	end_line = start_line;
	min_character = this->BeginCharacter();
	min_line = this->BeginLine();
	max_character = this->EndCharacter();
	max_line = this->EndLine();
}

int Cursor::GetSelectedWord(lng& word_start, lng& word_end) {
	// returns the currently selected word, if any
	if (start_line != end_line) return -1;

	TextLine* textline = this->file->GetLine(start_line);
	char* line = textline->GetLine();
	lng length = textline->GetLength();
	// select the word from the start character
	// note that words in this context can only contain PGCharacterTypeText
	// unlike word selections which can contain any type as long as all characters have the same type
	lng start_index = std::max(start_character - 1, (lng)0);
	lng end_index = start_index + 1;
	PGCharacterClass type = PGCharacterTypeText;
	for (end_index = start_index + 1; end_index < length; end_index++) {
		if (GetCharacterClass(line[end_index]) != type) {
			break;
		}
	}
	for (; start_index >= 0; start_index--) {
		if (GetCharacterClass(line[start_index]) != type) {
			start_index++;
			break;
		}
	}
	start_index = std::max(start_index, (lng)0);
	end_index = std::min(end_index, length);
	if (end_index <= start_index) {
		// no word found
		return -1;
	}
	if (start_index != BeginCharacter() || end_index != EndCharacter()) {
		// only highlight if the entire word is selected
		return -1;
	}
	if (end_character < start_index || end_character > end_index) {
		// the end of the selection does not contain the word: no word selected
		return -1;
	}
	word_start = start_index;
	word_end = end_index;
	return 0;

}

void Cursor::SelectLine() {
	this->end_character = this->start_character = 0;
	this->end_line = this->start_line;
	if (this->start_line != file->GetLineCount() - 1) {
		this->start_line++;
	} else {
		this->start_character = file->GetLine(this->start_line)->GetLength();
	}
	min_character = this->BeginCharacter();
	min_line = this->BeginLine();
	max_character = this->EndCharacter();
	max_line = this->EndLine();
}

void Cursor::OffsetWord(PGDirection direction) {
	OffsetSelectionWord(direction);
	end_character = start_character;
	end_line = start_line;
}

void Cursor::SelectStartOfLine() {
	if (this->start_character == 0) return;
	char* line = this->file->GetLine(this->start_line)->GetLine();
	lng character = 0;
	while (*line == ' ' || *line == '\t') {
		character++;
		line++;
	}
	if (this->start_character <= character) {
		this->start_character = 0;
	} else {
		this->start_character = character;
	}
}

void Cursor::SelectEndOfLine() {
	this->start_character = this->file->GetLine(start_line)->GetLength();
}

void Cursor::OffsetStartOfFile() {
	this->start_character = this->end_character = 0;
	this->start_line = this->end_line = 0;
}

void Cursor::OffsetEndOfFile() {
	this->start_line = this->end_line = this->file->GetLineCount() - 1;
	this->start_character = this->end_character = this->file->GetLine(this->start_line)->GetLength();
}

void Cursor::SelectStartOfFile() {
	this->start_character = 0;
	this->start_line = 0;
}

void Cursor::SelectEndOfFile() {
	this->start_line = this->file->GetLineCount() - 1;
	this->start_character = this->file->GetLine(this->start_line)->GetLength();
}

void Cursor::SetCursorLocation(lng linenr, lng characternr) {
	this->start_line = this->end_line = std::max(std::min((lng)  linenr, this->file->GetLineCount() - 1), (lng) 0);
	this->start_character = this->end_character = std::max(std::min((lng) characternr, file->GetLine(start_line)->GetLength()), (lng) 0);
	min_character = -1;
}

void Cursor::SetCursorStartLocation(lng linenr, lng characternr) {
	if (min_character >= 0) {
		if (linenr < min_line ||
			(linenr == min_line && characternr < min_character)) {
			this->end_character = max_character;
			this->end_line = max_line;
		} else if (linenr > max_line ||
			(linenr == max_line && characternr > max_character)) {
			this->end_character = min_character;
			this->end_line = min_line;
		} else if (linenr == min_line && characternr > min_character) {
			if (characternr > min_character + (max_character - min_character) / 2) {
				this->start_line = max_line;
				this->start_character = max_character;
				this->end_line = min_line;
				this->end_character = min_character;
			} else {
				this->start_line = min_line;
				this->start_character = min_character;
				this->end_line = max_line;
				this->end_character = max_character;
			}
			return;
		}
	}
	this->start_line = std::max(std::min((lng)  linenr, this->file->GetLineCount() - 1), (lng) 0);
	this->start_character = std::max(std::min((lng) characternr, file->GetLine(start_line)->GetLength()), (lng) 0);
}

void Cursor::SetCursorEndLocation(lng linenr, lng characternr) {
	this->end_line = std::max(std::min((lng)  linenr, this->file->GetLineCount() - 1), (lng) 0);
	this->end_character = std::max(std::min((lng) characternr, file->GetLine(start_line)->GetLength()), (lng) 0);
}

void Cursor::SetCursorLine(lng linenr) {
	this->start_line = this->end_line = std::max(std::min((lng)  linenr, this->file->GetLineCount() - 1), (lng) 0);
}

void Cursor::SetCursorCharacter(lng characternr) {
	this->start_character = this->end_character = std::max(std::min((lng) characternr, file->GetLine(start_line)->GetLength()), (lng) 0);
}

void Cursor::RestoreCursor(Cursor cursor) {
	this->start_line = cursor.start_line;
	this->start_character = cursor.start_character;
	this->end_line = cursor.end_line;
	this->end_character = cursor.end_character;
}

bool Cursor::SelectionIsEmpty() {
	return this->start_character == this->end_character && this->start_line == this->end_line;
}

lng Cursor::BeginCharacter() {
	if (start_line < end_line) return start_character;
	if (end_line < start_line) return end_character;
	return std::min(start_character, end_character);
}

lng Cursor::BeginLine() {
	return std::min(start_line, end_line);
}

lng Cursor::EndCharacter() {
	if (start_line < end_line) return end_character;
	if (end_line < start_line) return start_character;
	return std::max(start_character, end_character);
}

lng Cursor::EndLine() {
	return std::max(start_line, end_line);
}

std::string Cursor::GetText() {
	lng beginline = BeginLine();
	lng endline = EndLine();
	std::string text = "";
	for (lng linenr = beginline; linenr <= endline; linenr++) {
		TextLine* line = file->GetLine(linenr);
		lng start = linenr == beginline ? BeginCharacter() : 0;
		lng end = linenr == endline ? EndCharacter() : line->GetLength();

		if (linenr != beginline) {
			text += NEWLINE_CHARACTER;
		}
		text += std::string(line->GetLine() + start, end - start);
	}
	return text;
}

void Cursor::NormalizeCursors(TextFile* textfile, std::vector<Cursor*>& cursors, bool scroll_textfield) {
	// FIXME
	for (int i = 0; i < cursors.size(); i++) {
		for (int j = i + 1; j < cursors.size(); j++) {
			if (cursors[i]->OverlapsWith(*cursors[j])) {
				cursors[i]->Merge(*cursors[j]);
				if (textfile->active_cursor == cursors[j]) {
					textfile->active_cursor = cursors[i];
				}
				delete cursors[j];
				cursors.erase(cursors.begin() + j);
				j--;
				continue;
			}
		}
		if (cursors[i]->start_line >= cursors[i]->file->GetLineCount()) {
			if (textfile->active_cursor == cursors[i]) {
				textfile->active_cursor = nullptr;
			}
			delete cursors[i];
			cursors.erase(cursors.begin() + i);
			i--;
			continue;
		}
		assert(cursors[i]->start_character >= 0);
		if (cursors[i]->start_character > cursors[i]->file->GetLine(cursors[i]->start_line)->GetLength()) {
			if (textfile->active_cursor == cursors[i]) {
				textfile->active_cursor = nullptr;
			}
			delete cursors[i];
			cursors.erase(cursors.begin() + i);
			i--;
			continue;
		}
	}
	assert(cursors.size() > 0);
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	textfile->RefreshCursors();
	if (scroll_textfield) {
		lng line_offset = textfile->GetLineOffset();
		lng line_height = textfile->GetLineHeight();
		lng line_start = line_offset;
		lng line_end = line_start + line_height;
		lng cursor_min = INT_MAX, cursor_max = 0;
		for (int i = 0; i < cursors.size(); i++) {
			lng cursor_line = cursors[i]->start_line;
			cursor_min = std::min(cursor_min, cursor_line);
			cursor_max = std::max(cursor_max, cursor_line);
		}
		if (cursor_max - cursor_min > textfile->GetLineHeight()) {
			// cursors are too far apart to show everything, just show the first one
			line_offset = cursor_min;
		} else if (cursor_max > line_end) {
			// cursor is located past the end of what is visible, offset the view
			line_offset = cursor_max - line_height + 1;
		} else if (cursor_min < line_start) {
			// cursor is located before the start of what is visible, offset the view
			line_offset = cursor_min;
		}
		textfile->SetLineOffset(line_offset);
	}
	VerifyCursors(cursors);
}

void Cursor::VerifyCursors(std::vector<Cursor*>& cursors) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		assert((*it)->start_character >= 0 && (*it)->end_character >= 0 &&
			(*it)->start_character <= (*it)->file->GetLine((*it)->start_line)->GetLength() &&
			(*it)->end_character <= (*it)->file->GetLine((*it)->end_line)->GetLength() &&
			(*it)->start_line >= 0 && (*it)->end_line >= 0 &&
			(*it)->start_line < (*it)->file->GetLineCount() && (*it)->end_line < (*it)->file->GetLineCount());
	}
}

bool Cursor::Contains(lng linenr, lng characternr) {
	if (this->BeginLine() == this->EndLine() && 
		this->BeginLine() == linenr) {
		return characternr >= this->BeginCharacter() && characternr <= this->EndCharacter();
	} else if (this->BeginLine() == linenr) {
		return characternr >= this->BeginCharacter();
	} else if (this->EndLine() == linenr) {
		return characternr <= this->EndCharacter();
	} else if (linenr > this->BeginLine() && linenr < this->EndLine()) {
		return true;
	}
	return false;
}

bool Cursor::OverlapsWith(Cursor& cursor) {
	return
		this->Contains(cursor.BeginLine(), cursor.BeginCharacter()) ||
		this->Contains(cursor.EndLine(), cursor.EndCharacter()) ||
		cursor.Contains(BeginLine(), BeginCharacter()) ||
		cursor.Contains(EndLine(), EndCharacter());
}

void Cursor::Merge(Cursor& cursor) {
	bool beginIsStart = BeginLine() == start_line && BeginCharacter() == start_character ? true : false;
	lng beginline = BeginLine();
	lng endline = EndLine();
	lng begincharacter = BeginCharacter();
	lng endcharacter = EndCharacter();
	if (beginline > cursor.BeginLine() ||
		(beginline == cursor.BeginLine() && begincharacter > cursor.BeginCharacter())) {
		if (beginIsStart) {
			start_line = cursor.BeginLine();
			start_character = cursor.BeginCharacter();
		} else {
			end_line = cursor.BeginLine();
			end_character = cursor.BeginCharacter();
		}
	}
	if (endline < cursor.EndLine() ||
		(endline == cursor.EndLine() && endcharacter < cursor.EndCharacter())) {
		if (!beginIsStart) {
			start_line = cursor.EndLine();
			start_character = cursor.EndCharacter();
		} else {
			end_line = cursor.EndLine();
			end_character = cursor.EndCharacter();
		}
	}
}
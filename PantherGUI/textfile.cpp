
#include "textfield.h"
#include "textfile.h"
#include "text.h"
#include <algorithm>
#include <fstream>

TextFile::TextFile(std::string path) : textfield(NULL), base(NULL) {
	OpenFile(path);
}

TextFile::TextFile(std::string path, TextField* textfield) : textfield(textfield), base(NULL) {
	OpenFile(path);
}

TextFile::~TextFile() {
	if (base) {
		mmap::DestroyFileContents(base);
	}
}

void TextFile::OpenFile(std::string path) {
	this->path = path;
	this->base = (char*)mmap::ReadFile(path);

	this->lineending = PGLineEndingUnknown;
	this->indentation = PGIndentionTabs;
	char* ptr = base;
	lines.push_back(TextLine(base, 0));
	TextLine* current_line = &lines[0];
	int offset = 0;
	while (*ptr) {
		if (*ptr == '\n') {
			// Unix line ending: \n
			if (this->lineending == PGLineEndingUnknown) {
				this->lineending = PGLineEndingUnix;
			} else if (this->lineending != PGLineEndingUnix) {
				this->lineending = PGLineEndingMixed;
			}
		}
		if (*ptr == '\r') {
			if (*(ptr + 1) == '\n') {
				offset = 1;
				ptr++;
				// Windows line ending: \r\n
				if (this->lineending == PGLineEndingUnknown) {
					this->lineending = PGLineEndingWindows;
				} else if (this->lineending != PGLineEndingWindows) {
					this->lineending = PGLineEndingMixed;
				}
			} else {
				// OSX line ending: \r
				if (this->lineending == PGLineEndingUnknown) {
					this->lineending = PGLineEndingMacOS;
				} else if (this->lineending != PGLineEndingMacOS) {
					this->lineending = PGLineEndingMixed;
				}
			}
		}
		if (*ptr == '\r' || *ptr == '\n') {
			current_line->length = (ptr - current_line->line) - offset;
			lines.push_back(TextLine(ptr + 1, 0));
			current_line = &lines.back();
			offset = 0;
		}
		ptr++;
	}
	if (lines.size() == 1) {
		lineending = GetSystemLineEnding();
	}
	current_line->length = (ptr - current_line->line) - offset;
}

std::string TextFile::GetText() {
	return std::string(base);
}

// FIXME: "file has been modified without us being the one that modified it"
// FIXME: use memory mapping on big files and only parse initial lines
TextLine* TextFile::GetLine(int linenumber) {
	// FIXME: account for deltas
	if (linenumber < 0 || linenumber >= lines.size())
		return NULL;
	return &lines[linenumber];
}

ssize_t TextFile::GetLineCount() {
	// FIXME: if file has not been entirely loaded (i.e. READONLY) then read 
	return lines.size();
}

static bool
CursorsContainSelection(std::vector<Cursor*>& cursors) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (!(*it)->SelectionIsEmpty()) {
			return true;
		}
	}
	return false;
}

void TextFile::InsertText(char character, std::vector<Cursor*>& cursors) {
	InsertText(std::string(1, character), cursors);
}

void TextFile::InsertText(std::string text, std::vector<Cursor*>& cursors) {
	// FIXME: merge delta if it already exists
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (!(*it)->SelectionIsEmpty()) {
			// if any of the cursors select text, we delete that text before inserting the characters
			DeleteCharacter(delta, *it, PGDirectionLeft, true, false);
		}
		// now actually add the text
		// note that we add it at the end of the selected area
		// it will be shifted to the start of the selected area in PerformOperation
		delta->AddDelta(new AddText(*it, (*it)->EndLine(), (*it)->EndCharacter(), text));
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

std::string TextFile::CopyText(std::vector<Cursor*>& cursors) {
	std::string text = "";
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it != cursors.begin()) {
			text += NEWLINE_CHARACTER;
		}
		text += (*it)->GetText();
	}
	return text;
}

void TextFile::PasteText(std::vector<Cursor*>& cursors, std::string text) {
	std::vector<std::string> pasted_lines;
	ssize_t start = 0;
	ssize_t current = 0;
	for (auto it = text.begin(); it != text.end(); it++) {
		if (*it == '\r') {
			if (*(it + 1) == '\n') {
				it++;
			} else {
				pasted_lines.push_back(text.substr(start, current - start));
				start = current + 2;
				current = start;
				continue;
			}
		}
		if (*it == '\n') {
			pasted_lines.push_back(text.substr(start, current - start));
			start = current + 2;
			current = start;
			continue;
		}
		current++;
	}
	pasted_lines.push_back(text.substr(start, current - start));
	
	if (pasted_lines.size() == 1) {
		InsertText(pasted_lines[0], cursors);
	} else {
		AddNewLines(cursors, pasted_lines, false);
	}
}


void
TextFile::DeleteCharacter(MultipleDelta* delta, Cursor* it, PGDirection direction, bool delete_selection, bool include_cursor) {
	if (delete_selection) {
		if (!it->SelectionIsEmpty()) {
			// delete with selection
			RemoveLines *remove_lines = new RemoveLines(NULL, it->BeginLine() + 1);
			for (ssize_t i = it->BeginLine(); i <= it->EndLine(); i++) {
				if (i == it->BeginLine()) {
					if (i == it->EndLine()) {
						delete remove_lines;
						delta->AddDelta(new RemoveText(include_cursor ? &*it : NULL, i, it->EndCharacter(), it->EndCharacter() - it->BeginCharacter()));
					} else {
						delta->AddDelta(new RemoveText(include_cursor ? &*it : NULL, i, lines[i].GetLength(), lines[i].GetLength() - it->BeginCharacter()));
					}
				} else if (i == it->EndLine()) {
					assert(it->EndCharacter() <= lines[i].GetLength());
					// remove part of the last line
					std::string text = std::string(lines[i].GetLine() + it->EndCharacter(), lines[i].GetLength() - it->EndCharacter());
					remove_lines->AddLine(lines[i]);
					remove_lines->last_line_offset = it->EndCharacter();
					delta->AddDelta(remove_lines);
					delta->AddDelta(new AddText(NULL, it->BeginLine(), it->BeginCharacter(), text));
				} else {
					// remove the entire line
					remove_lines->AddLine(lines[i]);
				}
			}
		} else {
			if (include_cursor) {
				delta->AddDelta(new CursorDelta(it, it->start_line, it->start_character));
			}
		}
	} else {
		ssize_t characternumber = it->SelectedCharacter();
		ssize_t linenumber = it->SelectedLine();
		if (direction == PGDirectionLeft && characternumber == 0) {
			if (linenumber > 0) {
				// merge the line with the previous line
				std::string line = std::string(lines[linenumber].GetLine(), lines[linenumber].GetLength());
				delta->AddDelta(new RemoveLine(include_cursor ? &*it : NULL, linenumber, lines[linenumber]));
				TextDelta *addText = new AddText(NULL, linenumber - 1, lines[linenumber - 1].GetLength(), line);
				delta->AddDelta(addText);
			}
		} else if (direction == PGDirectionRight && characternumber == lines[linenumber].GetLength()) {
			if (linenumber < GetLineCount() - 1) {
				// merge the next line with the current line
				std::string line = std::string(lines[linenumber + 1].GetLine(), lines[linenumber + 1].GetLength());
				delta->AddDelta(new RemoveLine(include_cursor ? &*it : NULL, linenumber + 1, lines[linenumber + 1]));
				TextDelta *addText = new AddText(NULL, linenumber, lines[linenumber].GetLength(), line);
				delta->AddDelta(addText);
			}
		} else {
			// FIXME: merge delta if it already exists
			RemoveText* remove = new RemoveText(include_cursor ? &*it : NULL,
				linenumber,
				direction == PGDirectionRight ? characternumber + 1 : characternumber,
				1);
			delta->AddDelta(remove);
		}
	}
}
void
TextFile::DeleteCharacter(MultipleDelta* delta, std::vector<Cursor*>& cursors, PGDirection direction) {
	bool delete_selection = CursorsContainSelection(cursors);
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		DeleteCharacter(delta, *it, direction, delete_selection);
	}
}

void TextFile::DeleteCharacter(std::vector<Cursor*>& cursors, PGDirection direction) {
	MultipleDelta* delta = new MultipleDelta();
	DeleteCharacter(delta, cursors, direction);
	if (delta->deltas.size() == 0) {
		delete delta;
		return;
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::DeleteWord(std::vector<Cursor*>& cursors, PGDirection direction) {
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if ((*it)->SelectionIsEmpty() && (direction == PGDirectionLeft ? (*it)->SelectedCharacter() > 0 : (*it)->SelectedCharacter() < lines[(*it)->SelectedLine()].GetLength())) {
			int offset = direction == PGDirectionLeft ? -1 : 1;
			char* line = lines[(*it)->SelectedLine()].GetLine();
			ssize_t length = lines[(*it)->SelectedLine()].GetLength();
			ssize_t index = direction == PGDirectionLeft ? (*it)->SelectedCharacter() + offset : (*it)->SelectedCharacter();
			// a word is a series of characters of equal type (punctuation, spaces or everything else)
			// we scan the string to find out where the word ends 
			PGCharacterClass type = GetCharacterClass(line[index]);
			for (index += offset; index >= 0 && index < length; index += offset) {
				if (GetCharacterClass(line[index]) != type) {
					index -= direction == PGDirectionLeft ? offset : 0;
					break;
				}
			}
			index = std::min(std::max(index, (ssize_t)0), length);
			delta->AddDelta(new RemoveText(*it, (*it)->SelectedLine(), std::max(index, (*it)->SelectedCharacter()), std::abs((*it)->SelectedCharacter() - index)));
		} else {
			DeleteCharacter(delta, *it, direction, !(*it)->SelectionIsEmpty());
		}
	}
	if (delta->deltas.size() == 0) {
		delete delta;
		return;
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::AddNewLine(std::vector<Cursor*>& cursors) {
	// FIXME: newline with multiple cursors on the same line
	AddNewLine(cursors, "");
}

void TextFile::AddNewLine(std::vector<Cursor*>& cursors, std::string text) {
	std::vector<std::string> lines;
	lines.push_back(text);
	AddNewLines(cursors, lines, true);
}

void TextFile::AddNewLines(std::vector<Cursor*>& cursors, std::vector<std::string>& added_text, bool first_is_newline) {
	MultipleDelta* delta = new MultipleDelta();
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors select text, we delete that text before inserting the characters
		DeleteCharacter(delta, cursors, PGDirectionLeft);
		for (int i = 0; i < delta->deltas.size(); i++) {
			if (delta->deltas[i]->TextDeltaType() == PGDeltaAddText) {
				delta->deltas.erase(delta->deltas.begin() + i);
				i--;
			}
		}
	}
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		ssize_t characternumber = (*it)->EndCharacter();
		ssize_t linenumber = (*it)->EndLine();
		ssize_t length = lines[linenumber].GetLength();
		ssize_t current_line = (*it)->BeginLine();

		if (added_text.size() == 1) {
			delta->AddDelta(new AddLine(*it, current_line, (*it)->BeginCharacter(), added_text.front()));
			continue;
		}
		std::vector<std::string> text = added_text;
		if (!first_is_newline) {
			assert(added_text.size() > 1); // if added_text == 1 and first_is_newline is false, use InsertText
			delta->AddDelta(new AddText(NULL, (*it)->BeginLine(), (*it)->BeginCharacter(), added_text.front()));
			text.erase(text.begin());
		}
		delta->AddDelta(new AddLines(*it, (*it)->BeginLine(), (*it)->BeginCharacter(), text));
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);

}

void TextFile::Undo(std::vector<Cursor*>& cursors) {
	if (this->deltas.size() == 0) return;
	TextDelta* delta = this->deltas.back();
	textfield->ClearCursors(cursors);
	this->Undo(delta, cursors);
	this->deltas.pop_back();
	this->redos.push_back(delta);
}

void TextFile::Redo(std::vector<Cursor*>& cursors) {
	if (this->redos.size() == 0) return;
	TextDelta* delta = this->redos.back();
	textfield->ClearCursors(cursors);
	this->Redo(delta, cursors);
	this->PerformOperation(delta, false);
	this->redos.pop_back();
	this->deltas.push_back(delta);
}

void TextFile::AddDelta(TextDelta* delta) {
	for (auto it = redos.begin(); it != redos.end(); it++) {
		delete *it;
	}
	redos.clear();
	this->deltas.push_back(delta);
}

void TextFile::PerformOperation(TextDelta* delta, bool adjust_delta) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr].AddDelta(delta);
		if (add->cursor) {
			add->cursor->end_character = add->cursor->start_character = add->characternr + add->text.size();
			add->cursor->end_line = add->cursor->start_line = add->linenr;
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr].AddDelta(delta);
		if (remove->cursor) {
			remove->cursor->start_character = remove->cursor->end_character = (remove->characternr - remove->charactercount);
			remove->cursor->end_line = remove->cursor->start_line = remove->linenr;
		}
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		ssize_t length = lines[add->linenr].GetLength();
		add->extra_text = std::string(add->line);
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr].GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(NULL, add->linenr, length, length - add->characternr);
			lines[add->linenr].AddDelta(add->remove_text);
		}
		lines.insert(lines.begin() + (add->linenr + 1), TextLine((char*)add->extra_text.c_str(), (ssize_t)add->extra_text.size()));
		if (add->cursor) {
			add->cursor->end_line = add->cursor->start_line = add->linenr + 1;
			add->cursor->end_character = add->cursor->start_character = add->line.size();
		}
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		ssize_t length = lines[add->linenr].GetLength();
		ssize_t linenr = add->linenr + 1;
		// FIXME: we keep around a reference to a string here, this probably segfaults if the delta is removed
		for (auto it = add->lines.begin(); (it + 1) != add->lines.end(); it++) {
			lines.insert(lines.begin() + linenr, TextLine((char*)it->c_str(), (ssize_t)it->size()));
			linenr++;
		}
		add->extra_text = add->lines.back();
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr].GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(NULL, add->linenr, length, length - add->characternr);
			lines[add->linenr].AddDelta(add->remove_text);
		}
		lines.insert(lines.begin() + linenr, TextLine((char*)add->extra_text.c_str(), (ssize_t)add->extra_text.size()));
		if (add->cursor) {
			add->cursor->end_line = add->cursor->start_line = linenr;
			add->cursor->end_character = add->cursor->start_character = add->lines.back().size();
		}
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		if (remove->cursor) {
			remove->cursor->end_line = remove->cursor->start_line = remove->linenr - 1;
			remove->cursor->end_character = remove->cursor->start_character = this->lines[remove->cursor->start_line].GetLength();
		}
		this->lines.erase(this->lines.begin() + remove->linenr);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		if (remove->cursor) {
			remove->cursor->end_line = remove->cursor->start_line = remove->linenr;
			remove->cursor->end_character = remove->cursor->start_character = 0;
		}
		this->lines.erase(this->lines.begin() + remove->linenr, this->lines.begin() + remove->linenr + remove->lines.size());
		break;
	}
	case PGDeltaCursorMovement: {
		CursorDelta* cursor_delta = (CursorDelta*)delta;
		if (cursor_delta->cursor) {
			cursor_delta->cursor->end_line = cursor_delta->cursor->start_line = cursor_delta->linenr;
			cursor_delta->cursor->end_character = cursor_delta->cursor->start_character = cursor_delta->characternr;
		}
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (auto it = multi->deltas.begin(); it != multi->deltas.end(); it++) {
			assert((*it)->TextDeltaType() != PGDeltaMultiple); // nested multiple deltas not supported
			PerformOperation(*it, adjust_delta);
			if (!adjust_delta) continue;

			ssize_t shift_lines = 0;
			ssize_t shift_lines_line = (*it)->linenr;
			ssize_t shift_lines_character = (*it)->characternr;
			ssize_t shift_characters = 0;
			ssize_t shift_characters_line = (*it)->linenr;
			ssize_t shift_characters_character = (*it)->characternr;
			ssize_t last_line_offset = 0;
			switch ((*it)->TextDeltaType()) {
			case PGDeltaRemoveLine:
				shift_lines = -1;
				break;
			case PGDeltaRemoveManyLines:
				shift_lines = -((ssize_t)((RemoveLines*)(*it))->lines.size());
				last_line_offset = ((RemoveLines*)(*it))->last_line_offset;
				break;
			case PGDeltaAddLine:
				shift_lines = 1;
				shift_characters = -((AddLine*)(*it))->characternr;
				break;
			case PGDeltaAddManyLines:
				shift_lines = ((AddLines*)(*it))->lines.size();
				shift_characters = -((AddLines*)(*it))->characternr;
				break;
			case PGDeltaAddText:
				shift_characters = (ssize_t)((AddText*)(*it))->text.size();
				break;
			case PGDeltaRemoveText:
				shift_characters = -((ssize_t)((RemoveText*)(*it))->charactercount);
				break;
			}
			for (auto it2 = it + 1; it2 != multi->deltas.end(); it2++) {
				TextDelta* delta = *it2;
				if (delta->linenr == shift_characters_line &&
					delta->characternr >= shift_characters_character) {
					delta->characternr += shift_characters;
				} else if (delta->linenr == shift_characters_line &&
					delta->characternr < 0) {
					// special case: see below explanation
					delta->characternr = shift_characters_character + std::abs(delta->characternr + 1);
				}
				if (shift_lines != 0) {
					if (delta->linenr == (shift_lines_line + std::abs(shift_lines) - 1) && shift_lines < 0) {
						// special case: multi-line deletion that ends with deleting part of a line
						// this is done by removing the entire last line and then adding part of the line back
						// removing and readding text messes with cursors on that line,
						// to make the cursor go back to the proper position, 
						// we store the position of this delta on the original line as a negative number when removing it
						// when performing the AddText we do a special case for negative numbers
						// to shift the cursor position to its original position in the text again
						delta->characternr = -(delta->characternr - last_line_offset) - 1;
						delta->linenr = shift_lines_line - 1;
					} else if (delta->linenr >= shift_lines_line) {
						delta->linenr += shift_lines;
					}
				}
			}
			if ((*it)->TextDeltaType() != PGDeltaRemoveManyLines) {
				last_line_offset = 0;
			}
		}
		break;
	}
	}
}

void TextFile::Undo(TextDelta* delta, std::vector<Cursor*>& cursors) {
	assert(delta);
	CursorDelta* cursor_delta = dynamic_cast<CursorDelta*>(delta);
	if (cursor_delta && cursor_delta->cursor) {
		cursors.push_back(new Cursor(this));
		cursors.back()->RestoreCursor(cursor_delta->stored_cursor);
	}
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr].PopDelta();
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr].PopDelta();
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		if (add->remove_text) {
			lines[add->linenr].RemoveDelta(add->remove_text);
			add->remove_text = NULL;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1));
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		if (add->remove_text) {
			lines[add->linenr].RemoveDelta(add->remove_text);
			add->remove_text = NULL;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1), this->lines.begin() + (add->linenr + 1 + add->lines.size()));
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		this->lines.insert(this->lines.begin() + remove->linenr, remove->line);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		ssize_t index = remove->linenr;
		for (auto it = remove->lines.begin(); it != remove->lines.end(); it++, index++) {
			this->lines.insert(this->lines.begin() + index, *it);
		}
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (ssize_t index = multi->deltas.size() - 1; index >= 0; index--) {
			Undo(multi->deltas[index], cursors);
		}
		break;
	}
	}
}

void TextFile::Redo(TextDelta* delta, std::vector<Cursor*>& cursors) {
	assert(delta);
	CursorDelta* cursor_delta = dynamic_cast<CursorDelta*>(delta);
	if (cursor_delta && cursor_delta->cursor) {
		cursor_delta->cursor = new Cursor(this);
		cursors.push_back(cursor_delta->cursor); 
		cursor_delta->cursor->RestoreCursor(cursor_delta->stored_cursor);
	}

	switch (delta->TextDeltaType()) {
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (auto it = multi->deltas.begin(); it != multi->deltas.end(); it++) {
			Redo(*it, cursors);
		}
		break;
	}
	}
}

void TextFile::SaveChanges() {
	PGLineEnding line_ending = lineending;
	if (line_ending != PGLineEndingWindows && line_ending != PGLineEndingMacOS && line_ending != PGLineEndingUnix) {
		line_ending = GetSystemLineEnding();
	}
	// FIXME: respect file encoding
	PGFileHandle handle = mmap::OpenFile(this->path, PGFileReadWrite);
	ssize_t position = 0;
	for (auto it = lines.begin(); it != lines.end(); it++) {
		ssize_t length = it->GetLength();
		char* line = it->GetLine();
		mmap::WriteToFile(handle, line, length);

		if (it + 1 != lines.end()) {
			switch (line_ending) {
			case PGLineEndingWindows:
				mmap::WriteToFile(handle, "\r\n", 2);
				break;
			case PGLineEndingMacOS:
				mmap::WriteToFile(handle, "\r", 1);
				break;
			case PGLineEndingUnix:
				mmap::WriteToFile(handle, "\n", 1);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	mmap::CloseFile(handle);
}

void TextFile::ChangeLineEnding(PGLineEnding lineending) {
	assert(0);
}

void TextFile::ChangeFileEncoding(PGFileEncoding encoding) {
	assert(0);
}

void TextFile::ChangeIndentation(PGLineIndentation indentation) {

}

void TextFile::RemoveTrailingWhitespace() {

}

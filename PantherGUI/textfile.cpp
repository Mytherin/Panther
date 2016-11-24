
#include "textfield.h"
#include "textfile.h"
#include "text.h"
#include <algorithm>
#include <fstream>

TextFile::TextFile(std::string path) : textfield(NULL) {
	OpenFile(path);
}

TextFile::TextFile(std::string path, TextField* textfield) : textfield(textfield) {
	OpenFile(path);
}

TextFile::~TextFile() {
	FlushMemoryMappedFile(base);
	CloseMemoryMappedFile(base);
	DestroyMemoryMappedFile(file);
}

void TextFile::OpenFile(std::string path) {
	file = MemoryMapFile(path);
	base = (char*)OpenMemoryMappedFile(file);

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
			if (*(ptr + 1) != '\0') {
				lines.push_back(TextLine(ptr + 1, 0));
				current_line = &lines.back();
				offset = 0;
			}
		}
		ptr++;
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
CursorsContainSelection(std::vector<Cursor>& cursors) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (!it->SelectionIsEmpty()) {
			return true;
		}
	}
	return false;
}

void TextFile::InsertText(char character, std::vector<Cursor>& cursors) {
	// FIXME: merge delta if it already exists
	// FIXME: check if MemoryMapped file size is too high after write

	// FIXME: insert with selection
	MultipleDelta* delta = new MultipleDelta();
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors select text, we delete that text before inserting the characters
		DeleteCharacter(delta, cursors, PGDirectionLeft);
	}

	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		AddText* text = new AddText(&*it, it->BeginLine(), it->BeginCharacter(), std::string(1, character));
		delta->AddDelta(text);
	}
	this->AddDelta(delta);
	Redo(delta);
}

std::string TextFile::CopyText(std::vector<Cursor>& cursors) {
	std::string text = "";
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it != cursors.begin()) {
			text += NEWLINE_CHARACTER;
		}
		text += it->GetText();
	}
	return text;
}

void TextFile::PasteText(std::vector<Cursor>& cursors, std::string text) {
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
	if (current > start) {
		pasted_lines.push_back(text.substr(start, current - start));
	}
	// FIXME: don't add newline before first line
	// FIXME: cursor position should not be start of final line but end of it
	AddNewLines(cursors, pasted_lines);
}


void
TextFile::DeleteCharacter(MultipleDelta* delta, Cursor* it, PGDirection direction, bool delete_selection) {
	if (delete_selection) {
		if (!it->SelectionIsEmpty()) {
			// delete with selection
			RemoveLines *remove_lines = new RemoveLines(NULL, it->BeginLine() + 1);
			for (ssize_t i = it->BeginLine(); i <= it->EndLine(); i++) {
				if (i == it->BeginLine()) {
					if (i == it->EndLine()) {
						delete remove_lines;
						delta->AddDelta(new RemoveText(&*it, i, it->EndCharacter(), it->EndCharacter() - it->BeginCharacter()));
					} else {
						delta->AddDelta(new RemoveText(&*it, i, lines[i].GetLength(), lines[i].GetLength() - it->BeginCharacter()));
					}
				} else if (i == it->EndLine()) {
					// remove part of the last line
					std::string text = std::string(lines[i].GetLine() + it->EndCharacter(), lines[i].GetLength() - it->EndCharacter());
					remove_lines->AddLine(lines[i]);
					delta->AddDelta(remove_lines);
					delta->AddDelta(new AddText(NULL, it->BeginLine(), it->BeginCharacter(), text));
				} else {
					// remove the entire line
					remove_lines->AddLine(lines[i]);
				}
			}
		}
	} else {
		ssize_t characternumber = it->SelectedCharacter();
		ssize_t linenumber = it->SelectedLine();
		if (direction == PGDirectionLeft && characternumber == 0) {
			if (linenumber > 0) {
				// merge the line with the previous line
				std::string line = std::string(lines[linenumber].GetLine(), lines[linenumber].GetLength());
				delta->AddDelta(new RemoveLine(&*it, linenumber, lines[linenumber]));
				TextDelta *addText = new AddText(NULL, linenumber - 1, lines[linenumber - 1].GetLength(), line);
				delta->AddDelta(addText);
			}
		} else if (direction == PGDirectionRight && characternumber == lines[linenumber].GetLength()) {
			if (linenumber < GetLineCount() - 1) {
				// merge the next line with the current line
				std::string line = std::string(lines[linenumber + 1].GetLine(), lines[linenumber + 1].GetLength());
				delta->AddDelta(new RemoveLine(&*it, linenumber + 1, lines[linenumber + 1]));
				TextDelta *addText = new AddText(NULL, linenumber, lines[linenumber].GetLength(), line);
				delta->AddDelta(addText);
			}
		} else {
			// FIXME: merge delta if it already exists
			RemoveText* remove = new RemoveText(&*it, 
				linenumber, 
				direction == PGDirectionRight ? characternumber + 1 : characternumber,
				1);
			delta->AddDelta(remove);
		}
	}
}
void
TextFile::DeleteCharacter(MultipleDelta* delta, std::vector<Cursor>& cursors, PGDirection direction) {
	bool delete_selection = CursorsContainSelection(cursors);
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		DeleteCharacter(delta, &*it, direction, delete_selection);
	}
}

void TextFile::DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction) {
	MultipleDelta* delta = new MultipleDelta();
	DeleteCharacter(delta, cursors, direction);
	this->AddDelta(delta);
	Redo(delta);
}

void TextFile::DeleteWord(std::vector<Cursor>& cursors, PGDirection direction) {
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it->SelectionIsEmpty() && (direction == PGDirectionLeft ? it->SelectedCharacter() > 0 : it->SelectedCharacter() < lines[it->SelectedLine()].GetLength())) {
			int offset = direction == PGDirectionLeft ? -1 : 1;
			char* line = lines[it->SelectedLine()].GetLine();
			ssize_t length = lines[it->SelectedLine()].GetLength();
			ssize_t index = direction == PGDirectionLeft ? it->SelectedCharacter() + offset : it->SelectedCharacter();
			PGCharacterClass type = GetCharacterClass(line[index]);
			for (index += offset; index >= 0 && index < length; index += offset) {
				if (GetCharacterClass(line[index]) != type) {
					index -= direction == PGDirectionLeft ? offset : 0;
					break;
				}
		}	
			index = std::min(std::max(index, (ssize_t)0), length);
			delta->AddDelta(new RemoveText(&*it, it->SelectedLine(), std::max(index, it->SelectedCharacter()), std::abs(it->SelectedCharacter() - index)));
		} else {
			DeleteCharacter(delta, &*it, direction, !it->SelectionIsEmpty());
		}
	}
	this->AddDelta(delta);
	Redo(delta);
}

void TextFile::AddNewLine(std::vector<Cursor>& cursors) {
	AddNewLine(cursors, "");
}

void TextFile::AddNewLine(std::vector<Cursor>& cursors, std::string text) {
	std::vector<std::string> lines;
	lines.push_back(text);
	AddNewLines(cursors, lines);
}

void TextFile::AddNewLines(std::vector<Cursor>& cursors, std::vector<std::string>& added_text) {
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
		ssize_t characternumber = it->EndCharacter();
		ssize_t linenumber = it->EndLine();
		ssize_t length = lines[linenumber].GetLength();
		ssize_t current_line = it->BeginLine();
		std::string line = std::string(lines[linenumber].GetLine(), length).substr(characternumber, std::string::npos);
		if (it->BeginLine() == it->EndLine() && length > characternumber) {
			TextDelta* removeText = new RemoveText(NULL, linenumber, length, length - characternumber);
			delta->AddDelta(removeText);
		}
		for (auto it2 = added_text.begin(); (it2 + 1) != added_text.end(); it2++) {		
			delta->AddDelta(new AddLine(NULL, current_line, 0, *it2));
		}
		delta->AddDelta(new AddLine(&*it, current_line, 0, added_text.back() + line));

	}
	this->AddDelta(delta);
	Redo(delta);

}

void TextFile::Undo() {
	if (this->deltas.size() == 0) return;
	TextDelta* delta = this->deltas.back();
	this->Undo(delta);
	this->deltas.pop_back();
	this->redos.push_back(delta);
}

void TextFile::Redo() {
	if (this->redos.size() == 0) return;
	TextDelta* delta = this->redos.back();
	this->Redo(delta);
	this->redos.pop_back();
	this->deltas.push_back(delta);
}

void TextFile::AddDelta(TextDelta* delta) {
	if (delta->TextDeltaType() == PGDeltaMultiple) {
		((MultipleDelta*)delta)->FinalizeDeltas();
	}
	for (auto it = redos.begin(); it != redos.end(); it++) {
		delete *it;
	}
	redos.clear();
	this->deltas.push_back(delta);
}

void TextFile::Undo(TextDelta* delta) {
	assert(delta);
	CursorDelta* cursor_delta = dynamic_cast<CursorDelta*>(delta);
	if (cursor_delta && cursor_delta->cursor) {
		cursor_delta->cursor->RestoreCursor(cursor_delta->stored_cursor);
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
		this->lines.erase(this->lines.begin() + (add->linenr + 1));
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		this->lines.insert(this->lines.begin() + remove->linenr, remove->line);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		ssize_t index = remove->start;
		for (auto it = remove->lines.begin(); it != remove->lines.end(); it++, index++) {
			this->lines.insert(this->lines.begin() + index, *it);
		}
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		for (ssize_t index = multi->deltas.size() - 1; index >= 0; index--) {
			Undo(multi->deltas[index]);
		}
		break;
	}
	}
}

void TextFile::Redo(TextDelta* delta) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr].AddDelta(delta);
		if (add->cursor) {
			add->cursor->end_character = (add->cursor->start_character += add->text.size());
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
		// FIXME: we keep around a reference to a string here, this probably segfaults if the delta is removed
		lines.insert(lines.begin() + (add->linenr + 1), TextLine((char*)add->line.c_str(), add->line.size()));
		if (add->cursor) {
			add->cursor->end_line = (add->cursor->start_line += 1);
			add->cursor->end_character = add->cursor->start_character = 0;
		}
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		if (remove->cursor) {
			if (remove->cursor->start_line == remove->linenr) {
				remove->cursor->end_line = (remove->cursor->start_line -= 1);
				remove->cursor->end_character = remove->cursor->start_character = this->lines[remove->cursor->start_line].GetLength();
			}
		}
		this->lines.erase(this->lines.begin() + remove->linenr);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		if (remove->cursor) {
			remove->cursor->end_line = remove->cursor->start_line = remove->start;
			remove->cursor->end_character = remove->cursor->start_character = 0;
		}
		this->lines.erase(this->lines.begin() + remove->start, this->lines.begin() + remove->start + remove->lines.size());
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		for (auto it = multi->deltas.begin(); it != multi->deltas.end(); it++) {
			Redo(*it);
		}
		break;
	}
	}
}

void TextFile::Flush() {
	FlushMemoryMappedFile(this->base);
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

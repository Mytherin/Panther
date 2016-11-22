
#include "textfield.h"
#include "textfile.h"
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
			}
			else if (this->lineending != PGLineEndingUnix) {
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
				}
				else if (this->lineending != PGLineEndingWindows) {
					this->lineending = PGLineEndingMixed;
				}
			}
			else {
				// OSX line ending: \r
				if (this->lineending == PGLineEndingUnknown) {
					this->lineending = PGLineEndingMacOS;
				}
				else if (this->lineending != PGLineEndingMacOS) {
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

void TextFile::InsertText(char character, std::vector<Cursor>& cursors) {
	// FIXME: merge delta if it already exists
	// FIXME: check if MemoryMapped file size is too high after write

	// FIXME: insert with selection
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		AddText* text = new AddText(&*it, it->selected_line, it->selected_character, std::string(1, character));
		//lines[it->selected_line].AddDelta(text);
		//it->selected_character++;
		delta->AddDelta(text);
	}
	this->AddDelta(delta);
	Redo(delta);
}

void TextFile::DeleteCharacter(std::vector<Cursor>& cursors) {
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		// FIXME: delete with selection
		// FIXME: merge delta if it already exists
		ssize_t characternumber = it->SelectedCharacter();
		ssize_t linenumber = it->SelectedLine();
		//it->OffsetCharacter(-1);
		if (characternumber == 0) {
			if (linenumber > 0) {
				// merge the line with the previous line
				std::string line = std::string(lines[linenumber].GetLine(), lines[linenumber].GetLength());
				delta->AddDelta(new RemoveLine(&*it, linenumber, lines[linenumber]));
				//lines.erase(lines.begin() + linenumber);
				TextDelta *addText = new AddText(NULL, linenumber - 1, lines[linenumber - 1].GetLength(), line);
				//lines[linenumber - 1].AddDelta(addText);
				delta->AddDelta(addText);
			}
		}
		else {
			RemoveText* remove = new RemoveText(&*it, linenumber, characternumber, 1);
			delta->AddDelta(remove);
			//lines[linenumber].AddDelta(remove);
		}
	}
	this->AddDelta(delta);
	Redo(delta);
}

void TextFile::AddNewLine(std::vector<Cursor>& cursors) {
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		ssize_t characternumber = it->SelectedCharacter();
		ssize_t linenumber = it->SelectedLine();
		ssize_t length = lines[linenumber].GetLength();
		std::string line = std::string(lines[linenumber].GetLine(), length).substr(characternumber, std::string::npos);
		if (length > characternumber) {
			TextDelta* removeText = new RemoveText(&*it, linenumber, length, length - characternumber);
			delta->AddDelta(removeText);
			//lines[linenumber].AddDelta(removeText);
		}
		delta->AddDelta(new AddLine(&*it, linenumber, characternumber));
		//lines.insert(lines.begin() + (linenumber + 1), TextLine("", 0));
		if (length > characternumber) {
			TextDelta* addText = new AddText(NULL, linenumber + 1, 0, line);
			delta->AddDelta(addText);
			//lines[linenumber + 1].AddDelta(addText);
		}
		//it->selected_line++;
		//it->selected_character = 0;
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
	for (auto it = redos.begin(); it != redos.end(); it++) {
		delete *it;
	}
	redos.clear();
	this->deltas.push_back(delta);
}

void TextFile::Undo(TextDelta* delta) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		if (add->cursor) {
			add->cursor->selected_character -= add->text.size();
		}
		this->lines[add->linenr].PopDelta();
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		if (remove->cursor) {
			remove->cursor->selected_character += remove->charactercount;
		}
		this->lines[remove->linenr].PopDelta();
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		if (add->cursor) {
			add->cursor->selected_character = this->lines[add->linenr].GetLength();
			add->cursor->selected_line--;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1));
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		if (remove->cursor) {
			remove->cursor->selected_line++;
			remove->cursor->selected_character = 0;
		}
		this->lines.insert(this->lines.begin() + remove->linenr, remove->line);
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		for (auto it = multi->deltas.begin() + multi->deltas.size() - 1; ; it--) {
			Undo(*it);
			if (it == multi->deltas.begin()) break;
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
			add->cursor->selected_character += add->text.size();
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr].AddDelta(delta);
		if (remove->cursor) {
			remove->cursor->selected_character -= remove->charactercount;
		}
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		lines.insert(lines.begin() + (add->linenr + 1), TextLine("", 0));
		if (add->cursor) {
			add->cursor->selected_line += 1;
			add->cursor->selected_character = 0;
		}
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		if (remove->cursor) {
			remove->cursor->selected_line -= 1;
			remove->cursor->selected_character = this->lines[remove->linenr].GetLength();
		}
		this->lines.erase(this->lines.begin() + remove->linenr);
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

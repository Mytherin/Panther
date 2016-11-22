

#include "textfile.h"
#include <fstream>

TextFile::TextFile(std::string path) {
	file = MemoryMapFile(path);
	base = (char*)OpenMemoryMappedFile(file);

	this->lineending = PGLineEndingUnknown;
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

TextFile::~TextFile() {
	FlushMemoryMappedFile(base);
	CloseMemoryMappedFile(base);
	DestroyMemoryMappedFile(file);
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
	// FIXME: account for deltas
	// FIXME: if file has not been entirely loaded (i.e. READONLY) then read 
	return lines.size();
}

void TextFile::InsertText(char character, int linenumber, int characternumber) {
	// FIXME: delta
	// FIXME: merge delta if it already exists
	// FIXME: check if MemoryMapped file size is too high after write

	this->deltas.push_back(new AddText(linenumber, characternumber, std::string(1, character)));
	lines[linenumber].AddDelta(this->deltas.back());
	/*
	char *start = &lines[linenumber].line[characternumber];
	char* end = lines.back().line + lines.back().length + 1;
	memmove(start + 1, start, (end - start));
	lines[linenumber].line[characternumber] = character;
	lines[linenumber].length++;*/
}

void TextFile::DeleteCharacter(int linenumber, int characternumber) {
	/*char *start = &lines[linenumber].line[characternumber];
	char* end = lines.back().line + lines.back().length + 1;
	memmove(start - 1, start, (end - start));*/

	if (characternumber == 0) {
		// remove the line
		this->deltas.push_back(new RemoveLine(linenumber, lines[linenumber]));
		lines.erase(lines.begin() + linenumber);
	} else {
		// FIXME: add delta to line number
		// FIXME: merge delta if it already exists
		this->deltas.push_back(new RemoveText(linenumber, characternumber, 1));
		lines[linenumber].AddDelta(this->deltas.back());
	}
}

void TextFile::AddNewLine(int linenumber, int characternumber) {
	MultipleDelta* delta = new MultipleDelta();
	lines[linenumber].ApplyDeltas();
	std::string line = std::string(lines[linenumber].modified_line).substr(characternumber, std::string::npos);
	TextDelta* removeText = new RemoveText(linenumber, lines[linenumber].GetLength(), lines[linenumber].GetLength() - characternumber);
	delta->AddDelta(removeText);
	lines[linenumber].AddDelta(removeText);
	delta->AddDelta(new AddLine(linenumber, characternumber));
	lines.insert(lines.begin() + (linenumber + 1), TextLine("", 0));
	TextDelta* addText = new AddText(linenumber + 1, 0, line);
	delta->AddDelta(addText);
	lines[linenumber + 1].AddDelta(addText);
	this->deltas.push_back(delta);
}

void TextFile::Undo() {
	assert(0);
	// FIXME
}

void TextFile::Redo() {
	assert(0);
	// FIXME
}

void TextFile::Flush() {
	FlushMemoryMappedFile(this->base);
}

void TextFile::SetLineEnding(PGLineEnding lineending) {
	assert(0);
	// FIXME
}


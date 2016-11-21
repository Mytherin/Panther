

#include "textfile.h"
#include <fstream>

TextFile::TextFile(std::string path) {
	file = MemoryMapFile(path);
	base = (char*) OpenMemoryMappedFile(file);

	// FIXME: determine lineending of file while scanning

	char* ptr = base;
	lines.push_back(TextLine(base, 0));
	TextLine* current_line = &lines[0];
	int offset = 0;
	while (*ptr) {
		if (*ptr == '\r') {
			if (*(ptr + 1) == '\n') {
				offset = 1;
				ptr++;
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
	if (linenumber < 0 || linenumber >= lines.size()) 
		return NULL;
	return &lines[linenumber];
}

// FIXME: if file has not been entirely loaded (i.e. READONLY) then read 
ssize_t TextFile::GetLineCount() {
	return lines.size();
}

void TextFile::InsertText(char character, int linenumber, int characternumber) {
	// FIXME: check if MemoryMapped file size is too high after write
	char *start = &lines[linenumber].line[characternumber];
	char* end = lines.back().line + lines.back().length + 1;
	memmove(start + 1, start, (end - start));
	lines[linenumber].line[characternumber] = character;
	lines[linenumber].length++;
}

void TextFile::DeleteCharacter(int linenumber, int characternumber) {
	char *start = &lines[linenumber].line[characternumber];
	char* end = lines.back().line + lines.back().length + 1;
	memmove(start - 1, start, (end - start));
	if (characternumber == 0) {
		// remove the line
		lines.erase(lines.begin() + linenumber);
	} else {
		lines[linenumber].length--;
	}
}

void TextFile::Flush() {
	FlushMemoryMappedFile(this->base);
}

size_t TextLine::GetLength() {
	return this->length;
}

char* TextLine::GetLine() {
	return this->line;
}

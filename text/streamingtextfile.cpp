
#include "streamingtextfile.h"

StreamingTextFile::StreamingTextFile(PGFileHandle handle, std::string filename) :
	handle(handle), TextFile(filename), decoder(nullptr),
	output(nullptr), intermediate_buffer(nullptr),
	linenr(0), current_buffer(nullptr), max_length(-1), 
	current_width(0), prev_character('\0') {
	read_only = true;
	ReadBlock();
	is_loaded = true;
}

StreamingTextFile::~StreamingTextFile() {
	if (output) {
		free(output);
	}
	if (intermediate_buffer) {
		free(intermediate_buffer);
	}
	panther::CloseFile(handle);
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		delete *it;
	}
}

bool StreamingTextFile::ReadBlock() {
	if (!handle) return false;

	Lock(PGWriteLock);
	char buffer[PANTHER_BUFSIZ + 1];
	size_t bufsiz = panther::ReadFromFile(handle, buffer, PANTHER_BUFSIZ);
	char* buf = buffer;
	if (bufsiz == 0) {
		if (buffers.size() == 0) {
			ConsumeBytes("", 0, max_length, current_width, current_buffer, linenr, prev_character);
			this->encoding = PGEncodingUTF8;
		}
		panther::CloseFile(handle);
		handle = nullptr;
		return false;
	}

	// FIXME: we should consume bytes until we fill one complete buffer, always
	// how do we do this? 
	// don't use ConsumeBytes, instead, read X bytes where X = BUFSIZ
	// read until last newline character, and add all that to a new buffer
	// remaining text gets cached in the StreamingTextFile and will be used next time
	// if no newline: add all text to a new buffer and read next block (repeat until newline is found)

	if (encoding == PGEncodingUnknown) {
		// guess encoding from the buffer
		this->encoding = PGGuessEncoding((unsigned char*)buffer, std::min((size_t)1024, bufsiz));
		if (encoding != PGEncodingUTF8 || encoding != PGEncodingUTF8BOM) {
			decoder = PGCreateEncoder(this->encoding, PGEncodingUTF8);
		} else {
			if (((unsigned char*)buffer)[0] == 0xEF &&
				((unsigned char*)buffer)[1] == 0xBB &&
				((unsigned char*)buffer)[2] == 0xBF) {
				// skip UTF-8 BOM byte order mark
				buf += 3;
				bufsiz -= 3;
			}
		}
	}
	if (decoder) {
		bufsiz = PGConvertText(decoder, buf, bufsiz, &output, &output_size, &intermediate_buffer, &intermediate_size);
		buf = output;
	}
	ConsumeBytes(buf, bufsiz, max_length, current_width, current_buffer, linenr, prev_character);

	linecount = linenr;
	total_width = current_width;

	Unlock(PGWriteLock);
	return true;
}

std::shared_ptr<TextFile> StreamingTextFile::OpenTextFile(std::string filename, PGFileError& error, bool ignore_binary) {
	PGFileHandle handle = panther::OpenFile(filename, PGFileReadOnly, error);
	if (!handle) {
		return nullptr;
	}
	return std::shared_ptr<StreamingTextFile>(new StreamingTextFile(handle, filename));
}

TextLine StreamingTextFile::GetLine(lng linenumber) {
	if (linenumber < 0)
		return TextLine();
	return TextLine(GetBuffer(linenumber), linenumber);
}

void StreamingTextFile::InsertText(std::vector<Cursor>& cursors, char character) {
	return;
}

void StreamingTextFile::InsertText(std::vector<Cursor>& cursors, PGUTF8Character character) {
	return;
}

void StreamingTextFile::InsertText(std::vector<Cursor>& cursors, std::string text) {
	return;
}

void StreamingTextFile::DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction) {
	return;
}

void StreamingTextFile::DeleteWord(std::vector<Cursor>& cursors, PGDirection direction) {
	return;
}

void StreamingTextFile::AddNewLine(std::vector<Cursor>& cursors) {
	return;
}

void StreamingTextFile::AddNewLine(std::vector<Cursor>& cursors, std::string text) {
	return;
}

void StreamingTextFile::DeleteLines(std::vector<Cursor>& cursors) {
	return;
}

void StreamingTextFile::DeleteLine(std::vector<Cursor>& cursors, PGDirection direction) {
	return;
}

void StreamingTextFile::AddEmptyLine(std::vector<Cursor>& cursors, PGDirection direction) {
	return;
}

void StreamingTextFile::MoveLines(std::vector<Cursor>& cursors, int offset) {
	return;
}

std::string StreamingTextFile::GetText() {
	assert(0);
	return "";
}

std::string StreamingTextFile::CutText(std::vector<Cursor>& cursors) {
	return CopyText(cursors);
}

std::string StreamingTextFile::CopyText(std::vector<Cursor>& cursors) {
	std::string text = "";
	if (!is_loaded) return text;
	// FIXME: read lock?
	if (Cursor::CursorsContainSelection(cursors)) {
		bool first_copy = true;
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (!(it->SelectionIsEmpty())) {
				if (!first_copy) {
					text += NEWLINE_CHARACTER;
				}
				text += it->GetText();
				first_copy = false;
			}
		}
	} else {
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (it != cursors.begin()) {
				text += NEWLINE_CHARACTER;
			}
			text += it->GetLine();
		}
	}
	return text;
}

void StreamingTextFile::PasteText(std::vector<Cursor>& cursors, std::string& text) {
	return;
}

void StreamingTextFile::RegexReplace(std::vector<Cursor>& cursors, PGRegexHandle regex, std::string& replacement) {
	return;
}

bool StreamingTextFile::Reload(PGFileError& error) {
	return false;
}

void StreamingTextFile::ChangeIndentation(PGLineIndentation indentation) {
	return;
}

void StreamingTextFile::RemoveTrailingWhitespace() {
	return;
}

void StreamingTextFile::Undo(TextView* view) {
	return;
}

void StreamingTextFile::Redo(TextView* view) {
	return;
}

void StreamingTextFile::SaveChanges() {
	return;
}

void StreamingTextFile::SetLanguage(PGLanguage* language) {
	return;
}

lng StreamingTextFile::GetLineCount() {
	//assert(0);
	return linecount;
}

void StreamingTextFile::IndentText(std::vector<Cursor>& cursors, PGDirection direction) {
	return;
}

void StreamingTextFile::VerifyPartialTextfile() {
	return;
}

void StreamingTextFile::VerifyTextfile() {
	return;
}

PGScalar StreamingTextFile::GetMaxLineWidth(PGFontHandle font) {
	if (!is_loaded) return 0;
	assert(max_line_length.buffer);
	return GetTextFontSize(font) / 10.0 * max_line_length.buffer->line_lengths[max_line_length.position];
}

TextFile::PGStoreFileType StreamingTextFile::WorkspaceFileStorage() {
	return PGFileTooLarge;
}

PGTextRange StreamingTextFile::FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap) {
	PGTextBuffer* start_buffer = GetBuffer(start_line);
	PGTextBuffer* end_buffer = GetBuffer(end_line);
	lng start_position = start_buffer->GetBufferLocationFromCursor(start_line, start_character);
	lng end_position = end_buffer->GetBufferLocationFromCursor(end_line, end_character);
	return FindMatch(regex_handle, direction, start_buffer, start_position, end_buffer, end_position, wrap);
}

PGTextRange StreamingTextFile::FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap) {
	assert(0);
	return PGTextRange();
}

void StreamingTextFile::FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data) {

}

void StreamingTextFile::ConvertToIndentation(PGLineIndentation indentation) {
	return;
}

PGTextBuffer* StreamingTextFile::GetBuffer(lng line) {
	while (line > linecount && handle) {
		ReadBlock();
	}
	return buffers[PGTextBuffer::GetBuffer(buffers, line)];
}

PGTextBuffer* StreamingTextFile::GetBufferFromWidth(double width) {
	while (width > total_width && handle) {
		ReadBlock();
	}
	return buffers[PGTextBuffer::GetBufferFromWidth(buffers, width)];
}

PGTextBuffer* StreamingTextFile::GetFirstBuffer() {
	return buffers.front();
}

PGTextBuffer* StreamingTextFile::GetLastBuffer() {
	return buffers.back();
}


#include "streamingtextfile.h"
#include "style.h"

StreamingTextFile::StreamingTextFile(PGFileHandle handle, std::string filename) :
	handle(handle), TextFile(filename), decoder(nullptr),
	output(nullptr), intermediate_buffer(nullptr),
	cached_buffer(nullptr), cached_index(0), cached_size(0) {
	read_only = true;
	this->encoding = PGEncodingUnknown;
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

struct BufferData {
	char* data;
	size_t length;

	BufferData(char* data, size_t length) : data(data), length(length) { }
};

lng StreamingTextFile::ReadIntoBuffer(char*& buffer, lng& start_index) {
	if (!buffer) {
		free(buffer);
		buffer = nullptr;
	}
	start_index = 0;
	lng bufsiz = TEXT_BUFFER_SIZE + 1;
	buffer = (char*)malloc(sizeof(char) * (TEXT_BUFFER_SIZE + 1));
	bufsiz = panther::ReadFromFile(handle, buffer, TEXT_BUFFER_SIZE - cached_size);
	if (bufsiz == 0) {
		if (buffers.size() == 0) {
			// FIXME: add initial buffer
			//ConsumeBytes("", 0, max_length, current_width, current_buffer, linenr, prev_character);
			assert(0);
			this->encoding = PGEncodingUTF8;
		}
		panther::CloseFile(handle);
		handle = nullptr;
		free(buffer);
		buffer = nullptr;
		return 0;
	}
	if (encoding == PGEncodingUnknown) {
		// guess encoding from the buffer
		this->encoding = PGGuessEncoding((unsigned char*)buffer, std::min((lng)1024, bufsiz));
		if (encoding != PGEncodingUTF8 && encoding != PGEncodingUTF8BOM) {
			decoder = PGCreateEncoder(this->encoding, PGEncodingUTF8);
		} else {
			if (((unsigned char*)buffer)[0] == 0xEF &&
				((unsigned char*)buffer)[1] == 0xBB &&
				((unsigned char*)buffer)[2] == 0xBF) {
				// skip UTF-8 BOM byte order mark
				start_index = 3;
			}
		}
	}
	if (decoder) {
		bufsiz = PGConvertText(decoder, buffer, bufsiz, &output, &output_size, &intermediate_buffer, &intermediate_size);
		free(buffer);
		buffer = nullptr;
		start_index = 0;
		if (bufsiz >= 0) {
			buffer = output;
			output = nullptr;
			output_size = 0;
		}
	}
	return bufsiz;
}

bool StreamingTextFile::ReadBlock() {
	if (!handle) return false;

	LockMutex(text_lock.get());

	lng start_index = 0;
	char* buf = nullptr;
	lng bufsiz = ReadIntoBuffer(buf, start_index);
	if (bufsiz <= start_index) {
		UnlockMutex(text_lock.get());
		return false;
	}
#ifdef PANTHER_DEBUG
	for (size_t i = cached_index; i < cached_size; i++) {
		// there should be no more newlines remaining in the cached buffer
		assert(cached_buffer[i] != '\n' && cached_buffer[i] != '\r');
	}
#endif

	// look for the last newline in the buffer
	size_t total_size = cached_size;
	std::vector<BufferData> buffers;
	lng p, cp = 0;
	while (bufsiz > start_index) {
		for (p = bufsiz - 1; p >= start_index; p--) {
			if (buf[p] == '\n' || buf[p] == '\r') {
				cp = p + 1;
				if (buf[p] == '\n' && p > 0 && buf[p - 1] == '\r') {
					p--;
				}
				p--;
				break;
			}
		}
		if (p < start_index) {
			// no newline found
			// we need to read until we find a newline, or until nothing is left in the buffer
			buffers.push_back(BufferData(buf, bufsiz));
			total_size += bufsiz;

			bufsiz = ReadIntoBuffer(buf, start_index);
			if (bufsiz <= start_index) {
				break;
			}
		} else {
			// found a newline
			total_size += p;
			break;
		}
	}
	// create a new buffer holding the data
	// note: we only need to search the last buffer for new line characters
	PGTextBuffer* last_buffer = this->buffers.size() > 0 ? this->buffers.back() : nullptr;
	PGTextBuffer* new_buffer = new PGTextBuffer(nullptr, total_size, last_buffer ? last_buffer->start_line + last_buffer->line_count : 0);

	if (last_buffer) {
		last_buffer->_next = new_buffer;
		new_buffer->_prev = last_buffer;
		new_buffer->cumulative_width = last_buffer->cumulative_width + last_buffer->width;
		new_buffer->start_line = last_buffer->start_line + last_buffer->line_count;
		new_buffer->index = last_buffer->index + 1;
	}

	size_t position = 0;
	// first copy the cached data into the buffer
	if (cached_buffer) {
		assert(memchr(cached_buffer + cached_index, '\n', cached_size) == nullptr);
		memcpy(new_buffer->buffer, cached_buffer + cached_index, cached_size);
		position += cached_size;
	}
	// now copy any previous buffers into the array
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		memcpy(new_buffer->buffer + position, it->data, it->length);
		position += it->length;
		free(it->data);
	}
	// now move onto the last buffer
	// this buffer we have to scan for newline characters
	size_t prev_position = 0;
	size_t prev_buffer_position = 0;
	for (size_t i = start_index; i < p; i++) {
		if (buf[i] == '\n' || buf[i] == '\r' || i + 1 == bufsiz) {
			memcpy(new_buffer->buffer + position, buf + prev_position, i - prev_position);
			position += i - prev_position;
			new_buffer->buffer[position++] = '\n';
			size_t cp = i;
			if (buf[i] == '\r' && i + 1 < bufsiz && buf[i + 1] == '\n') {
				i++;
			}
			// newline
			new_buffer->line_start.push_back(position);
			PGScalar length = MeasureTextWidth(PGStyleManager::default_font, new_buffer->buffer + prev_buffer_position, position - prev_buffer_position - 1);
			if (max_line_length.buffer == nullptr || length > max_line_length.buffer->line_lengths[max_line_length.position]) {
				max_line_length.buffer = new_buffer;
				max_line_length.position = new_buffer->line_count;
			}
			new_buffer->line_lengths.push_back(length);
			prev_position = i + 1;
			prev_buffer_position = position;

			linecount++;
			new_buffer->width += length;
			new_buffer->line_count++;
			total_width += length;
		}
	}
	new_buffer->current_size = position;

	new_buffer->next_callback = [](PGTextBuffer* buffer, void* data) {
		StreamingTextFile* file = (StreamingTextFile*)data;
		file->ReadBlock();
	};
	new_buffer->callback_data = this;
	new_buffer->VerifyBuffer();
	this->buffers.push_back(new_buffer);

	// delete the old cache, as it has been placed into a buffer now
	if (cached_buffer) {
		free(cached_buffer);
		cached_buffer = nullptr;
		cached_size = cached_index = 0;
	}
	// move the remainder of the data into the cached buffer, if there is any
	if (cp < bufsiz) {
		cached_buffer = buf;
		cached_size = bufsiz - cp;
		cached_index = cp;
	} else {
		// no data remaining in the current buffer; simply delete it
		free(buf);
	}
	VerifyTextfile();

	if (highlighter) {
		HighlightText();
	}
	UnlockMutex(text_lock.get());
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

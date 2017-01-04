
#include "textfield.h"
#include "textfile.h"
#include "text.h"
#include <algorithm>
#include <fstream>
#include "scheduler.h"
#include "unicode.h"
#include "regex.h"

struct OpenFileInformation {
	TextFile* textfile;
	std::string path;

	OpenFileInformation(TextFile* file, std::string path) : textfile(file), path(path) {}
};

void TextFile::OpenFileAsync(Task* task, void* inp) {
	OpenFileInformation* info = (OpenFileInformation*)inp;
	info->textfile->OpenFile(info->path);
	delete info;
}

TextFile::TextFile(BasicTextField* textfield) : textfield(textfield), highlighter(nullptr) {
	this->path = "";
	this->name = std::string("untitled");
	this->text_lock = CreateMutex();
	this->buffers.push_back(new PGTextBuffer("\n", 1, 0));
	cursors.push_back(new Cursor(this));
	this->linecount = 1;
	is_loaded = true;
	unsaved_changes = true;
}

TextFile::TextFile(BasicTextField* textfield, std::string path, bool immediate_load) : textfield(textfield), highlighter(nullptr), path(path) {
	this->name = path.substr(path.find_last_of(GetSystemPathSeparator()) + 1);
	lng pos = path.find_last_of('.');
	this->ext = pos == std::string::npos ? std::string("") : path.substr(pos + 1);
	this->current_task = nullptr;
	this->text_lock = CreateMutex();
	loaded = 0;

	this->language = PGLanguageManager::GetLanguage(ext);
	if (this->language) {
		highlighter = this->language->CreateHighlighter();
	}
	unsaved_changes = false;
	is_loaded = false;
	// FIXME: switch to immediate_load for small files
	if (!immediate_load) {
		OpenFileInformation* info = new OpenFileInformation(this, path);
		this->current_task = new Task((PGThreadFunctionParams)OpenFileAsync, info);
		Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
	} else {
		OpenFile(path);
		is_loaded = true;
	}
}

TextFile::~TextFile() {
	pending_delete = true;
	if (current_task) {
		current_task->active = false;
		current_task = nullptr;
	}
	if (find_task) {
		find_task->active = false;
		find_task = nullptr;
	}
	// we only proceed with deleting after we have obtained a lock on everything
	if (is_loaded) {
		// if loaded we make sure there are no read locks and no write locks
		Lock(PGWriteLock);
	} else {
		// otherwise we only need the text_lock, which is used by the loading code
		LockMutex(text_lock);
	}
	DestroyMutex(text_lock);
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		delete *it;
	}
	for (auto it = deltas.begin(); it != deltas.end(); it++) {
		delete *it;
	}
	for (auto it = redos.begin(); it != redos.end(); it++) {
		delete *it;
	}
	if (highlighter) {
		delete highlighter;
	}
}

PGTextBuffer* TextFile::GetBuffer(lng line) {
	return buffers[PGTextBuffer::GetBuffer(buffers, line)];
}

void TextFile::RunHighlighter(Task* task, TextFile* textfile) {
	for (lng i = 0; i < (lng)textfile->buffers.size(); i++) {
		if (!textfile->buffers[i]->parsed) {
			// if we encounter a non-parsed block, parse it and any subsequent blocks that have to be parsed
			lng current_block = i;
			while (current_block < (lng)textfile->buffers.size()) {
				if (textfile->current_task != task) {
					// a new syntax highlighting task has been activated
					// stop parsing with possibly outdated information
					textfile->buffers[current_block]->parsed = false;
					return;
				}
				textfile->Lock(PGWriteLock);
				PGTextBuffer* buffer = textfile->buffers[current_block];
				PGParseErrors errors;
				PGParserState oldstate = buffer->state;
				PGParserState state = current_block == 0 ? textfile->highlighter->GetDefaultState() : textfile->buffers[current_block - 1]->state;

				lng linecount = buffer->GetLineCount(textfile->GetLineCount());
				assert(linecount > 0);
				if (buffer->syntax) {
					free(buffer->syntax);
				}
				buffer->syntax = (PGSyntax*)malloc(sizeof(PGSyntax) * linecount);

				int index = 0;
				for (auto it = TextLineIterator(textfile, textfile->buffers[current_block]); ; it++) {
					TextLine line = it.GetLine();
					state = textfile->highlighter->IncrementalParseLine(line, i, state, errors);
					buffer->syntax[index++] = line.syntax;
					if (index == linecount) break;
				}
				buffer->parsed = true;
				buffer->state = textfile->highlighter->CopyParserState(state);
				bool equivalent = !oldstate ? false : textfile->highlighter->StateEquivalent(textfile->buffers[current_block]->state, oldstate);
				if (oldstate) {
					textfile->highlighter->DeleteParserState(oldstate);
				}
				textfile->Unlock(PGWriteLock);
				if (equivalent) {
					break;
				}
				current_block++;
			}
		}
	}
	textfile->current_task = nullptr;
}

int TextFile::GetLineHeight() {
	if (!textfield) return -1;
	return textfield->GetLineHeight();
}

void TextFile::InvalidateParsing() {
	assert(is_loaded);
	if (!highlighter) return;

	this->current_task = new Task((PGThreadFunctionParams)RunHighlighter, (void*) this);
	Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
}

void TextFile::Lock(PGLockType type) {
	assert(is_loaded);
	if (type == PGWriteLock) {
		while (true) {
			if (shared_counter == 0) {
				LockMutex(text_lock);
				if (shared_counter != 0) {
					UnlockMutex(text_lock);
				} else {
					break;
				}
			}
		}
	} else if (type == PGReadLock) {
		// ensure there are no write operations by locking the TextLock
		LockMutex(text_lock);
		// notify threads we are only reading by incrementing the shared counter
		shared_counter++;
		UnlockMutex(text_lock);
	}
}

void TextFile::Unlock(PGLockType type) {
	assert(is_loaded);
	if (type == PGWriteLock) {
		UnlockMutex(text_lock);
	} else if (type == PGReadLock) {
		LockMutex(text_lock);
		// decrement the shared counter
		shared_counter--;
		UnlockMutex(text_lock);
	}
}

void TextFile::OpenFile(std::string path) {
	lng size = 0;
	char* base = (char*)panther::ReadFile(path, size);
	if (!base || size < 0) {
		// FIXME: proper error message
		return;
	}

	this->lineending = PGLineEndingUnknown;
	this->indentation = PGIndentionTabs;
	char* ptr = base;
	char* prev = base;
	int offset = 0;
	loaded = 0;
	LockMutex(text_lock);
	lng linenr = 0;
	PGTextBuffer* current_buffer = nullptr;
	while (*ptr) {
		int character_offset = utf8_character_length(*ptr);
		assert(character_offset >= 0); // invalid UTF8, FIXME: throw error message or something else
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
			char* line_start = prev;
			lng line_size = (lng)((ptr - prev) - offset);
			if (current_buffer == nullptr ||
				(current_buffer->current_size + line_size >= (current_buffer->buffer_size - current_buffer->buffer_size / 10))) {
				// create a new buffer
				PGTextBuffer* new_buffer = new PGTextBuffer(line_start, line_size, linenr);
				if (current_buffer) current_buffer->next = new_buffer;
				new_buffer->prev = current_buffer;
				current_buffer = new_buffer;
				current_buffer->buffer[current_buffer->current_size++] = '\n';
				buffers.push_back(current_buffer);
			} else {
				//add line to the current buffer
				memcpy(current_buffer->buffer + current_buffer->current_size, line_start, line_size);
				current_buffer->current_size += line_size;
				current_buffer->buffer[current_buffer->current_size++] = '\n';
			}
			assert(current_buffer->current_size < current_buffer->buffer_size);

			linenr++;
			loaded = ((double)(ptr - base)) / size;

			prev = ptr + 1;
			offset = 0;

			if (pending_delete) {
				panther::DestroyFileContents(base);
				UnlockMutex(text_lock);
				return;
			}
		}
		ptr += character_offset;
	}
	{
		// add the final line
		char* line_start = prev;
		lng line_size = (lng)(ptr - prev);
		if (current_buffer == nullptr ||
			(current_buffer->current_size + line_size >= (current_buffer->buffer_size - current_buffer->buffer_size / 10))) {
			// create a new buffer
			PGTextBuffer* new_buffer = new PGTextBuffer(line_start, line_size, linenr);
			if (current_buffer) current_buffer->next = new_buffer;
			new_buffer->prev = current_buffer;
			current_buffer = new_buffer;
			current_buffer->buffer[current_buffer->current_size++] = '\n';
			buffers.push_back(current_buffer);
		} else {
			//add line to the current buffer
			memcpy(current_buffer->buffer + current_buffer->current_size, line_start, line_size);
			current_buffer->current_size += line_size;
			current_buffer->buffer[current_buffer->current_size++] = '\n';
		}
		linenr++;
	}
	if (linenr == 0) {
		lineending = GetSystemLineEnding();
		current_buffer = new PGTextBuffer("", 1, 0);
		buffers.push_back(current_buffer);
		linenr++;
	}
	linecount = linenr;

	cursors.push_back(new Cursor(this));

	panther::DestroyFileContents(base);
	loaded = 1;

	if (highlighter) {
		// we parse the first 10 blocks before opening the textfield for viewing
		// (heuristic: probably should be dependent on highlight speed/amount of text/etc)
		// anything else we schedule for highlighting in a separate thread
		// first create all the textblocks
		lng current_line = 0;
		// parse the first 10 blocks
		PGParseErrors errors;
		PGParserState state = highlighter->GetDefaultState();
		for (lng i = 0; i < (lng)std::min((size_t)10, buffers.size()); i++) {
			auto lines = buffers[i]->GetLines();
			buffers[i]->syntax = (PGSyntax*)malloc(sizeof(PGSyntax) * lines.size());
			int index = 0;
			for (auto it = lines.begin(); it != lines.end(); it++) {
				state = highlighter->IncrementalParseLine(*it, i, state, errors);
				buffers[i]->syntax[index++] = (*it).syntax;
			}
			buffers[i]->state = highlighter->CopyParserState(state);
			buffers[i]->parsed = true;
		}
		highlighter->DeleteParserState(state);
		if (buffers.size() > 10) {
			this->current_task = new Task((PGThreadFunctionParams)RunHighlighter, (void*) this);
			Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
		}
	}
	is_loaded = true;
	UnlockMutex(text_lock);
}

// FIXME: "file has been modified without us being the one that modified it"
TextLine TextFile::GetLine(lng linenumber) {
	if (!is_loaded) return TextLine();
	if (linenumber < 0 || linenumber >= linecount)
		return TextLine();
	return TextLine(GetBuffer(linenumber), linenumber);
}

lng TextFile::GetLineCount() {
	if (!is_loaded) return 0;
	return linecount;
}

void TextFile::SetMaxLineWidth(lng new_width) {
	if (new_width < 0) {
		lng max = 0;
		for (auto it = buffers.begin(); it != buffers.end(); it++) {
			max = std::max((lng)(*it)->current_size, max);
		}
		longest_line = max;
	} else {
		longest_line = new_width;
	}
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

void TextFile::InsertText(char character) {
	if (!is_loaded) return;
	InsertText(std::string(1, character));
}

void TextFile::InsertText(PGUTF8Character u) {
	if (!is_loaded) return;
	InsertText(std::string((char*)u.character, u.length));
}

void TextFile::InsertText(std::string text, size_t i) {
	Cursor* cursor = cursors[i];
	assert(cursor->SelectionIsEmpty());
	lng insert_point = cursor->start_buffer_position;
	PGTextBuffer* buffer = cursor->start_buffer;
	buffer->parsed = false;
	PGBufferUpdate update = PGTextBuffer::InsertText(buffers, cursor->start_buffer, insert_point, text, this->linecount);
	// since the split point can be BEFORE this cursor
	// we need to look at previous cursors as well
	// cursors before this cursor might also have to move to the new buffer
	for (lng j = i - 1; j >= 0; j--) {
		Cursor* c2 = cursors[j];
		if (c2->start_buffer != buffer && c2->end_buffer != buffer) break;
		for (int bufpos = 0; bufpos < 2; bufpos++) {
			if (update.new_buffer != nullptr) {
				if (c2->BUFPOS(bufpos) > update.split_point) {
					c2->BUF(bufpos) = update.new_buffer;
					c2->BUFPOS(bufpos) -= update.split_point;
				}
			}
		}
		assert(c2->start_buffer_position < c2->start_buffer->current_size);
		assert(c2->end_buffer_position < c2->end_buffer->current_size);
	}
	for (size_t j = i; j < cursors.size(); j++) {
		Cursor* c2 = cursors[j];
		if (c2->start_buffer != buffer) break;
		for (int bufpos = 0; bufpos < 2; bufpos++) {
			if (update.new_buffer == nullptr) {
				c2->BUFPOS(bufpos) += update.split_point;
			} else {
				if (c2->BUFPOS(bufpos) > update.split_point) {
					// cursor moves to new buffer
					c2->BUF(bufpos) = update.new_buffer;
					c2->BUFPOS(bufpos) -= update.split_point;
					if (insert_point >= update.split_point) {
						c2->BUFPOS(bufpos) += text.size();
					}
				} else {
					c2->BUFPOS(bufpos) += text.size();
				}
			}
		}
		assert(c2->start_buffer_position < c2->start_buffer->current_size);
		assert(c2->end_buffer_position < c2->end_buffer->current_size);
	}
	VerifyTextfile();
}

void TextFile::DeleteSelection(int i) {
	Cursor* cursor = cursors[i];
	assert(!cursor->SelectionIsEmpty());

	auto begin = cursor->BeginCursorPosition();
	auto end = cursor->EndCursorPosition();

	begin.buffer->parsed = false;
	end.buffer->parsed = false;

	PGTextBuffer* buffer = begin.buffer;
	lng lines_deleted = 0;
	lng delete_size;
	if (end.buffer != begin.buffer) {
		lines_deleted += begin.buffer->DeleteLines(begin.position);
		buffer = buffer->next;

		lng buffer_position = PGTextBuffer::GetBuffer(buffers, begin.buffer->start_line);
		while (buffer != end.buffer) {
			lines_deleted += buffer->GetLineCount(this->GetLineCount());
			buffers.erase(buffers.begin() + buffer_position + 1);
			PGTextBuffer* next = buffer->next;
			delete buffer;
			buffer = next;
		}

		// we need to move everything that is AFTER the selection
		// but IN the current line from "endbuffer" to "beginbuffer"
		// look for the first newline character in "endbuffer"
		lng split_point = end.position;
		for (int i = split_point; i < buffer->current_size; i++) {
			if (buffer->buffer[i] == '\n') {
				// copy the text into beginbuffer
				// for implementation simplicity, we don't split on this insertion
				// if the text does not fit in beginbuffer, we simply extend beginbuffer
				if (i > split_point) {
					std::string text = std::string(buffer->buffer + split_point, i - split_point);
					if (begin.buffer->current_size + text.size() >= begin.buffer->buffer_size) {
						begin.buffer->Extend(begin.buffer->current_size + text.size() + 10);
					}
					begin.buffer->InsertText(begin.position, text);
				}
				split_point = i;
				break;
			}
		}
		if (split_point < buffer->current_size - 1) {
			// only deleting part of end buffer
			buffer->start_line -= lines_deleted;
			lines_deleted += buffer->DeleteLines(0, split_point + 1);
			begin.buffer->next = end.buffer;
			end.buffer->prev = begin.buffer;
			// we know there are no cursors within the selection
			// because overlapping cursors are not allowed
			// however, we have to update any cursors after the selection
			for (int j = i + 1; j < cursors.size(); j++) {
				if (cursors[j]->start_buffer != end.buffer &&
					cursors[j]->end_buffer != end.buffer) break;
				for (int bufpos = 0; bufpos < 2; bufpos++) {
					if (cursors[j]->BUF(bufpos) == end.buffer) {
						if (cursors[j]->BUFPOS(bufpos) < split_point) {
							// the cursor occurs on the text we moved to the begin line
							// this means we have to move the cursor the begin buffer
							cursors[j]->BUF(bufpos) = begin.buffer;
							cursors[j]->BUFPOS(bufpos) = cursors[j]->BUFPOS(bufpos) + begin.position - end.position;
						} else {
							// otherwise, we offset the cursor by the deleted amount
							cursors[j]->BUFPOS(bufpos) -= split_point + 1;
						}
					}
				}
			}
		} else {
			// have to delete entire end buffer
			lines_deleted += end.buffer->GetLineCount(this->GetLineCount());
			begin.buffer->next = end.buffer->next;
			if (begin.buffer->next) begin.buffer->next->prev = begin.buffer;
			buffers.erase(buffers.begin() + buffer_position + 1);
			delete end.buffer;
			// there can still be cursors in the end buffer
			// AFTER the selection but BEFORE the split point
			// we have to move these cursors to the begin buffer
			for (int j = i + 1; j < cursors.size(); j++) {
				if (cursors[j]->start_buffer != end.buffer &&
					cursors[j]->end_buffer != end.buffer) break;
				for (int bufpos = 0; bufpos < 2; bufpos++) {
					if (cursors[j]->BUF(bufpos) == end.buffer) {
						cursors[j]->BUF(bufpos) = begin.buffer;
						cursors[j]->BUFPOS(bufpos) += begin.position;
					}
				}
			}
		}
	} else {
		// begin buffer = end buffer
		lng deleted_text = end.position - begin.position;
		// in this case, we only need to update cursors in this buffer
		lines_deleted += begin.buffer->DeleteLines(begin.position, end.position);
		for (int j = i + 1; j < cursors.size(); j++) {
			if (cursors[j]->start_buffer != begin.buffer &&
				cursors[j]->end_buffer != begin.buffer) break;
			for (int bufpos = 0; bufpos < 2; bufpos++) {
				if (cursors[j]->BUF(bufpos) == begin.buffer) {
					cursors[j]->BUFPOS(bufpos) -= deleted_text;
				}
			}
		}
	}
	// update the current cursor so it only select the start of the selection
	cursor->start_buffer = begin.buffer;
	cursor->start_buffer_position = begin.position;
	cursor->end_buffer_position = cursor->start_buffer_position;
	cursor->end_buffer = cursor->start_buffer;

	// update start_line of the buffers
	buffer = end.buffer->next;
	while (buffer) {
		buffer->start_line -= lines_deleted;
		buffer = buffer->next;
	}
	linecount -= lines_deleted;

	VerifyTextfile();
}

// insert text into the textfile at each cursors' position
// text cannot contain newlines
void TextFile::InsertText(std::string text) {
	if (!is_loaded) return;

	TextDelta* delta = new AddText(text);
	PerformOperation(delta);
}

void TextFile::InsertLines(std::vector<std::string>& lines, size_t i) {
	Cursor* cursor = cursors[i];
	assert(cursor->SelectionIsEmpty());
	// InsertText should be used to insert text to a line
	assert(lines.size() > 1);
	lng added_lines = lines.size() - 1;
	// the first line gets added to the current line we are on
	if (lines[0].size() > 0) {
		InsertText(lines[0], i);
	}

	lng start_position = cursor->start_buffer_position;
	PGTextBuffer* start_buffer = cursor->start_buffer;

	lng position = cursor->start_buffer_position;
	PGTextBuffer* buffer = cursor->start_buffer;

	lng cursor_offset = 0;
	lng final_line_size = -1;
	lng line_position = 0;
	PGTextBuffer* extra_buffer = nullptr;

	buffer->parsed = false;

	lng start_line = buffer->start_line;
	for (lng i = 0; i < position; i++) {
		if (buffer->buffer[i] == '\n') {
			start_line++;
		}
	}

	if (position < buffer->current_size - 1) {
		// there is some text in the buffer that we have to move
		// we add the text to the to-be-inserted lines
		// the remainder of the current line gets appended to the last line
		// the other lines get added to a new buffer we create
		std::string& current_line = lines.back();
		final_line_size = current_line.size();
		for (lng i = position; i < buffer->current_size; i++) {
			if (buffer->buffer[i] == '\n' || i == buffer->current_size - 1) {
				line_position = i;
				cursor_offset = line_position - position;
				current_line += std::string(buffer->buffer + position, i - position);
				if (i < buffer->current_size - 1) {
					// this is not the last line in the buffer
					// add the remaining lines to "extra_buffer"
					extra_buffer = new PGTextBuffer(buffer->buffer + i + 1, buffer->current_size - (i + 1), start_line + added_lines + 1);
				}
				break;
			}
		}
		buffer->buffer[position] = '\n';
		buffer->current_size = position + 1;
	}
	position = buffer->current_size;

	lng buffer_position = PGTextBuffer::GetBuffer(buffers, buffer->start_line) + 1;
	lng current_lines = 1;
	for (auto it = lines.begin() + 1; it != lines.end(); it++) {
		if ((*it).size() + 1 >= buffer->buffer_size - buffer->current_size) {
			start_line += current_lines;
			// line does not fit within the current buffer: have to make a new buffer
			PGTextBuffer* new_buffer = new PGTextBuffer((*it).c_str(), (*it).size(), start_line);
			new_buffer->next = buffer->next;
			if (new_buffer->next) new_buffer->next->prev = new_buffer;
			new_buffer->prev = buffer;
			buffer->next = new_buffer;
			buffer = new_buffer;
			current_lines = 1;
			buffer->buffer[buffer->current_size++] = '\n';
			position = buffer->current_size;

			buffers.insert(buffers.begin() + buffer_position, new_buffer);
			buffer_position++;
		} else {
			// line fits within the current buffer
			if ((*it).size() > 0) {
				buffer->InsertText(position, (*it));
			}
			buffer->buffer[buffer->current_size++] = '\n';
			position = buffer->current_size;
			current_lines++;
		}
	}
	cursor->start_buffer = buffer;
	cursor->start_buffer_position = buffer->current_size - cursor_offset - 1;
	cursor->end_buffer = cursor->start_buffer;
	cursor->end_buffer_position = cursor->start_buffer_position;
	if (extra_buffer) {
		// insert the extra buffer, if we have it
		extra_buffer->next = buffer->next;
		if (extra_buffer->next) extra_buffer->next->prev = extra_buffer;
		buffer->next = extra_buffer;
		extra_buffer->prev = buffer;
		buffers.insert(buffers.begin() + buffer_position, extra_buffer);
		buffer = extra_buffer;
	}
	for (size_t j = i + 1; j < cursors.size(); j++) {
		if (cursors[j]->start_buffer != start_buffer &&
			cursors[j]->end_buffer != start_buffer) break;
		for (int bufpos = 0; bufpos < 2; bufpos++) {
			if (cursors[j]->BUFPOS(bufpos) >= start_position) {
				if (cursors[j]->BUFPOS(bufpos) < line_position) {
					// the is on the same line as our initial cursor
					// it is now part of the final buffer we inserted
					cursors[j]->BUF(bufpos) = buffer;
					cursors[j]->BUFPOS(bufpos) = final_line_size + (line_position - cursors[j]->BUFPOS(bufpos));
				} else {
					// the cursor occurs in the same initial buffer
					// but not on the same line
					// thus the cursor now points to the extra_buffer
					cursors[j]->BUF(bufpos) = extra_buffer;
					cursors[j]->BUFPOS(bufpos) = cursors[j]->BUFPOS(bufpos) - line_position;
				}
			}
		}
	}
	buffer = buffer->next;
	while (buffer) {
		buffer->start_line += added_lines;
		buffer = buffer->next;
	}
	linecount += added_lines;
	VerifyTextfile();
}

// insert text into the textfile at each cursors' position
// text can contain newlines
void TextFile::InsertLines(std::vector<std::string>& lines) {
	if (!is_loaded) return;

	TextDelta* delta = new AddLines(lines);
	PerformOperation(delta);
}

std::vector<std::string> TextFile::SplitLines(std::string text) {
	std::vector<std::string> lines;
	lng start = 0;
	lng current = 0;
	for (auto it = text.begin(); it != text.end(); it++) {
		if (*it == '\r') {
			if (*(it + 1) == '\n') {
				current++;
				it++;
			} else {
				lines.push_back(text.substr(start, current - start));
				start = current + 1;
				current = start;
				continue;
			}
		}
		if (*it == '\n') {
			lines.push_back(text.substr(start, current - start));
			start = current + 1;
			current = start;
			continue;
		}
		current++;
	}
	lines.push_back(text.substr(start, current - start));
	return lines;
}

void TextFile::OffsetLineOffset(lng offset) {
	lineoffset_y = lineoffset_y + offset;
	lineoffset_y = std::max(std::min(lineoffset_y, linecount - 1), (lng)0);
}

void TextFile::SetCursorLocation(lng line, lng character) {
	ClearExtraCursors();
	cursors[0]->SetCursorLocation(line, character);
	if (textfield) textfield->SelectionChanged();
}

void TextFile::SetCursorLocation(lng start_line, lng start_character, lng end_line, lng end_character) {
	ClearExtraCursors();
	cursors[0]->SetCursorLocation(end_line, end_character);
	Cursor::NormalizeCursors(this, cursors, true);
	if (textfield) textfield->SelectionChanged();
}

void TextFile::AddNewCursor(lng line, lng character) {
	cursors.push_back(new Cursor(this, line, character));
	active_cursor = cursors.back();
	Cursor::NormalizeCursors(this, cursors, false);
}

void TextFile::OffsetLine(lng offset) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetLine(offset);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetSelectionLine(lng offset) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetSelectionLine(offset);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetCharacter(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if ((*it)->SelectionIsEmpty()) {
			(*it)->OffsetCharacter(direction);
		} else {
			PGCursorPosition pos = direction == PGDirectionLeft ? (*it)->BeginPosition() : (*it)->EndPosition();
			(*it)->SetCursorLocation(pos.line, pos.character);
		}
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetSelectionCharacter(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetSelectionCharacter(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetWord(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetWord(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetSelectionWord(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetSelectionWord(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetStartOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetStartOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::SelectStartOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->SelectStartOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetStartOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetStartOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::SelectStartOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->SelectStartOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetEndOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetEndOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::SelectEndOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->SelectEndOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetEndOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->OffsetEndOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::SelectEndOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		(*it)->SelectEndOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::ClearExtraCursors() {
	if (cursors.size() > 1) {
		for (auto it = cursors.begin() + 1; it != cursors.end(); it++) {
			delete *it;
		}
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	active_cursor = cursors.front();
}

void TextFile::ClearCursors() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		delete *it;
	}
	cursors.clear();
	active_cursor = nullptr;
}

Cursor*& TextFile::GetActiveCursor() {
	if (active_cursor == nullptr)
		active_cursor = cursors.front();
	return active_cursor;
}

void TextFile::SelectEverything() {
	ClearCursors();
	this->cursors.push_back(new Cursor(this, linecount - 1, GetLine(linecount - 1).GetLength(), 0, 0));
	if (this->textfield) textfield->SelectionChanged();
}

struct Interval {
	lng start_line;
	lng end_line;
	std::vector<Cursor*> cursors;
	Interval(lng start, lng end, Cursor* cursor) : start_line(start), end_line(end) { cursors.push_back(cursor); }
};

std::vector<Interval> TextFile::GetCursorIntervals() {
	assert(is_loaded);
	std::vector<Interval> intervals;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		auto begin_position = (*it)->BeginPosition();
		auto end_position = (*it)->BeginPosition();
		intervals.push_back(Interval(begin_position.line, end_position.line, *it));
	}
	// if we get here we know the movement is possible
	// first merge the different intervals so each interval is "standalone" (i.e. not adjacent to another interval)
	for (lng i = 0; i < (lng)intervals.size(); i++) {
		Interval& interval = intervals[i];
		for (lng j = i + 1; j < (lng)intervals.size(); j++) {
			if ((interval.start_line >= intervals[j].start_line - 1 && interval.start_line <= intervals[j].end_line + 1) ||
				(interval.end_line >= intervals[j].start_line - 1 && interval.end_line <= intervals[j].end_line + 1)) {
				// intervals overlap, merge the two intervals
				interval.start_line = std::min(interval.start_line, intervals[j].start_line);
				interval.end_line = std::max(interval.end_line, intervals[j].end_line);
				for (auto it = intervals[j].cursors.begin(); it != intervals[j].cursors.end(); it++) {
					interval.cursors.push_back(*it);
				}
				intervals.erase(intervals.begin() + j);
				j--;
			}
		}
	}
	return intervals;
}

void TextFile::MoveLines(int offset) {
	if (!is_loaded) return;
	assert(0);
}

void TextFile::DeleteLines() {
	if (!is_loaded) return;
	assert(0);
}

void TextFile::DeleteLine(PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new RemoveText(direction, PGDeltaRemoveLine);
	PerformOperation(delta);
}

void TextFile::AddEmptyLine(PGDirection direction) {
	if (!is_loaded) return;
	assert(0);
}

std::string TextFile::CopyText() {
	std::string text = "";
	if (!is_loaded) return text;
	if (CursorsContainSelection(cursors)) {
		bool first_copy = true;
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (!(*it)->SelectionIsEmpty()) {
				if (!first_copy) {
					text += NEWLINE_CHARACTER;
				}
				text += (*it)->GetText();
				first_copy = false;
			}
		}
	} else {
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (it != cursors.begin()) {
				text += NEWLINE_CHARACTER;
			}
			text += (*it)->GetLine();
		}
	}
	return text;
}

void TextFile::PasteText(std::string& text) {
	if (!is_loaded) return;

	std::vector<std::string> pasted_lines = SplitLines(text);
	if (pasted_lines.size() == 1) {
		InsertText(pasted_lines[0]);
	} else {
		InsertLines(pasted_lines);
	}
}

void TextFile::VerifyTextfile() {
	//#ifdef _DEBUG
	for (size_t i = 0; i < buffers.size(); i++) {
		if (i < buffers.size() - 1) {
			assert(buffers[i]->start_line < buffers[i + 1]->start_line);
			assert(buffers[i]->next == buffers[i + 1]);
			// only the final buffer can end with a non-newline character
			assert(buffers[i]->buffer[buffers[i]->current_size - 1] == '\n');
		}
		if (i > 0) {
			assert(buffers[i]->prev == buffers[i - 1]);
		}
		assert(buffers[i]->current_size < buffers[i]->buffer_size);
	}
	for (int i = 0; i < cursors.size(); i++) {
		Cursor* c = cursors[i];
		assert(i == cursors.size() - 1 || Cursor::CursorOccursFirst(c, cursors[i + 1]));
		assert(c->start_buffer_position >= 0 && c->start_buffer_position < c->start_buffer->current_size);
		assert(c->end_buffer_position >= 0 && c->end_buffer_position < c->end_buffer->current_size);
		assert(std::find(buffers.begin(), buffers.end(), c->start_buffer) != buffers.end());
		assert(std::find(buffers.begin(), buffers.end(), c->end_buffer) != buffers.end());
	}
	//#endif
}


void TextFile::DeleteCharacter(PGDirection direction, size_t i) {
	Cursor* cursor = cursors[i];
	assert(cursor->SelectionIsEmpty());
	lng delete_point = cursor->start_buffer_position;
	PGTextBuffer* buffer = cursor->start_buffer;
	buffer->parsed = false;

	if (direction == PGDirectionLeft && (delete_point == 0 || buffer->buffer[delete_point - 1] == '\n')) {
		// backward delete line
		cursor->OffsetSelectionCharacter(PGDirectionLeft);
		if (!cursor->SelectionIsEmpty())
			DeleteSelection(i);
	} else if (direction == PGDirectionRight && (delete_point >= buffer->current_size - 1 || buffer->buffer[delete_point] == '\n')) {
		if (delete_point == buffer->current_size) return;
		// forward delete line
		cursor->OffsetSelectionCharacter(PGDirectionRight);
		if (!cursor->SelectionIsEmpty())
			DeleteSelection(i);
	} else {
		lng delete_size = direction == PGDirectionRight ? utf8_character_length(buffer->buffer[delete_point]) : (delete_point - utf8_prev_character(buffer->buffer, delete_point));
		delete_point = direction == PGDirectionRight ? delete_point : delete_point - delete_size;
		PGBufferUpdate update = PGTextBuffer::DeleteText(buffers, cursor->start_buffer, delete_point, delete_size);
		for (size_t j = i; j < cursors.size(); j++) {
			Cursor* c2 = cursors[j];
			if (c2->start_buffer != buffer && c2->start_buffer != update.new_buffer &&
				c2->end_buffer != buffer && c2->end_buffer != update.new_buffer)
				break;
			for (int bufpos = 0; bufpos < 2; bufpos++) {
				if (update.new_buffer == nullptr) {
					if (c2->BUFPOS(bufpos) > delete_point)
						c2->BUFPOS(bufpos) -= update.split_point;
				} else {
					if (c2->BUF(bufpos) == buffer) {
						if (c2->BUFPOS(bufpos) > delete_point)
							c2->BUFPOS(bufpos) -= delete_size;
					} else {
						c2->BUF(bufpos) = buffer;
						c2->BUFPOS(bufpos) += update.split_point;
					}
				}
			}
			assert(c2->start_buffer_position < c2->start_buffer->current_size);
			assert(c2->end_buffer_position < c2->end_buffer->current_size);
		}
	}
}

void TextFile::DeleteCharacter(PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new RemoveText(direction, PGDeltaRemoveText);
	PerformOperation(delta);
}

void TextFile::DeleteWord(PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new RemoveText(direction, PGDeltaRemoveWord);
	PerformOperation(delta);
}

void TextFile::AddNewLine() {
	if (!is_loaded) return;
	AddNewLine("");
}

void TextFile::AddNewLine(std::string text) {
	if (!is_loaded) return;
	std::vector<std::string> lines;
	lines.push_back("");
	lines.push_back(text);
	InsertLines(lines);
}

void TextFile::SetUnsavedChanges(bool changes) {
	if (changes != unsaved_changes) {
		RefreshWindow(textfield->GetWindow());
	}
	unsaved_changes = changes;
}

void TextFile::Undo() {
	if (!is_loaded) return;
	SetUnsavedChanges(true);
	if (this->deltas.size() == 0) return;
	TextDelta* delta = this->deltas.back();
	Lock(PGWriteLock);
	this->Undo(delta);
	Unlock(PGWriteLock);
	this->deltas.pop_back();
	this->redos.push_back(delta);
	if (this->textfield) {
		this->textfield->TextChanged();
	}
	InvalidateParsing();
}

void TextFile::Redo() {
	if (!is_loaded) return;
	if (this->redos.size() == 0) return;
	TextDelta* delta = this->redos.back();
	SetUnsavedChanges(true);
	current_task = nullptr;
	// lock the blocks
	Lock(PGWriteLock);
	VerifyTextfile();
	// perform the operation
	PerformOperation(delta, true);
	// release the locks again
	VerifyTextfile();
	Unlock(PGWriteLock);
	this->redos.pop_back();
	this->deltas.push_back(delta);
	if (this->textfield) {
		this->textfield->TextChanged();
	}
	// invalidate any lines for parsing
	InvalidateParsing();
}

void TextFile::AddDelta(TextDelta* delta) {
	if (!is_loaded) return;
	for (auto it = redos.begin(); it != redos.end(); it++) {
		delete *it;
	}
	redos.clear();
	this->deltas.push_back(delta);
}

void TextFile::PerformOperation(TextDelta* delta) {
	// set the current_task to the nullptr, this will cause any active syntax highlighting to end
	// this prevents long syntax highlighting tasks (e.g. highlighting a long document) from 
	// locking us out of editing until they're finished
	SetUnsavedChanges(true);
	current_task = nullptr;
	// this should be an assertion
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	// lock the blocks
	Lock(PGWriteLock);
	VerifyTextfile();
	// perform the operation
	PerformOperation(delta, false);
	// release the locks again
	VerifyTextfile();
	Unlock(PGWriteLock);
	AddDelta(delta);
	if (this->textfield) {
		this->textfield->TextChanged();
	}
	// invalidate any lines for parsing
	InvalidateParsing();
}

void TextFile::PerformOperation(TextDelta* delta, bool redo) {
	switch (delta->type) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		bool selection = !redo && CursorsContainSelection(cursors);
		RemoveText* text = nullptr;
		if (selection) {
			add->next = new RemoveText(PGDirectionLeft, PGDeltaRemoveText);
			text = (RemoveText*)add->next;
		}
		for (size_t i = 0; i < cursors.size(); i++) {
			if (!cursors[i]->SelectionIsEmpty()) {
				if (!redo) {
					text->removed_text.push_back(cursors[i]->GetText());
					add->next->stored_cursors.push_back(BackupCursor(i));
				}
				DeleteSelection(i);
			} else if (selection) {
				text->removed_text.push_back("");
			}
			InsertText(add->text, i);
			if (!redo) {
				add->stored_cursors.push_back(BackupCursor(i));
			}
		}
		break;
	}
	case PGDeltaAddLines: {
		AddLines* add = (AddLines*)delta;
		for (size_t i = 0; i < cursors.size(); i++) {
			if (!cursors[i]->SelectionIsEmpty()) {
				// first delete the selection, if any
				// FIXME: add deleted text to undo if redo == false
				DeleteSelection(i);
			}
			InsertLines(add->lines, i);
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		bool delete_selection = CursorsContainSelection(cursors);
		for (size_t i = 0; i < cursors.size(); i++) {
			if (delete_selection) {
				if (!cursors[i]->SelectionIsEmpty()) {
					if (!redo) {
						remove->removed_text.push_back("");
					}
					DeleteSelection(i);
				} else if (!redo) {
					remove->removed_text.push_back("");
				}
			} else {
				if (!redo) {
					remove->removed_text.push_back("");
				}
				DeleteCharacter(remove->direction, i);
			}
		}
		Cursor::NormalizeCursors(this, cursors, true);
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveText* remove = (RemoveText*)delta;
		RemoveText remove_text = RemoveText(remove->direction, PGDeltaRemoveText);

		for (int i = 0; i < cursors.size(); i++) {
			if (remove->direction == PGDirectionLeft) {
				cursors[i]->SelectStartOfLine();
			} else {
				cursors[i]->SelectEndOfLine();
			}
		}
		Cursor::NormalizeCursors(this, cursors, false);
		PerformOperation(&remove_text, redo);
		break;
	}
	case PGDeltaRemoveWord: {
		RemoveText* remove = (RemoveText*)delta;
		RemoveText remove_text = RemoveText(remove->direction, PGDeltaRemoveText);
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if ((*it)->SelectionIsEmpty()) {
				(*it)->OffsetSelectionWord(remove->direction);
			}
		}
		PerformOperation(&remove_text, redo);
		break;
	}
	default:
		assert(0);
		break;
	}
}

void TextFile::Undo(AddText& delta, int i) {
	cursors[i]->OffsetSelectionPosition(-(lng)delta.text.size());
	DeleteSelection(i);
}

void TextFile::Undo(RemoveText& delta, std::string& text, int i) {
	if (text.size() > 0) {
		std::vector<std::string> lines = SplitLines(text);
		if (lines.size() == 1) {
			InsertText(text, i);
		} else {
			InsertLines(lines, i);
		}
	}
}

void TextFile::Undo(TextDelta* delta) {
	switch (delta->type) {
	case PGDeltaAddText: {
		ClearCursors();
		AddText* add = (AddText*)delta;
		// we perform undo's in reverse order
		for (int i = 0; i < delta->stored_cursors.size(); i++) {
			int index = delta->stored_cursors.size() - (i + 1);
			cursors.insert(cursors.begin(), RestoreCursor(delta->stored_cursors[index]));
			Undo(*add, 0);
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		ClearCursors();
		for (int i = 0; i < delta->stored_cursors.size(); i++) {
			int index = delta->stored_cursors.size() - (i + 1);
			cursors.insert(cursors.begin(), RestoreCursorPartial(delta->stored_cursors[index]));
			Undo(*remove, remove->removed_text[index], 0);
			delete cursors[0];
			cursors[0] = RestoreCursor(delta->stored_cursors[index]);
		}
		break;
	}
	}
	if (delta->next) {
		Undo(delta->next);
	}
}

void TextFile::SaveChanges() {
	if (!is_loaded) return;
	if (this->path == "") return;
	SetUnsavedChanges(false);
	this->Lock(PGReadLock);
	PGLineEnding line_ending = lineending;
	if (line_ending != PGLineEndingWindows && line_ending != PGLineEndingMacOS && line_ending != PGLineEndingUnix) {
		line_ending = GetSystemLineEnding();
	}
	// FIXME: respect file encoding
	// FIXME: handle errors properly
	PGFileHandle handle = panther::OpenFile(this->path, PGFileReadWrite);
	lng position = 0;
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		char* line = (*it)->buffer;
		lng prev_position = 0;
		lng i = 0;
		for (i = 0; i <= (*it)->current_size; i++) {
			if (line[i] == '\n') {
				// new line
				panther::WriteToFile(handle, line + prev_position, i - prev_position);
				switch (line_ending) {
				case PGLineEndingWindows:
					panther::WriteToFile(handle, "\r\n", 2);
					break;
				case PGLineEndingMacOS:
					panther::WriteToFile(handle, "\r", 1);
					break;
				case PGLineEndingUnix:
					panther::WriteToFile(handle, "\n", 1);
					break;
				default:
					assert(0);
					break;
				}
				prev_position = i + 1;
			}
		}
		if (prev_position != (*it)->current_size) {
			assert((*it) == buffers.back());
			panther::WriteToFile(handle, line + prev_position, i - prev_position);
		}
	}
	this->Unlock(PGReadLock);
	panther::CloseFile(handle);
}

void TextFile::ChangeLineEnding(PGLineEnding lineending) {
	if (!is_loaded) return;
	this->lineending = lineending;
}

void TextFile::ChangeFileEncoding(PGFileEncoding encoding) {
	if (!is_loaded) return;
	assert(0);
}

void TextFile::ChangeIndentation(PGLineIndentation indentation) {
	if (!is_loaded) return;

}

void TextFile::RemoveTrailingWhitespace() {
	if (!is_loaded) return;

}

std::vector<CursorData> TextFile::BackupCursors() {
	std::vector<CursorData> backup;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		backup.push_back((*it)->GetCursorData());
	}
	return backup;
}

CursorData TextFile::BackupCursor(int i) {
	return cursors[i]->GetCursorData();
}

void TextFile::RestoreCursors(std::vector<CursorData>& data) {
	ClearCursors();
	for (auto it = data.begin(); it != data.end(); it++) {
		cursors.push_back(new Cursor(this, *it));
	}
	Cursor::NormalizeCursors(this, cursors);
}

Cursor* TextFile::RestoreCursor(CursorData data) {
	return new Cursor(this, data);
}

Cursor* TextFile::RestoreCursorPartial(CursorData data) {
	if (data.start_line < data.end_line ||
		(data.start_line == data.end_line && data.start_position < data.end_position)) {
		data.end_line = data.start_line;
		data.end_position = data.start_position;
	} else {
		data.start_line = data.end_line;
		data.start_position = data.end_position;
	}
	return new Cursor(this, data);
}

PGFindMatch TextFile::FindMatch(std::string text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex, Task* current_task) {
	if (!match_case) {
		text = utf8_tolower(text);
	}

	// we start "outside" of the current selection
	// e.g. if we go right, we start at the end and continue right
	// if we go left, we start at the beginning and go left
	lng begin_line = direction == PGDirectionLeft ? start_line : end_line;
	lng begin_character = direction == PGDirectionLeft ? start_character : end_character;
	int offset = direction == PGDirectionLeft ? -1 : 1;
	lng end = direction == PGDirectionLeft ? 0 : linecount - 1;
	lng i = begin_line;
	if (!regex) {
		bool wrapped_search = false;
		while (true) {
			for (; i != end; i += offset) {
				if (current_task != nullptr && find_task != current_task) {
					return PGFindMatch(-1, -1, -1, -1);
				}
				TextLine textline = GetLine(i);
				std::string line = std::string(textline.GetLine(), textline.GetLength());
				// FIXME: don't use utf8_tolower, it has slow performance
				if (!match_case) {
					line = utf8_tolower(line);
				}
				size_t pos = std::string::npos;
				if (i == begin_line) {
					// at the beginning line, we have to keep the begin_character in mind
					size_t start = 0, end = line.size();
					std::string l;
					if (direction == PGDirectionLeft) {
						// if we search left, we search UP TO the begin character
						l = line.substr(0, begin_character);
						pos = l.rfind(text);
					} else {
						// if we search right, we search STARTING FROM the begin character
						l = line.substr(begin_character);
						pos = l.find(text);
						if (pos != std::string::npos) {
							// correct "pos" to account for the substr we did
							pos += begin_character;
						}
					}
				} else {
					pos = direction == PGDirectionLeft ? line.rfind(text) : line.find(text);
				}
				if (pos != std::string::npos) {
					return PGFindMatch(pos, i, pos + text.size(), i);
				}
			}
			if (wrap && !wrapped_search) {
				// if we wrap, we also search the rest of the text
				i = direction == PGDirectionLeft ? linecount - 1 : 0;
				end = begin_line + offset;
				// we set begin_line to -1, so we search the starting line entirely
				// this is because the match might be part of the selection
				// e.g. consider we have a string "ab|cd" and we search for "abc"
				// if we first search right of the cursor and then left we don't find the match
				// thus we search the final line in its entirety after we wrap
				begin_line = -1;
				wrapped_search = true;
			} else {
				break;
			}
		}
		return PGFindMatch(-1, -1, -1, -1);
	} else {
		// FIXME: split text on newline
		PGRegexHandle regex = PGCompileRegex(text, match_case ? PGRegexFlagsNone : PGRegexCaseInsensitive);
		if (!regex) {
			// FIXME: throw an error, regex was not compiled properly
			*error_message = panther::strdup("Error");
			return PGFindMatch(-1, -1, -1, -1);
		}
		bool wrapped_search = false;
		while (true) {
			for (; i != end; i += offset) {
				if (current_task != nullptr && find_task != current_task) {
					PGDeleteRegex(regex);
					return PGFindMatch(-1, -1, -1, -1);
				}
				TextLine textline = GetLine(i);

				std::string line = std::string(textline.GetLine(), textline.GetLength());
				PGRegexMatch match;

				size_t pos = std::string::npos;
				if (i == begin_line) {
					size_t start = 0, end = line.size();
					size_t addition = 0;
					std::string l;
					if (direction == PGDirectionLeft) {
						l = line.substr(0, begin_character);
					} else {
						l = line.substr(begin_character);
						addition = begin_character;
					}
					match = PGMatchRegex(regex, l, direction);
					match.start += (int)addition;
					match.end += (int)addition;
				} else {
					match = PGMatchRegex(regex, line, direction);
				}
				if (match.matched) {
					PGDeleteRegex(regex);
					return PGFindMatch(match.start, i, match.end, i);
				}
			}
			if (wrap && !wrapped_search) {
				// if we wrap, we also search the rest of the text
				i = direction == PGDirectionLeft ? linecount - 1 : 0;
				end = begin_line + offset;
				begin_line = -1;
				wrapped_search = true;
			} else {
				break;
			}
		}
		PGDeleteRegex(regex);
		return PGFindMatch(-1, -1, -1, -1);
	}
}

struct FindInformation {
	TextFile* textfile;
	std::string pattern;
	PGDirection direction;
	lng start_line;
	lng start_character;
	lng end_line;
	lng end_character;
	bool match_case;
	bool wrap;
	bool regex;
};

void TextFile::RunTextFinder(Task* task, TextFile* textfile, std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool match_case, bool wrap, bool regex) {
	textfile->Lock(PGWriteLock);
	textfile->finished_search = false;
	textfile->matches.clear();
	textfile->Unlock(PGWriteLock);

	char* error_message = nullptr;
	textfile->Lock(PGReadLock);
	PGFindMatch first_match = textfile->FindMatch(text, direction, start_line, start_character, end_line, end_character, &error_message, match_case, wrap, regex, task);
	textfile->Unlock(PGReadLock);
	// we should never fail here, because we already have compiled the regex
	assert(error_message == nullptr);
	if (first_match.start_line < 0) {
		return; // no matches found
	}

	textfile->Lock(PGWriteLock);
	textfile->SetCursorLocation(first_match.start_line, first_match.start_character, first_match.end_line, first_match.end_character);
	textfile->matches.push_back(first_match);
	textfile->Unlock(PGWriteLock);

	PGFindMatch match = first_match;
	while (true) {
		if (textfile->find_task != task) {
			// a new find task has been activated
			// stop searching for an outdated find query
			return;
		}
		textfile->Lock(PGReadLock);
		match = textfile->FindMatch(text, direction, match.start_line, match.start_character, match.end_line, match.end_character, &error_message, match_case, wrap, regex, task);
		textfile->Unlock(PGReadLock);
		assert(error_message == nullptr);
		if (match.start_line == first_match.start_line &&
			match.start_character == first_match.start_character &&
			match.end_line == first_match.end_line &&
			match.end_character == first_match.end_character) {
			// the current match equals the first match, we are done looking
			textfile->finished_search = true;
			return;
		}
		textfile->Lock(PGWriteLock);
		textfile->matches.push_back(match);
		textfile->Unlock(PGWriteLock);
	}
}

void TextFile::ClearMatches() {
	if (!is_loaded) return;
	find_task = nullptr;
	this->Lock(PGWriteLock);
	matches.clear();
	this->Unlock(PGWriteLock);
	textfield->Invalidate();
}

void TextFile::FindAllMatches(std::string& text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex) {
	if (regex) {
		PGRegexHandle handle = PGCompileRegex(text, match_case ? PGRegexFlagsNone : PGRegexCaseInsensitive);
		if (!handle) {
			*error_message = "Error";
			return;
		}
		PGDeleteRegex(handle);
	}
	if (!is_loaded) return;

	FindInformation* info = new FindInformation();
	info->textfile = this;
	info->pattern = text;
	info->direction = direction;
	info->start_line = start_line;
	info->start_character = start_character;
	info->end_line = end_line;
	info->end_character = end_character;
	info->match_case = match_case;
	info->wrap = wrap;
	info->regex = regex;

	this->find_task = new Task([](Task* t, void* in) {
		FindInformation* info = (FindInformation*)in;
		RunTextFinder(t, info->textfile, info->pattern, info->direction, info->start_line,
			info->start_character, info->end_line, info->end_character, info->match_case,
			info->wrap, info->regex);
		delete info;
	}, (void*)info);
	Scheduler::RegisterTask(this->find_task, PGTaskUrgent);
}

bool TextFile::FindMatch(std::string text, PGDirection direction, char** error_message, bool match_case, bool wrap, bool regex, lng& selected_match, bool include_selection) {
	if (!is_loaded) return false;
	PGFindMatch match;
	if (selected_match >= 0) {
		// if FindAll has been performed, we already have all the matches
		// simply select the next match, rather than searching again
		Lock(PGWriteLock);
		if (matches.size() == 0) return false;
		if (!include_selection) {
			if (direction == PGDirectionLeft) {
				selected_match = selected_match == 0 ? matches.size() - 1 : selected_match - 1;
			} else {
				selected_match = selected_match == matches.size() - 1 ? 0 : selected_match + 1;
			}
		}

		match = matches[selected_match];
		if (match.start_character >= 0) {
			this->SetCursorLocation(match.start_line, match.start_character, match.end_line, match.end_character);
		}
		Unlock(PGWriteLock);
		return true;
	}
	// otherwise, search only for the next match in either direction
	*error_message = nullptr;
	Lock(PGReadLock);
	auto begin = GetActiveCursor()->BeginPosition();
	auto end = GetActiveCursor()->EndPosition();
	match = this->FindMatch(text, direction,
		include_selection ? end.line : begin.line,
		include_selection ? end.position : begin.position,
		include_selection ? begin.line : end.line,
		include_selection ? begin.position : end.position,
		error_message, match_case, wrap, regex, nullptr);
	Unlock(PGReadLock);
	if (!(*error_message)) {
		Lock(PGWriteLock);
		if (match.start_character >= 0) {
			SetCursorLocation(match.start_line, match.start_character, match.end_line, match.end_character);
		}
		matches.clear();
		if (match.start_character >= 0) {
			matches.push_back(match);
		}
		Unlock(PGWriteLock);
		return match.start_character >= 0;
	}
	return false;
}

void TextFile::SelectMatches() {
	cursors.clear();
	for (auto it = matches.begin(); it != matches.end(); it++) {
		Cursor* c = new Cursor(this, (*it).start_line, (*it).start_character);
		cursors.push_back(c);
		c->SetCursorStartLocation((*it).end_line, (*it).end_character);
	}
}

TextLineIterator TextFile::GetIterator(lng linenumber) {
	return TextLineIterator(this, linenumber);
}

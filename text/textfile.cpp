
#include "textfield.h"
#include "textfile.h"
#include "text.h"
#include <algorithm>
#include <fstream>
#include "scheduler.h"
#include "unicode.h"
#include "regex.h"
#include "wrappedtextiterator.h"

#include "projectexplorer.h"
#include "controlmanager.h"
#include "statusbar.h"

#include "style.h"

#include "inmemorytextfile.h"

TextFile::TextFile() :
	highlighter(nullptr), bytes(0), total_bytes(1), last_modified_time(-1), last_modified_notification(-1),
	last_modified_deletion(false), saved_undo_count(0), read_only(false), reload_on_changed(true),
	error(PGFileSuccess) {
	this->path = "";
	this->name = std::string("untitled");
	this->text_lock = std::unique_ptr<PGMutex>(CreateMutex());
	this->loading_lock = std::unique_ptr<PGMutex>(CreateMutex());
	this->indentation = PGIndentionTabs;
	this->tabwidth = 4;
	this->encoding = PGEncodingUTF8;
	is_loaded = true;
	unsaved_changes = false;
	saved_undo_count = 0;
}

TextFile::TextFile(std::string path)  :
	highlighter(nullptr), path(path),
	bytes(0), total_bytes(1), is_loaded(false), last_modified_time(-1),
	last_modified_notification(-1), last_modified_deletion(false), saved_undo_count(0), read_only(false),
	encoding(PGEncodingUTF8), reload_on_changed(true), error(PGFileSuccess), tabwidth(4) {

	this->name = path.substr(path.find_last_of(GetSystemPathSeparator()) + 1);
	lng pos = path.find_last_of('.');
	this->ext = pos == std::string::npos ? std::string("") : path.substr(pos + 1);
	this->current_task = nullptr;
	this->text_lock = std::unique_ptr<PGMutex>(CreateMutex());
	this->loading_lock = std::unique_ptr<PGMutex>(CreateMutex());

	this->language = PGLanguageManager::GetLanguage(ext);
	if (this->language) {
		highlighter = std::unique_ptr<SyntaxHighlighter>(this->language->CreateHighlighter());
	}
	unsaved_changes = false;
}

TextFile::~TextFile() {

}


void TextFile::AddTextView(std::shared_ptr<TextView> view) {
	views.push_back(std::weak_ptr<TextView>(view));
}

void TextFile::PendDelete() {
	if (find_task) {
		find_task->active = false;
		find_task = nullptr;
	}
	current_task = nullptr;
	pending_delete = true;
}

void TextFile::InvalidateBuffer(PGTextBuffer* buffer) {
	buffer->parsed = false;
	buffer->line_lengths.clear();
	buffer->cumulative_width = -1;
}

void TextFile::Lock(PGLockType type) {
	assert(is_loaded);
	if (type == PGWriteLock) {
		while (true) {
			if (shared_counter == 0) {
				LockMutex(text_lock.get());
				if (shared_counter != 0) {
					UnlockMutex(text_lock.get());
				} else {
					break;
				}
			}
		}
	} else if (type == PGReadLock) {
		// ensure there are no write operations by locking the TextLock
		LockMutex(text_lock.get());
		// notify threads we are only reading by incrementing the shared counter
		shared_counter++;
		UnlockMutex(text_lock.get());
	}
}

void TextFile::Unlock(PGLockType type) {
	assert(is_loaded);
	if (type == PGWriteLock) {
		UnlockMutex(text_lock.get());
	} else if (type == PGReadLock) {
		LockMutex(text_lock.get());
		// decrement the shared counter
		shared_counter--;
		UnlockMutex(text_lock.get());
	}
}

void TextFile::SetTabWidth(int tabwidth) {
	// FIXME: have to recompute line widths?
	this->tabwidth = tabwidth;
}

bool TextFile::SplitLines(const std::string& text, std::vector<std::string>& lines) {
	lng start = 0;
	lng current = 0;
	lng offset = 0;
	int utf_offset = 0;
	for (const char* ptr = text.c_str(); *ptr; ptr += utf_offset) {
		utf_offset = utf8_character_length(*ptr);
		if (utf_offset <= 0) return false;
		if (*ptr == '\r') {
			if (*(ptr + 1) == '\n') {
				offset = 1;
				ptr++;
			} else {
				lines.push_back(text.substr(start, current - start));
				start = current + 1;
				current = start;
				continue;
			}
		}
		if (*ptr == '\n') {
			lines.push_back(text.substr(start, current - start));
			start = current + 1 + offset;
			current = start;
			offset = 0;
			continue;
		}
		current += utf_offset;
	}
	lines.push_back(text.substr(start, current - start));
	return true;
}

std::vector<std::string> TextFile::SplitLines(const std::string& text) {
	std::vector<std::string> lines;
	SplitLines(text, lines);
	return lines;
}

std::vector<Interval> TextFile::GetCursorIntervals(std::vector<Cursor>& cursors) {
	std::vector<Interval> intervals;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		auto begin_position = it->BeginPosition();
		auto end_position = it->EndPosition();
		intervals.push_back(Interval(begin_position.line, end_position.line, &*it));
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

void TextFile::SetUnsavedChanges(bool changes) {
	// FIXME:
	/*if (changes != unsaved_changes && textfield) {
		RefreshWindow(textfield->GetWindow());
	}*/
	unsaved_changes = changes;
}

void TextFile::SaveAs(std::string path) {
	if (!is_loaded) return;
	this->SetFilePath(path);
	this->SaveChanges();
}

void TextFile::SetFilePath(std::string path) {
	this->path = path;
	this->name = path.substr(path.find_last_of(GetSystemPathSeparator()) + 1);
	lng pos = path.find_last_of('.');
	this->ext = pos == std::string::npos ? std::string("") : path.substr(pos + 1);
}

void TextFile::UpdateModificationTime() {
	auto stats = PGGetFileFlags(path);
	if (stats.flags == PGFileFlagsEmpty) {
		last_modified_time = stats.modification_time;
		last_modified_notification = stats.modification_time;
	}
}

void TextFile::ChangeLineEnding(PGLineEnding lineending) {
	if (!is_loaded) return;
	this->lineending = lineending;
}

void TextFile::ChangeFileEncoding(PGFileEncoding encoding) {
	if (!is_loaded) return;
	assert(0);
}

struct TextFileApplySettings {
	std::weak_ptr<TextFile> file;
	PGTextFileSettings settings;
};

void TextFile::SetSettings(PGTextFileSettings settings) {
	TextFileApplySettings* s = new TextFileApplySettings();
	s->settings = settings;
	s->file = std::weak_ptr<TextFile>(shared_from_this());

	this->OnLoaded([](std::shared_ptr<TextFile> textfile, void* data) {
		TextFileApplySettings* s = (TextFileApplySettings*)data;
		auto file = s->file.lock();
		if (file) {
			file->ApplySettings(s->settings);
		}
	}, [](void* data) {
		TextFileApplySettings* s = (TextFileApplySettings*)data;
		delete s;
	}, s);
}

void TextFile::ApplySettings(PGTextFileSettings settings) {
	if (settings.line_ending != PGLineEndingUnknown) {
		this->lineending = settings.line_ending;
	}
	if (settings.encoding != PGEncodingUnknown) {
		this->encoding = settings.encoding;
	}
	if (settings.name.size() > 0) {
		this->name = settings.name;
	}
}

PGTextFileSettings TextFile::GetSettings() {
	PGTextFileSettings settings;
	settings.line_ending = GetLineEnding();
	settings.name = FileInMemory() ? this->name : "";
	return settings;
}

void TextFile::OnLoaded(PGTextFileLoadedCallback callback, PGTextFileDestructorCallback destructor, void* data) {
	LockMutex(loading_lock.get());
	if (is_loaded) {
		this->Lock(PGWriteLock);
		callback(shared_from_this(), data);
		this->Unlock(PGWriteLock);
		if (data && destructor) {
			destructor(data);
		}
	} else {
		auto d = std::unique_ptr<LoadCallbackData>(new LoadCallbackData());
		d->callback = callback;
		d->destructor = destructor;
		d->data = data;
		loading_data.push_back(std::move(d));
	}
	UnlockMutex(loading_lock.get());
}

void TextFile::FinalizeLoading() {
	is_loaded = true;
	LockMutex(loading_lock.get());
	for (auto it = loading_data.begin(); it != loading_data.end(); it++) {
		(*it)->callback(shared_from_this(), (*it)->data);
	}
	loading_data.clear();
	UnlockMutex(loading_lock.get());
}


void TextFile::_InsertLine(const char* ptr, size_t current, size_t prev, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr) {
	const char* line_start = ptr + prev;
	lng line_size = (lng)(current - prev);
	if (current_buffer == nullptr ||
		(current_buffer->current_size > TEXT_BUFFER_SIZE) ||
		(current_buffer->current_size + line_size + 1 >= (current_buffer->buffer_size - current_buffer->buffer_size / 10))) {
		// create a new buffer
		PGTextBuffer* new_buffer = new PGTextBuffer(line_start, line_size, linenr);
		if (current_buffer) current_buffer->_next = new_buffer;
		new_buffer->_prev = current_buffer;
		current_buffer = new_buffer;
		current_buffer->cumulative_width = current_width;
		current_buffer->buffer[current_buffer->current_size++] = '\n';
		current_buffer->index = buffers.size();
		buffers.push_back(current_buffer);
	} else {
		//add line to the current buffer
		memcpy(current_buffer->buffer + current_buffer->current_size, line_start, line_size);
		current_buffer->line_start.push_back(current_buffer->current_size);
		assert(current_buffer->line_start.size() == 1 ||
			   current_buffer->line_start.back() > current_buffer->line_start[current_buffer->line_start.size() - 2]);
		current_buffer->current_size += line_size;
		current_buffer->buffer[current_buffer->current_size++] = '\n';
	}
	assert(current_buffer->current_size < current_buffer->buffer_size);
	PGScalar length = MeasureTextWidth(PGStyleManager::default_font, line_start, line_size);
	if (length > max_length) {
		max_line_length.buffer = current_buffer;
		max_line_length.position = current_buffer->line_lengths.size();
		max_length = length;
	}
	current_buffer->line_lengths.push_back(length);
	current_buffer->line_count++;
	current_buffer->width += length;
	current_width += length;

	linenr++;
}

void TextFile::_InsertText(const char* ptr, size_t current, size_t prev, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr) {
	if (current_buffer == nullptr) {
		_InsertLine(ptr, current, prev, max_length, current_width, current_buffer, linenr);
		return;
	}
	if (current == prev) {
		return;
	}
	const char* line_start = ptr + prev;
	lng line_size = (lng)(current - prev);

	// add text to the last line of the buffer
	auto update = current_buffer->InsertText(buffers, current_buffer, current_buffer->current_size - 1, std::string(line_start, line_size));
	PGTextBuffer* new_buffer = update.new_buffer ? update.new_buffer : current_buffer;
	// measure the new line width
	PGScalar prev_length = current_buffer->line_lengths.back();
	lng new_line_begin = new_buffer->line_start.size() == 0 ? 0 : new_buffer->line_start.back();
	PGScalar line_length = MeasureTextWidth(PGStyleManager::default_font, new_buffer->buffer + new_line_begin, new_buffer->current_size - 1 - new_line_begin);
	PGScalar added_length = line_length - prev_length;
	if (line_length > max_length) {
		max_line_length.buffer = new_buffer;
		max_line_length.position = new_buffer->line_count - 1;
		max_length = line_length;
	}
	current_buffer->line_lengths.back() = line_length;
	current_buffer->width += added_length;
	if (update.new_buffer) {
		size_t start = current_buffer->line_count;
		for (size_t i = 0; i < update.new_buffer->line_count; i++) {
			current_buffer->width -= current_buffer->line_lengths[start + i];
			update.new_buffer->width += current_buffer->line_lengths[start + i];
			update.new_buffer->line_lengths.push_back(current_buffer->line_lengths[start + i]);
		}
		current_buffer->line_lengths.erase(current_buffer->line_lengths.begin() + start, current_buffer->line_lengths.end());
		update.new_buffer->index = current_buffer->index + 1;
		update.new_buffer->cumulative_width = current_buffer->cumulative_width + current_buffer->width;
		current_buffer = update.new_buffer;
	}
	current_width += added_length;
}

void TextFile::ConsumeBytes(const char* buffer, size_t buffer_size,
									PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr, char& prev_character) {
	size_t prev = 0;
	for (size_t i = 0; i < buffer_size; i++) {
		if (buffer[i] == '\r' || buffer[i] == '\n') {
			// if this is the first character of this buffer, and the last character of the previous
			// buffer was \r, we skip this character as well
			if (!(i == 0 && prev_character == '\r' && buffer[i] == '\n')) {
				if (prev == 0) {
					_InsertText(buffer, i, prev, max_length, current_width, current_buffer, linenr);
				} else {
					_InsertLine(buffer, i, prev, max_length, current_width, current_buffer, linenr);
				}
				if (i + 1 < buffer_size && buffer[i] == '\r' && buffer[i + 1] == '\n') {
					// skip \n in \r\n newline sequence
					i++;
				}
			}
			prev = i + 1;
		}
	}
	_InsertLine(buffer, buffer_size, prev, max_length, current_width, current_buffer, linenr);
	if (buffer_size > 0) {
		prev_character = buffer[buffer_size - 1];
	}
}

void TextFile::ConsumeBytes(const char* buffer, size_t buffer_size, size_t& prev, int& offset,
									PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr) {
	size_t i = 0;
	while (i < buffer_size) {
		int character_offset = utf8_character_length(buffer[i]);
		if (character_offset <= 0) {
			character_offset = 1;
		}
		if (buffer[i] == '\n') {
			// Unix line ending: \n
			if (this->lineending == PGLineEndingUnknown) {
				this->lineending = PGLineEndingUnix;
			} else if (this->lineending != PGLineEndingUnix) {
				this->lineending = PGLineEndingMixed;
			}
		}
		if (buffer[i] == '\r') {
			if (buffer[i + 1] == '\n') {
				offset = 1;
				i++;
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
		if (buffer[i] == '\r' || buffer[i] == '\n') {
			_InsertLine(buffer, i - offset, prev, max_length, current_width, current_buffer, linenr);

			prev = i + 1;
			offset = 0;
		}
		i += character_offset;
	}
	bytes = i;
}

void TextFile::VerifyPartialTextfile() {
#ifdef PANTHER_DEBUG
	lng total_lines = 0;
	for (size_t i = 0; i < buffers.size(); i++) {
		PGTextBuffer* buffer = buffers[i];
		buffer->VerifyBuffer();
		assert(buffers[i]->index == i);
		lng current_lines = 0;
		for (lng j = 0; j < buffer->current_size; j++) {
			if (buffer->buffer[j] == '\n') {
				current_lines++;
			}
		}
		assert(buffers[i]->start_line == total_lines);
		assert(buffers[i]->line_count == current_lines);
		total_lines += current_lines;
		if (i < buffers.size() - 1) {
			assert(buffers[i]->start_line < buffers[i + 1]->start_line);
			assert(buffers[i]->_next == buffers[i + 1]);
			// only the final buffer can end with a non-newline character
			assert(buffers[i]->buffer[buffers[i]->current_size - 1] == '\n');
		}
		if (i > 0) {
			assert(buffers[i]->_prev == buffers[i - 1]);
		}
		assert(buffers[i]->current_size < buffers[i]->buffer_size);
	}
	assert(linecount == total_lines);
	for (lng i = 0; i < views.size(); i++) {
		auto ptr = views[i].lock();
		if (ptr) {
			ptr->VerifyTextView();
		} else {
			views.erase(views.begin() + i);
			i--;
		}
	}
#endif
}

void TextFile::VerifyTextfile() {
#ifdef PANTHER_DEBUG
	VerifyPartialTextfile();
	lng total_lines = 0;
	double measured_width = 0;
	for (size_t i = 0; i < buffers.size(); i++) {
		PGTextBuffer* buffer = buffers[i];
		buffer->VerifyBuffer();
		assert(panther::epsilon_equals(measured_width, buffer->cumulative_width, std::max(0.0001, measured_width / 1000000)));
		measured_width = buffer->cumulative_width;
		assert(buffers[i]->index == i);
		lng current_lines = 0;
		char* ptr = buffer->buffer;
		for (lng j = 0; j < buffer->current_size; j++) {
			if (buffer->buffer[j] == '\n') {
				char* current_ptr = buffer->buffer + j;
				TextLine line = TextLine(ptr, current_ptr - ptr);

				ptr = buffer->buffer + j + 1;

				assert(current_lines < buffer->line_lengths.size());
				PGScalar current_length = buffer->line_lengths[current_lines];
				PGScalar measured_length = MeasureTextWidth(PGStyleManager::default_font, line.GetLine(), line.GetLength());
				assert(panther::epsilon_equals(current_length, measured_length));
				measured_width += measured_length;
				current_lines++;
			}
		}
		assert(buffers[i]->start_line == total_lines);
		assert(buffers[i]->line_count == current_lines);
		total_lines += current_lines;
		if (i < buffers.size() - 1) {
			assert(buffers[i]->start_line < buffers[i + 1]->start_line);
			assert(buffers[i]->_next == buffers[i + 1]);
			// only the final buffer can end with a non-newline character
			assert(buffers[i]->buffer[buffers[i]->current_size - 1] == '\n');
		}
		if (i > 0) {
			assert(buffers[i]->_prev == buffers[i - 1]);
		}
		assert(buffers[i]->current_size < buffers[i]->buffer_size);
	}
#endif
}

void TextFile::HighlightText() {
	for (lng i = 0; i < (lng)this->buffers.size(); i++) {
		if (!this->buffers[i]->parsed) {
			// if we encounter a non-parsed block, parse it and any subsequent blocks that have to be parsed
			lng current_block = i;
			while (current_block < (lng)this->buffers.size()) {
				PGTextBuffer* buffer = this->buffers[current_block];
				PGParseErrors errors;
				PGParserState oldstate = buffer->state;
				PGParserState state = current_block == 0 ? this->highlighter->GetDefaultState() : this->buffers[current_block - 1]->state;

				lng linecount = buffer->GetLineCount();
				assert(linecount > 0);

				buffer->syntax.clear();

				int index = 0;
				for (auto it = TextLineIterator(this->buffers[current_block]); ; it++) {
					TextLine line = it.GetLine();
					PGSyntax syntax;
					state = this->highlighter->IncrementalParseLine(line, i, state, errors, syntax);
					buffer->syntax.push_back(syntax);
					index++;
					if (index == linecount) break;
				}
				buffer->parsed = true;
				buffer->state = this->highlighter->CopyParserState(state);
				bool equivalent = !oldstate ? false : this->highlighter->StateEquivalent(this->buffers[current_block]->state, oldstate);
				if (oldstate) {
					this->highlighter->DeleteParserState(oldstate);
				}
				current_block++;
				if (equivalent) {
					break;
				}
			}
			i = current_block - 1;
		}
	}
}

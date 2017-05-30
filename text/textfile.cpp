
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

struct FindAllInformation {
	ProjectExplorer* explorer;
	std::shared_ptr<PGStatusNotification> notification;
	TextFile* textfile;
	PGRegexHandle regex_handle;
	PGGlobSet whitelist;
	std::vector<PGFile> files;
	bool ignore_binary;
	int context_lines;
	std::shared_ptr<Task> task;
};

struct OpenFileInformation {
	std::shared_ptr<TextFile> file;
	char* base;
	lng size;
	bool delete_file;

	OpenFileInformation(std::shared_ptr<TextFile> file, char* base, lng size, bool delete_file) : file(file), base(base), size(size), delete_file(delete_file) {}
};


TextFile::TextFile() :
	highlighter(nullptr), bytes(0), total_bytes(1), last_modified_time(-1), last_modified_notification(-1),
	last_modified_deletion(false), saved_undo_count(0), read_only(false), reload_on_changed(true),
	error(PGFileSuccess) {
	this->path = "";
	this->name = std::string("untitled");
	this->text_lock = std::unique_ptr<PGMutex>(CreateMutex());
	this->indentation = PGIndentionTabs;
	this->tabwidth = 4;
	this->encoding = PGEncodingUTF8;
	this->buffers.push_back(new PGTextBuffer("\n", 1, 0));
	buffers.back()->line_count = 1;
	buffers.back()->line_lengths.push_back(0);
	max_line_length.buffer = buffers.back();
	max_line_length.position = 0;
	this->linecount = 1;
	is_loaded = true;
	unsaved_changes = false;
	saved_undo_count = 0;
}

TextFile::TextFile(std::string path)  :
	highlighter(nullptr), path(path),
	bytes(0), total_bytes(1), is_loaded(false), last_modified_time(-1),
	last_modified_notification(-1), last_modified_deletion(false), saved_undo_count(0), read_only(false),
	encoding(PGEncodingUTF8), reload_on_changed(true), error(PGFileSuccess) {

	this->name = path.substr(path.find_last_of(GetSystemPathSeparator()) + 1);
	lng pos = path.find_last_of('.');
	this->ext = pos == std::string::npos ? std::string("") : path.substr(pos + 1);
	this->current_task = nullptr;
	this->text_lock = std::unique_ptr<PGMutex>(CreateMutex());

	this->language = PGLanguageManager::GetLanguage(ext);
	if (this->language) {
		highlighter = std::unique_ptr<SyntaxHighlighter>(this->language->CreateHighlighter());
	}
	unsaved_changes = false;
}

void TextFile::OpenFile(std::shared_ptr<TextFile> file, PGFileEncoding encoding, char* base, size_t size, bool immediate_load) {
	file->encoding = encoding;
	if (!immediate_load) {
		OpenFileInformation* info = new OpenFileInformation(file, base, size, false);
		this->current_task = std::shared_ptr<Task>(new Task(
			[](std::shared_ptr<Task> task, void* inp) {
				OpenFileInformation* info = (OpenFileInformation*)inp;
				info->file->OpenFile(info->base, info->size, info->delete_file);
				delete info;
			}
			, info));
		Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
	} else {
		OpenFile(base, size, false);
	}
}

void TextFile::ReadFile(std::shared_ptr<TextFile> file, bool immediate_load, bool ignore_binary) {
	if (!immediate_load) {
		OpenFileInformation* info = new OpenFileInformation(file, nullptr, 0, ignore_binary);
		this->current_task = std::shared_ptr<Task>(new Task(
			[](std::shared_ptr<Task> task, void* inp) {
				OpenFileInformation* info = (OpenFileInformation*)inp;
				info->file->ActuallyReadFile(info->file, info->delete_file);
				delete info;
			}
			, info));
		Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
	} else {
		ActuallyReadFile(file, ignore_binary);
	}
}

#define PANTHER_BUFSIZ 8192
//lng PGConvertText(PGEncoderHandle encoder, const char* input_text, size_t input_size, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {

void TextFile::ActuallyReadFile(std::shared_ptr<TextFile> file, bool ignore_binary) {
	PGFileHandle handle = panther::OpenFile(file->path, PGFileReadOnly, this->error);
	if (!handle) {
		bytes = -1;
		return;
	}
	this->lineending = PGLineEndingUnknown;
	this->indentation = PGIndentionTabs; // FIXME: default from settings
	this->tabwidth = 4; // FIXME: default tabwidth

	LockMutex(text_lock.get());
	lng linenr = 0;
	PGTextBuffer* current_buffer = nullptr;
	PGScalar max_length = -1;
	double current_width = 0;
	char prev_character = '\0';

	this->encoding = PGEncodingUnknown;
	char buffer[PANTHER_BUFSIZ + 1];
	total_bytes = panther::GetFileSize(handle);
	size_t bytes_to_read = total_bytes;
	bytes = 0;
	PGEncoderHandle decoder = nullptr;

	char* output = nullptr;
	lng output_size = 0;
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;
	while (bytes < total_bytes) {
		size_t bytes_read = std::min((size_t) PANTHER_BUFSIZ, bytes_to_read);
		panther::ReadFromFile(handle, buffer, bytes_read);
		bytes_to_read -= bytes_read;
		size_t bufsiz = bytes_read;
		char* buf = buffer;
		if (encoding == PGEncodingUnknown) {
			// first read from file: determine encoding based on sample
			this->encoding = PGGuessEncoding((unsigned char*)buffer, std::min((size_t)1024, bytes_read));
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
		bytes += bytes_read;

		if (file->pending_delete) {
			bytes = -1;
			goto wrapup;
		}
	}
	linecount = linenr;
	total_width = current_width;

	for (auto it = views.begin(); it != views.end(); it++) {
		auto view = it->lock();
		if (view) {
			view->ActuallyApplySettings(view->settings);
		}
	}

	VerifyTextfile();

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
			buffers[i]->syntax.clear();
			int index = 0;
			for (auto it = lines.begin(); it != lines.end(); it++) {
				PGSyntax syntax;
				state = highlighter->IncrementalParseLine(*it, i, state, errors, syntax);
				buffers[i]->syntax.push_back(syntax);
			}
			buffers[i]->state = highlighter->CopyParserState(state);
			buffers[i]->parsed = true;
		}
		highlighter->DeleteParserState(state);
		if (buffers.size() > 10) {
			is_loaded = true;
			this->current_task = std::shared_ptr<Task>(new Task((PGThreadFunctionParams)RunHighlighter, (void*) this));
			Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
		}
	}

	ApplySettings(this->settings);
	is_loaded = true;
wrapup:
	UnlockMutex(text_lock.get());

	if (output) {
		free(output);
	}
	if (intermediate_buffer) {
		free(intermediate_buffer);
	}
	panther::CloseFile(handle);

	/*
	lng size = 0;
	char* base = (char*)panther::ReadFile(file->path, size, error);
	if (!base || size < 0) {
		// FIXME: proper error message
		bytes = -1;
		return;
	}
	if (file->pending_delete) {
		panther::DestroyFileContents(base);
		return;
	}
	char* output_text = nullptr;
	lng output_size = 0;
	PGFileEncoding result_encoding;
	if (!PGTryConvertToUTF8(base, size, &output_text, &output_size, &result_encoding, ignore_binary) || !output_text) {
		error = PGFileEncodingFailure;
		bytes = -1;
		return;
	}
	if (output_text != base) {
		panther::DestroyFileContents(base);
		base = output_text;
		size = output_size;
	}
	if (file->pending_delete) {
		panther::DestroyFileContents(base);
		return;
	}
	auto stats = PGGetFileFlags(file->path);
	file->encoding = result_encoding;
	if (stats.flags == PGFileFlagsEmpty) {
		file->last_modified_time = stats.modification_time;
		file->last_modified_notification = stats.modification_time;
	}
	OpenFile(base, size, true);*/
}


void TextFile::AddTextView(std::shared_ptr<TextView> view) {
	views.push_back(std::weak_ptr<TextView>(view));
}

void TextFile::SetLanguage(PGLanguage* language) {
	if (!this->is_loaded) return;
	this->Lock(PGWriteLock);
	this->language = language;
	this->highlighter = this->language ? std::unique_ptr<SyntaxHighlighter>(this->language->CreateHighlighter()) : nullptr;
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		(*it)->parsed = false;
	}
	this->Unlock(PGWriteLock);
	this->InvalidateParsing();
}

bool TextFile::Reload(PGFileError& error) {
	if (!is_loaded) return true;

	lng size = 0;
	char* base = (char*)panther::ReadFile(path, size, error);
	if (!base || size < 0) {
		// FIXME: proper error message
		return false;
	}
	std::string text = std::string(base, size);
	panther::DestroyFileContents(base);
	if (encoding != PGEncodingUTF8) {
		char* output = nullptr;
		lng output_size = PGConvertText(text, &output, encoding, PGEncodingUTF8);
		if (!output || output_size < 0) {
			error = PGFileEncodingFailure;
			return false;
		}
		text = std::string(output, output_size);
		free(output);
	}

	std::vector<std::string> lines;
	if (!SplitLines(text, lines)) {
		error = PGFileEncodingFailure;
		return false;
	}

	// first backup the cursors
	std::vector<PGTextViewSettings> settings;
	for (lng i = 0; i < views.size(); i++) {
		auto ptr = views[i].lock();
		PGTextViewSettings s;
		if (ptr) {
			s.xoffset = ptr->xoffset;
			s.yoffset = ptr->yoffset;
			s.wordwrap = ptr->wordwrap;
			s.cursor_data = Cursor::BackupCursors(ptr->cursors);
		} else {
			views.erase(views.begin() + i);
			i--;
		}
		settings.push_back(s);
	}

	std::vector<Cursor> cursors;
	cursors.push_back(Cursor(nullptr, PGTextRange(buffers.front(), 0, buffers.back(), buffers.back()->current_size)));

	//this->SelectEverything();
	if (lines.size() == 0 || (lines.size() == 1 && lines[0].size() == 0)) {
		this->DeleteCharacter(cursors, PGDirectionLeft);
	} else {
		panther::replace(text, "\r\n", "\n");
		panther::replace(text, "\r", "\n");
		PasteText(cursors, text);
	}
	this->SetUnsavedChanges(false);
	for(size_t i = 0; i < settings.size(); i++) {
		assert(i < views.size());
		auto ptr = views[i].lock();
		ptr->ApplySettings(settings[i]);
	} 
	//this->ApplySettings(settings);
	return true;
}

std::shared_ptr<TextFile> TextFile::OpenTextFile(std::string filename, PGFileError& error, bool immediate_load, bool ignore_binary) {
	error = PGFileSuccess;
	auto file = std::make_shared<TextFile>(filename);
	file->ReadFile(file, immediate_load, ignore_binary);
	return file;
}

std::shared_ptr<TextFile> TextFile::OpenTextFile(PGFileEncoding encoding, std::string path, char* buffer, size_t buffer_size, bool immediate_load) {
	auto file = std::make_shared<TextFile>(path);
	file->OpenFile(file, encoding, buffer, buffer_size, immediate_load);
	return file;
}

TextFile::~TextFile() {
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		delete *it;
	}
}

void TextFile::PendDelete() {
	if (find_task) {
		find_task->active = false;
		find_task = nullptr;
	}
	current_task = nullptr;
	pending_delete = true;
}

PGTextBuffer* TextFile::GetBuffer(lng line) {
	return buffers[PGTextBuffer::GetBuffer(buffers, line)];
}

void TextFile::RunHighlighter(std::shared_ptr<Task> task, TextFile* textfile) {
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

				lng linecount = buffer->GetLineCount();
				assert(linecount > 0);

				buffer->syntax.clear();

				int index = 0;
				for (auto it = TextLineIterator(textfile, textfile->buffers[current_block]); ; it++) {
					TextLine line = it.GetLine();
					PGSyntax syntax;
					state = textfile->highlighter->IncrementalParseLine(line, i, state, errors, syntax);
					buffer->syntax.push_back(syntax);
					index++;
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

void TextFile::InvalidateBuffer(PGTextBuffer* buffer) {
	buffer->parsed = false;
	buffer->line_lengths.clear();
	buffer->cumulative_width = -1;
}

void TextFile::InvalidateBuffers(TextView* responsible_view) {
	double current_width = 0;
	double current_lines = 0;
	PGTextBuffer* buffer = buffers.front();

	bool find_new_max = false;
	PGScalar current_maximum_size = -1;
	if (max_line_length.buffer == nullptr || max_line_length.buffer->cumulative_width < 0) {
		// the maximum line used to be in an invalidated buffer
		// we have to find the new maximum line
		max_line_length.buffer = nullptr;
		find_new_max = true;
	} else {
		current_maximum_size = max_line_length.buffer->line_lengths[max_line_length.position];
	}

	lng buffer_index = 0;
	while (buffer) {
		assert(buffer->index == buffer_index);
		buffer_index++;
		if (buffer->cumulative_width < 0) {
			// we don't know the length of this buffer
			// get the current length
			double new_width = 0;
			lng linenr = 0;

			char* ptr = buffer->buffer;
			buffer->line_lengths.resize(buffer->line_start.size() + 1);
			for (size_t i = 0; i <= buffer->line_start.size(); i++) {
				lng end_index = ((i == buffer->line_start.size()) ? buffer->current_size : buffer->line_start[i]) - 1;
				char* current_ptr = buffer->buffer + end_index;
				TextLine line = TextLine(ptr, current_ptr - ptr);

				ptr = current_ptr + 1;
				PGScalar line_width = MeasureTextWidth(PGStyleManager::default_font, line.GetLine(), line.GetLength());
				buffer->line_lengths[linenr] = line_width;
				new_width += line_width;
				if (line_width > current_maximum_size) {
					// the current line is bigger than the previous maximum line
					max_line_length.buffer = buffer;
					max_line_length.position = linenr;
					current_maximum_size = line_width;
				}
				linenr++;
			}
			buffer->width = new_width;
			buffer->line_count = linenr;
		} else if (find_new_max) {
			// have to find the current maximum line
			for (lng i = 0; i < buffer->line_lengths.size(); i++) {
				if (buffer->line_lengths[i] > current_maximum_size) {
					max_line_length.buffer = buffer;
					max_line_length.position = i;
					current_maximum_size = buffer->line_lengths[i];
				}
			}
		}
		buffer->cumulative_width = current_width;
		//buffer->start_line = current_lines;
		current_width += buffer->width;
		current_lines += buffer->line_count;
		buffer = buffer->next;
	}
	total_width = current_width;
	linecount = current_lines;

	// invalidate all the views of this textfield
	for (lng i = 0; i < views.size(); i++) {
		auto ptr = views[i].lock();
		if (ptr) {
			// only scroll the view if it was responsible for the changed text
			ptr->InvalidateTextView(ptr.get() == responsible_view);
		} else {
			views.erase(views.begin() + i);
			i--;
		}
	}
}

void TextFile::InvalidateParsing() {
	assert(is_loaded);

	if (!highlighter) return;

	this->current_task = std::shared_ptr<Task>(new Task((PGThreadFunctionParams)RunHighlighter, (void*) this));
	Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
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

void TextFile::_InsertLine(char* ptr, size_t current, size_t prev, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr) {
	char* line_start = ptr + prev;
	lng line_size = (lng)(current - prev);
	if (current_buffer == nullptr ||
		(current_buffer->current_size > TEXT_BUFFER_SIZE) ||
		(current_buffer->current_size + line_size + 1 >= (current_buffer->buffer_size - current_buffer->buffer_size / 10))) {
		// create a new buffer
		PGTextBuffer* new_buffer = new PGTextBuffer(line_start, line_size, linenr);
		if (current_buffer) current_buffer->next = new_buffer;
		new_buffer->prev = current_buffer;
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

void TextFile::_InsertText(char* ptr, size_t current, size_t prev, PGScalar& max_length, double& current_width, PGTextBuffer*& current_buffer, lng& linenr) {
	if (current_buffer == nullptr) {
		_InsertLine(ptr, current, prev, max_length, current_width, current_buffer, linenr);
		return;
	}
	if (current == prev) {
		return;
	}
	char* line_start = ptr + prev;
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
		for(size_t i = 0; i < update.new_buffer->line_count; i++) {
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

void TextFile::ConsumeBytes(char* buffer, size_t buffer_size,
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

void TextFile::ConsumeBytes(char* buffer, size_t buffer_size, size_t& prev, int& offset,
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



void TextFile::OpenFile(char* base, lng size, bool delete_file) {
	this->lineending = PGLineEndingUnknown;
	this->indentation = PGIndentionTabs; // FIXME: default from settings
	this->tabwidth = 4; // FIXME: default tabwidth

	char* ptr = base;
	size_t prev = 0;
	int offset = 0;
	LockMutex(text_lock.get());
	lng linenr = 0;
	PGTextBuffer* current_buffer = nullptr;
	PGScalar max_length = -1;

	double current_width = 0;
	if (((unsigned char*)ptr)[0] == 0xEF &&
		((unsigned char*)ptr)[1] == 0xBB &&
		((unsigned char*)ptr)[2] == 0xBF) {
		// skip byte order mark
		ptr += 3;
		size -= 3;
	}
	bytes = 0;
	total_bytes = size;

	ConsumeBytes(ptr, size, prev, offset, max_length, current_width, current_buffer, linenr);
	// insert the final line
	_InsertLine(ptr, bytes - offset, prev, max_length, current_width, current_buffer, linenr);
	if (linenr == 0) {
		lineending = GetSystemLineEnding();
		current_buffer = new PGTextBuffer("", 1, 0);
		current_buffer->line_count++;
		buffers.push_back(current_buffer);
		max_line_length.buffer = buffers.back();
		max_line_length.position = 0;
		linenr++;
	}
	linecount = linenr;
	total_width = current_width;

	for (auto it = views.begin(); it != views.end(); it++) {
		auto view = it->lock();
		if (view) {
			view->cursors.push_back(Cursor(view.get()));
		}
	}

	if (delete_file) {
		panther::DestroyFileContents(base);
	}
	VerifyTextfile();

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
			buffers[i]->syntax.clear();
			int index = 0;
			for (auto it = lines.begin(); it != lines.end(); it++) {
				PGSyntax syntax;
				state = highlighter->IncrementalParseLine(*it, i, state, errors, syntax);
				buffers[i]->syntax.push_back(syntax);
			}
			buffers[i]->state = highlighter->CopyParserState(state);
			buffers[i]->parsed = true;
		}
		highlighter->DeleteParserState(state);
		if (buffers.size() > 10) {
			is_loaded = true;
			this->current_task = std::shared_ptr<Task>(new Task((PGThreadFunctionParams)RunHighlighter, (void*) this));
			Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
		}
	}

	ApplySettings(this->settings);
	is_loaded = true;
	UnlockMutex(text_lock.get());
}

TextLine TextFile::GetLine(lng linenumber) {
	if (!is_loaded) return TextLine();
	if (linenumber < 0 || linenumber >= linecount)
		return TextLine();
	return TextLine(GetBuffer(linenumber), linenumber);
}

void TextFile::SetTabWidth(int tabwidth) {
	this->tabwidth = tabwidth;
}

lng TextFile::GetLineCount() {
	if (!is_loaded) return 0;
	return linecount;
}

PGScalar TextFile::GetMaxLineWidth(PGFontHandle font) {
	if (!is_loaded) return 0;
	assert(max_line_length.buffer);
	return GetTextFontSize(font) / 10.0 * max_line_length.buffer->line_lengths[max_line_length.position];
}

void TextFile::InsertText(std::vector<Cursor>& cursors, char character) {
	if (!is_loaded) return;
	InsertText(cursors, std::string(1, character));
}

void TextFile::InsertText(std::vector<Cursor>& cursors, PGUTF8Character u) {
	if (!is_loaded) return;
	InsertText(cursors, std::string((char*)u.character, u.length));
}

void TextFile::InsertText(std::vector<Cursor>& cursors, std::string text, size_t i) {
	Cursor& cursor = cursors[i];
	assert(cursor.SelectionIsEmpty());
	lng insert_point = cursor.start_buffer_position;
	PGTextBuffer* buffer = cursor.start_buffer;
	// invalidate parsing of the current buffer
	buffer->parsed = false;
	PGBufferUpdate update = PGTextBuffer::InsertText(buffers, cursor.start_buffer, insert_point, text);

	if (update.new_buffer != nullptr) {
		// since the split point can be BEFORE this cursor
		// we need to look at previous cursors as well
		// cursors before this cursor might also have to move to the new buffer
		for (lng j = i - 1; j >= 0; j--) {
			Cursor& c2 = cursors[j];
			if (c2.start_buffer != buffer && c2.end_buffer != buffer) break;
			for (int bufpos = 0; bufpos < 2; bufpos++) {
				if (c2.BUFPOS(bufpos) > update.split_point) {
					c2.BUF(bufpos) = update.new_buffer;
					c2.BUFPOS(bufpos) -= update.split_point;
				}
			}
			assert(c2.start_buffer_position < c2.start_buffer->current_size);
			assert(c2.end_buffer_position < c2.end_buffer->current_size);
		}
	}

	for (size_t j = i; j < cursors.size(); j++) {
		Cursor& c2 = cursors[j];
		if (c2.start_buffer != buffer) break;
		for (int bufpos = 0; bufpos < 2; bufpos++) {
			if (update.new_buffer == nullptr) {
				c2.BUFPOS(bufpos) += update.split_point;
			} else {
				if (c2.BUFPOS(bufpos) >= update.split_point) {
					// cursor moves to new buffer
					c2.BUF(bufpos) = update.new_buffer;
					c2.BUFPOS(bufpos) -= update.split_point;
					if (insert_point >= update.split_point) {
						c2.BUFPOS(bufpos) += text.size();
					}
				} else {
					c2.BUFPOS(bufpos) += text.size();
				}
			}
		}
		assert(c2.start_buffer_position < c2.start_buffer->current_size);
		assert(c2.end_buffer_position < c2.end_buffer->current_size);
	}
	if (update.new_buffer != nullptr) {
		for (lng index = buffer->index + 1; index < buffers.size(); index++) {
			buffers[index]->index = index;
		}
	}
	InvalidateBuffer(buffer);
}

void TextFile::DeleteSelection(std::vector<Cursor>& cursors, size_t i) {
	Cursor& cursor = cursors[i];
	assert(!cursor.SelectionIsEmpty());

	auto begin = cursor.BeginCursorPosition();
	auto end = cursor.EndCursorPosition();

	begin.buffer->parsed = false;
	end.buffer->parsed = false;

	PGTextBuffer* buffer = begin.buffer;
	lng lines_deleted = 0;
	lng buffers_deleted = 0;
	lng delete_size;
	lng start_index = begin.buffer->index + 1;

	if (end.buffer != begin.buffer) {
		lines_deleted += begin.buffer->DeleteLines(begin.position);
		begin.buffer->line_count -= lines_deleted;

		buffer = buffer->next;

		lng buffer_position = PGTextBuffer::GetBuffer(buffers, begin.buffer);
		while (buffer != end.buffer) {
			if (buffer == max_line_length.buffer) {
				max_line_length.buffer = nullptr;
			}
			lines_deleted += buffer->GetLineCount();
			buffers_deleted++;
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
			// first adjust start_line of end buffer based on previously deleted lines
			end.buffer->start_line -= lines_deleted;
			// now delete the lines in end_buffer and update the line_count
			lng deleted_lines_in_end_buffer = buffer->DeleteLines(0, split_point + 1);
			lines_deleted += deleted_lines_in_end_buffer;
			end.buffer->line_count -= deleted_lines_in_end_buffer;
			// set the index, in case there were any deleted buffers
			end.buffer->index = begin.buffer->index + 1;
			begin.buffer->next = end.buffer;
			end.buffer->prev = begin.buffer;
			start_index = end.buffer->index + 1;
			InvalidateBuffer(end.buffer);

			end.buffer->VerifyBuffer();
			// we know there are no cursors within the selection
			// because overlapping cursors are not allowed
			// however, we have to update any cursors after the selection
			for (int j = i + 1; j < cursors.size(); j++) {
				if (cursors[j].start_buffer != end.buffer &&
					cursors[j].end_buffer != end.buffer) break;
				for (int bufpos = 0; bufpos < 2; bufpos++) {
					if (cursors[j].BUF(bufpos) == end.buffer) {
						if (cursors[j].BUFPOS(bufpos) < split_point) {
							// the cursor occurs on the text we moved to the begin line
							// this means we have to move the cursor the begin buffer
							cursors[j].BUF(bufpos) = begin.buffer;
							cursors[j].BUFPOS(bufpos) = cursors[j].BUFPOS(bufpos) + begin.position - end.position;
						} else {
							// otherwise, we offset the cursor by the deleted amount
							cursors[j].BUFPOS(bufpos) -= split_point + 1;
						}
					}
				}
			}
		} else {
			// have to delete entire end buffer
			if (end.buffer == max_line_length.buffer) {
				max_line_length.buffer = nullptr;
			}
			lines_deleted += end.buffer->GetLineCount();
			begin.buffer->next = end.buffer->next;
			if (begin.buffer->next) begin.buffer->next->prev = begin.buffer;
			buffers_deleted++;
			buffers.erase(buffers.begin() + buffer_position + 1);
			delete end.buffer;
			// there can still be cursors in the end buffer
			// AFTER the selection but BEFORE the split point
			// we have to move these cursors to the begin buffer
			for (int j = i + 1; j < cursors.size(); j++) {
				if (cursors[j].start_buffer != end.buffer &&
					cursors[j].end_buffer != end.buffer) break;
				for (int bufpos = 0; bufpos < 2; bufpos++) {
					if (cursors[j].BUF(bufpos) == end.buffer) {
						cursors[j].BUF(bufpos) = begin.buffer;
						cursors[j].BUFPOS(bufpos) = begin.position + (cursors[j].BUFPOS(bufpos) - end.position);
					}
				}
			}
		}
	} else {
		// begin buffer = end buffer
		lng deleted_text = end.position - begin.position;
		// in this case, we only need to update cursors in this buffer
		lines_deleted += begin.buffer->DeleteLines(begin.position, end.position);
		begin.buffer->line_count -= lines_deleted;
		for (int j = i + 1; j < cursors.size(); j++) {
			if (cursors[j].start_buffer != begin.buffer &&
				cursors[j].end_buffer != begin.buffer) break;
			for (int bufpos = 0; bufpos < 2; bufpos++) {
				if (cursors[j].BUF(bufpos) == begin.buffer) {
					cursors[j].BUFPOS(bufpos) -= deleted_text;
				}
			}
		}
	}
	// update the current cursor so it only select the start of the selection
	cursor.start_buffer = begin.buffer;
	cursor.start_buffer_position = begin.position;
	cursor.end_buffer_position = cursor.start_buffer_position;
	cursor.end_buffer = cursor.start_buffer;

	// delete linecount from line_lengths
	// recompute line_lengths and cumulative width for begin buffer and end buffer
	InvalidateBuffer(begin.buffer);

	begin.buffer->VerifyBuffer();

	if (buffers_deleted > 0 || lines_deleted > 0) {
		for (lng i = start_index; i < buffers.size(); i++) {
			buffers[i]->index = i;
			buffers[i]->start_line -= lines_deleted;
			buffers[i]->VerifyBuffer();
		}
		linecount -= lines_deleted;
	}
	VerifyPartialTextfile();
}

// insert text into the textfile at each cursors' position
// text cannot contain newlines
void TextFile::InsertText(std::vector<Cursor>& cursors, std::string text) {
	if (!is_loaded) return;

	TextDelta* delta = new PGReplaceText(text);
	PerformOperation(cursors, delta);
}

void TextFile::ReplaceText(std::vector<Cursor>& cursors, std::string replacement_text, size_t i) {
	Cursor& cursor = cursors[i];
	if (replacement_text.size() == 0) {
		if (cursor.SelectionIsEmpty()) {
			// nothing to do; this shouldn't happen (probably)
			assert(0);
			return;
		}
		// nothing to replace; just delete the selection
		DeleteSelection(cursors, i);
		return;
	}
	if (cursor.SelectionIsEmpty()) {
		// nothing to replace; perform a normal insert
		InsertLines(cursors, replacement_text, i);
		return;
	}
	// first replace the text that can be replaced in-line
	lng current_position = 0;
	auto beginpos = cursor.BeginCursorPosition();
	auto curpos = beginpos;
	auto endpos = cursor.EndCursorPosition();
	lng inserted_lines = 0;
	while (curpos < endpos) {
		if (current_position == replacement_text.size()) {
			break;
		}
		bool replace_newline = curpos.buffer->buffer[curpos.position] == '\n';
		bool place_newline = replacement_text[current_position] == '\n';
		if (replace_newline && !place_newline) {
			if (curpos.position == curpos.buffer->current_size - 1) {
				// replacing the last newline of a buffer is tricky because
				// we have to move data between buffers
				// we can let DeleteSelection and InsertLines handle that complexity
				break;
			}
			lng start_line = curpos.buffer->GetStartLine(curpos.position);
			assert(curpos.buffer->line_start[start_line] == curpos.position + 1);
			curpos.buffer->line_start.erase(curpos.buffer->line_start.begin() + start_line);
			curpos.buffer->line_count--;
			inserted_lines--;
		} else if (place_newline && !replace_newline) {
			lng start_line = curpos.buffer->GetStartLine(curpos.position);
			curpos.buffer->line_count++;
			curpos.buffer->line_start.insert(curpos.buffer->line_start.begin() + start_line, curpos.position + 1);
			inserted_lines++;
		}
		curpos.buffer->buffer[curpos.position] = replacement_text[current_position];
		current_position++;
		curpos.Offset(1);
	}
	PGTextBuffer* buffer = beginpos.buffer;
	while (buffer) {
		InvalidateBuffer(buffer);
		if (buffer == endpos.buffer) {
			break;
		}
		buffer = buffer->next;
	}
	if (inserted_lines != 0) {
		buffer = beginpos.buffer;
		lng lines = buffer->start_line;
		while (buffer) {
			buffer->start_line = lines;
			lines += buffer->line_count;
			buffer = buffer->next;
		}
		linecount += inserted_lines;
	}

	// move the cursor to the end and select the remaining text (if any)
	cursor.start_buffer = curpos.buffer;
	cursor.start_buffer_position = curpos.position;
	cursor.end_buffer = endpos.buffer;
	cursor.end_buffer_position = endpos.position;
	// now we can be in one of three situations:
	// 1) the deleted text and selection were identical, as such we are done
	// 2) we replaced the entire selection, but there is still more text to be inserted
	// 3) we replaced part of the selection, but there is still more text to be deleted
	// 4) we replaced part of the selection AND there is still more text to be inserted
	//     this case happens if we try to replace the final newline in a buffer
	if (current_position < replacement_text.size()) {
		if (curpos == endpos) {
			// case (2) haven't finished inserting everything
			// insert the remaining text
			InsertLines(cursors, replacement_text.substr(current_position), i);
		} else {
			// case (4), need to both delete and insert
			DeleteSelection(cursors, i);
			InsertLines(cursors, replacement_text.substr(current_position), i);
		}
	} else if (curpos != endpos) {
		// case (3) haven't finished deleting everything
		// delete the remaining text
		DeleteSelection(cursors, i);
	} else {
		// case (1), finished everything
		return;
	}
}

void TextFile::InsertLines(std::vector<Cursor>& cursors, std::string text, size_t i) {
	Cursor& cursor = cursors[i];
	assert(cursor.SelectionIsEmpty());
	assert(text.size() > 0);
#ifdef PANTHER_DEBUG
	assert(std::find(text.begin(), text.end(), '\r') == text.end());
#endif
	auto lines = SplitLines(text);
	lng added_lines = lines.size() - 1;
	// the first line gets added to the current line we are on
	if (lines[0].size() > 0) {
		InsertText(cursors, lines[0], i);
	}
	if (lines.size() == 1) {
		return;
	}

	auto begin_position = cursor.BeginPosition();

	lng start_position = cursor.start_buffer_position;
	PGTextBuffer* start_buffer = cursor.start_buffer;

	lng position = cursor.start_buffer_position;
	PGTextBuffer* buffer = cursor.start_buffer;

	start_buffer->cumulative_width = -1;

	lng cursor_offset = 0;
	lng final_line_size = -1;
	lng line_position = 0;
	PGTextBuffer* extra_buffer = nullptr;

	buffer->parsed = false;
	bool inserted_buffers = false;

	if (position < buffer->current_size - 1) {
		// there is some text in the buffer that we have to move
		// we add the text to the to-be-inserted lines
		// the remainder of the current line gets appended to the last line
		// the other lines get added to a new buffer we create
		std::string& current_line = lines.back();
		final_line_size = current_line.size();

		lng start_line = buffer->GetStartLine(position);
		line_position = (start_line == buffer->line_start.size() ? buffer->current_size : buffer->line_start[start_line]);
		cursor_offset = line_position - (position + 1);
		current_line += std::string(buffer->buffer + position, line_position - position - 1);
		if (start_line != buffer->line_start.size()) {
			// this is not the last line in the buffer
			// add the remaining lines to "extra_buffer"
			extra_buffer = new PGTextBuffer(buffer->buffer + line_position, buffer->current_size - line_position, -1);
			extra_buffer->line_count = buffer->line_count - (start_line + 1);
			for (lng i = start_line + 1; i < buffer->line_start.size(); i++) {
				extra_buffer->line_start.push_back(buffer->line_start[i] - line_position);
			}
			extra_buffer->cumulative_width = -1;
			extra_buffer->VerifyBuffer();
			buffer->line_count -= extra_buffer->line_count;
			buffer->line_start.erase(buffer->line_start.begin() + buffer->line_count - 1, buffer->line_start.end());
		}
		buffer->buffer[position] = '\n';
		buffer->current_size = position + 1;
	}

	buffer->VerifyBuffer();
	position = buffer->current_size;

	lng buffer_position = PGTextBuffer::GetBuffer(buffers, buffer) + 1;
	for (auto it = lines.begin() + 1; it != lines.end(); it++) {
		if ((*it).size() + 1 >= buffer->buffer_size - buffer->current_size) {
			// line does not fit within the current buffer: have to make a new buffer
			PGTextBuffer* new_buffer = new PGTextBuffer((*it).c_str(), (*it).size(), -1);
			new_buffer->next = buffer->next;
			if (new_buffer->next) new_buffer->next->prev = new_buffer;
			new_buffer->prev = buffer;
			new_buffer->cumulative_width = -1;
			new_buffer->line_count = 1;
			new_buffer->start_line = buffer->start_line + buffer->line_count;
			buffer->next = new_buffer;
			buffer = new_buffer;
			buffer->buffer[buffer->current_size++] = '\n';
			position = buffer->current_size;
			buffers.insert(buffers.begin() + buffer_position, new_buffer);
			inserted_buffers = true;
			buffer_position++;
		} else {
			lng current_line = buffer->current_size;
			// line fits within the current buffer
			if ((*it).size() > 0) {
				buffer->InsertText(position, (*it));
			}
			buffer->buffer[buffer->current_size++] = '\n';
			buffer->line_start.push_back(current_line);
			buffer->line_count++;
			position = buffer->current_size;
		}
	}
	cursor.start_buffer = buffer;
	cursor.start_buffer_position = buffer->current_size - cursor_offset - 1;
	cursor.end_buffer = cursor.start_buffer;
	cursor.end_buffer_position = cursor.start_buffer_position;
	if (extra_buffer) {
		// insert the extra buffer, if we have it
		extra_buffer->next = buffer->next;
		if (extra_buffer->next) extra_buffer->next->prev = extra_buffer;
		buffer->next = extra_buffer;
		extra_buffer->prev = buffer;
		extra_buffer->cumulative_width = -1;
		extra_buffer->start_line = buffer->start_line + buffer->line_count;
		extra_buffer->parsed = false;
		buffers.insert(buffers.begin() + buffer_position, extra_buffer);
		inserted_buffers = true;
	}
	for (size_t j = i + 1; j < cursors.size(); j++) {
		if (cursors[j].start_buffer != start_buffer &&
			cursors[j].end_buffer != start_buffer) break;
		for (int bufpos = 0; bufpos < 2; bufpos++) {
			if (cursors[j].BUFPOS(bufpos) >= start_position) {
				if (cursors[j].BUFPOS(bufpos) < line_position) {
					// the is on the same line as our initial cursor
					// it is now part of the final buffer we inserted
					cursors[j].BUF(bufpos) = buffer;
					cursors[j].BUFPOS(bufpos) = buffer->current_size - (line_position - cursors[j].BUFPOS(bufpos));
				} else {
					// the cursor occurs in the same initial buffer
					// but not on the same line
					// thus the cursor now points to the extra_buffer
					cursors[j].BUF(bufpos) = extra_buffer;
					cursors[j].BUFPOS(bufpos) = cursors[j].BUFPOS(bufpos) - line_position;
				}
			}
		}
	}

	buffer->VerifyBuffer();
	if (extra_buffer) {
		buffer = extra_buffer;
		extra_buffer->VerifyBuffer();
	}

	buffer = buffer->next;
	while (buffer) {
		buffer->start_line += added_lines;
		buffer = buffer->next;
	}
	linecount += added_lines;

	if (inserted_buffers) {
		for (lng i = start_buffer->index; i < buffers.size(); i++) {
			buffers[i]->index = i;
		}
	}

	InvalidateBuffer(start_buffer);
	VerifyPartialTextfile();
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


struct Interval {
	lng start_line;
	lng end_line;
	std::vector<Cursor*> cursors;
	Interval(lng start, lng end, Cursor* cursor) : start_line(start), end_line(end) { cursors.push_back(cursor); }
};

std::vector<Interval> TextFile::GetCursorIntervals(std::vector<Cursor>& cursors) {
	assert(is_loaded);
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

void TextFile::MoveLines(std::vector<Cursor>& cursors, int offset) {
	if (!is_loaded) return;
	assert(0);
}

void TextFile::DeleteLines(std::vector<Cursor>& cursors) {
	if (!is_loaded) return;
	assert(0);
}

void TextFile::IndentText(std::vector<Cursor>& cursors, PGDirection direction) {
	bool contains_selection = Cursor::CursorsContainSelection(cursors);
	std::string added_indentation = "\t";
	if (indentation == PGIndentionSpaces) {
		added_indentation = std::string(this->tabwidth, ' ');
	}
	if (!contains_selection) {
		InsertText(cursors, added_indentation);
	} else {
		auto intervals = GetCursorIntervals(cursors);

		if (direction == PGDirectionRight) {
			AddTextPosition* add = new AddTextPosition();
			for (auto it = intervals.begin(); it != intervals.end(); it++) {
				for (lng line = it->start_line; line <= it->end_line; line++) {
					add->data.push_back(AddTextPosition::AddTextPositionData(added_indentation, line, 0));
				}
			}
			if (add->data.size() == 0) {
				delete add;
				return;
			}
			this->PerformOperation(cursors, add);
		} else {
			RemoveTextPosition* remove = new RemoveTextPosition();
			for (auto it = intervals.begin(); it != intervals.end(); it++) {
				for (lng line = it->start_line; line <= it->end_line; line++) {
					TextLine tl = GetLine(line);
					int start = 0;
					int end = 0;
					lng length = tl.GetLength();
					if (length > 0) {
						char* text = tl.GetLine();
						if (text[0] == '\t') {
							end = 1;
						} else {
							for (int i = 0; i < std::min(length, (lng) this->tabwidth); i++) {
								if (text[i] == ' ') {
									end = i + 1;
								} else {
									break;
								}
							}
						}
						if (start != end) {
							remove->data.push_back(PGCursorRange(line, 0, line, end));
						}
					}
				}
			}
			if (remove->data.size() == 0) {
				delete remove;
				return;
			}
			this->PerformOperation(cursors, remove);
		}
	}
}

void TextFile::ConvertToIndentation(PGLineIndentation indentation) {
	this->indentation = indentation;
	std::vector<Cursor> cursors;
	ReplaceTextPosition* replace = new ReplaceTextPosition();
	for (auto iterator = TextLineIterator(this, (lng)0);; iterator++) {
		TextLine line = iterator.GetLine();
		if (!line.IsValid()) break;
		char* data = line.GetLine();
		lng length = line.GetLength();
		int spaces = 0;

		int start = 0;
		int end = length;
		std::string inserted_text = "";

		bool found_replacement = false;
		for (lng i = 0; i < length; i++) {
			if (data[i] == ' ') {
				if (indentation == PGIndentionTabs) {
					spaces++;
					if (spaces == this->tabwidth) {
						inserted_text += "\t";
						spaces = 0;
						found_replacement = true;
					}
				} else {
					inserted_text += " ";
				}
			} else if (data[i] == '\t') {
				if (indentation == PGIndentionSpaces) {
					found_replacement = true;
					inserted_text += std::string(this->tabwidth, ' ');
				} else {
					inserted_text += "\t";
				}
			} else {
				end = i;
				break;
			}
		}
		if (found_replacement) {
			lng linenumber = iterator.GetCurrentLineNumber();
			cursors.push_back(Cursor(nullptr, PGCursorRange(linenumber, start, linenumber, end)));
			replace->replacement_text.push_back(inserted_text);
		}
	}
	if (replace->data.size() == 0) {
		delete replace;
		return;
	}
	PerformOperation(cursors, replace);
}

void TextFile::DeleteLine(std::vector<Cursor>& cursors, PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new RemoveSelection(direction, PGDeltaRemoveLine);
	PerformOperation(cursors, delta);
}

void TextFile::AddEmptyLine(std::vector<Cursor>& cursors, PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new InsertLineBefore(direction);
	PerformOperation(cursors, delta);
}

void TextFile::InsertText(std::vector<Cursor>& cursors, std::string text, PGTextBuffer* buffer, lng insert_point) {
	// invalidate parsing of the current buffer
	buffer->parsed = false;
	// insert the actual text
	PGBufferUpdate update = PGTextBuffer::InsertText(buffers, buffer, insert_point, text);

	// now we need to update the cursors
	// we only need to consider cursors that currently point to "buffer"
	// first find the first cursor that points to buffer (if any) 
	lng cursor_position = Cursor::FindFirstCursorInBuffer(cursors, buffer);
	if (cursor_position < cursors.size()) {
		for (lng i = cursor_position; i < cursors.size(); i++) {
			Cursor& c = cursors[i];
			if (c.start_buffer != buffer && c.end_buffer != buffer) break;

			for (int bufpos = 0; bufpos < 2; bufpos++) {
				if (c.BUF(bufpos) == buffer) {
					if (update.new_buffer == nullptr) {
						if (c.BUFPOS(bufpos) >= insert_point) {
							c.BUFPOS(bufpos) += update.split_point;
						}
					} else {
						if (c.BUFPOS(bufpos) >= update.split_point) {
							// cursor moves to new buffer
							c.BUF(bufpos) = update.new_buffer;
							c.BUFPOS(bufpos) -= update.split_point;
							if (insert_point >= update.split_point) {
								c.BUFPOS(bufpos) += text.size();
							}
						} else if (c.BUFPOS(bufpos) >= insert_point) {
							c.BUFPOS(bufpos) += text.size();
						}
					}
				}
			}
			assert(c.start_buffer_position < c.start_buffer->current_size);
			assert(c.end_buffer_position < c.end_buffer->current_size);
		}
	}
	if (update.new_buffer != nullptr) {
		for (lng index = buffer->index + 1; index < buffers.size(); index++) {
			buffers[index]->index = index;
		}
	}
	InvalidateBuffer(buffer);
	buffer->VerifyBuffer();
	VerifyPartialTextfile();
}

void TextFile::DeleteText(std::vector<Cursor>& cursors, PGTextRange range) {
	auto begin = range.startpos();
	auto end = range.endpos();

	begin.buffer->parsed = false;
	end.buffer->parsed = false;

	PGTextBuffer* buffer = begin.buffer;
	lng lines_deleted = 0;
	lng buffers_deleted = 0;
	lng delete_size;
	lng start_index = begin.buffer->index + 1;

	lng split_point = -1;

	std::vector<std::unique_ptr<PGTextBuffer>> deleted_buffers;
	if (end.buffer != begin.buffer) {
		lines_deleted += begin.buffer->DeleteLines(begin.position);
		begin.buffer->line_count -= lines_deleted;

		buffer = buffer->next;

		lng buffer_position = PGTextBuffer::GetBuffer(buffers, begin.buffer);
		while (buffer != end.buffer) {
			if (buffer == max_line_length.buffer) {
				max_line_length.buffer = nullptr;
			}
			lines_deleted += buffer->GetLineCount();
			buffers_deleted++;
			buffers.erase(buffers.begin() + buffer_position + 1);
			PGTextBuffer* next = buffer->next;
			deleted_buffers.push_back(std::unique_ptr<PGTextBuffer>(buffer));
			buffer = next;
		}

		// we need to move everything that is AFTER the selection
		// but IN the current line from "endbuffer" to "beginbuffer"
		// look for the first newline character in "endbuffer"
		split_point = end.position;
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
			// first adjust start_line of end buffer based on previously deleted lines
			end.buffer->start_line -= lines_deleted;
			// now delete the lines in end_buffer and update the line_count
			lng deleted_lines_in_end_buffer = buffer->DeleteLines(0, split_point + 1);
			lines_deleted += deleted_lines_in_end_buffer;
			end.buffer->line_count -= deleted_lines_in_end_buffer;
			// set the index, in case there were any deleted buffers
			end.buffer->index = begin.buffer->index + 1;
			begin.buffer->next = end.buffer;
			end.buffer->prev = begin.buffer;
			start_index = end.buffer->index + 1;
			InvalidateBuffer(end.buffer);

			end.buffer->VerifyBuffer();
		} else {
			// have to delete entire end buffer
			if (end.buffer == max_line_length.buffer) {
				max_line_length.buffer = nullptr;
			}
			lines_deleted += end.buffer->GetLineCount();
			begin.buffer->next = end.buffer->next;
			if (begin.buffer->next) begin.buffer->next->prev = begin.buffer;
			buffers_deleted++;
			buffers.erase(buffers.begin() + buffer_position + 1);
			deleted_buffers.push_back(std::unique_ptr<PGTextBuffer>(end.buffer));
		}
	} else {
		// begin buffer = end buffer
		lng deleted_text = end.position - begin.position;
		// in this case, we only need to update cursors in this buffer
		lines_deleted += begin.buffer->DeleteLines(begin.position, end.position);
		begin.buffer->line_count -= lines_deleted;
	}
	// update the cursors
	lng cursor_position = Cursor::FindFirstCursorInBuffer(cursors, begin.buffer);
	for (lng i = cursor_position; i < cursors.size(); i++) {
		Cursor& c = cursors[i];
		if (c.start_buffer->index > end.buffer->index &&
			c.end_buffer->index > end.buffer->index) break;
		for (int bufpos = 0; bufpos < 2; bufpos++) {
			if (c.BUF(bufpos) == begin.buffer &&
				c.BUFPOS(bufpos) < begin.position) {
				// cursor is in begin buffer, but before the deleted text
				// do nothing
			} else if (c.BUF(bufpos) == end.buffer &&
				c.BUFPOS(bufpos) > end.position) {
				// cursor is in the end buffer AFTER the deleted text
				if (split_point < 0) {
					// begin buffer = end buffer
					// simply offset the cursor by the deleted text
					c.BUF(bufpos) = begin.buffer;
					c.BUFPOS(bufpos) -= end.position - begin.position;
				} else {
					c.BUF(bufpos) = begin.buffer;
					c.BUFPOS(bufpos) -= end.position - begin.position;
				}
			} else if (c.BUF(bufpos)->index >= begin.buffer->index && c.BUF(bufpos)->index <= end.buffer->index) {
				// the cursor falls within the deleted text, move to delete position
				c.BUF(bufpos) = begin.buffer;
				c.BUFPOS(bufpos) = begin.position;
			}
		}

	}
	// delete linecount from line_lengths
	// recompute line_lengths and cumulative width for begin buffer and end buffer
	InvalidateBuffer(begin.buffer);

	begin.buffer->VerifyBuffer();

	if (buffers_deleted > 0 || lines_deleted > 0) {
		for (lng i = start_index; i < buffers.size(); i++) {
			buffers[i]->index = i;
			buffers[i]->start_line -= lines_deleted;
			buffers[i]->VerifyBuffer();
		}
		linecount -= lines_deleted;
	}
	VerifyPartialTextfile();
	// FIXME
}

void TextFile::ReplaceText(std::vector<Cursor>& cursors, PGTextRange range, std::string replacement_text) {
	bool empty_range = range.startpos() == range.endpos();
	if (replacement_text.size() == 0) {
		if (empty_range) {
			// nothing to do; this shouldn't happen (probably)
			assert(0);
			return;
		}
		// nothing to replace; just delete the selection
		DeleteText(cursors, range);
		return;
	}
	if (empty_range) {
		// nothing to replace; perform a normal insert
		InsertText(cursors, replacement_text, range.start_buffer, range.start_position);
		return;
	}
	// first replace the text that can be replaced in-line
	lng current_position = 0;
	auto beginpos = range.startpos();
	auto curpos = beginpos;
	auto endpos = range.endpos();
	lng inserted_lines = 0;
	while (curpos < endpos) {
		if (current_position == replacement_text.size()) {
			break;
		}
		assert(curpos.buffer->buffer[curpos.position] != '\n');
		assert(replacement_text[current_position] != '\n');
		curpos.buffer->buffer[curpos.position] = replacement_text[current_position];
		current_position++;
		curpos.Offset(1);
	}
	PGTextBuffer* buffer = beginpos.buffer;
	while (buffer) {
		InvalidateBuffer(buffer);
		if (buffer == endpos.buffer) {
			break;
		}
		buffer = buffer->next;
	}
	if (inserted_lines != 0) {
		buffer = beginpos.buffer;
		lng lines = buffer->start_line;
		while (buffer) {
			buffer->start_line = lines;
			lines += buffer->line_count;
			buffer = buffer->next;
		}
		linecount += inserted_lines;
	}

	// move the cursor to the end and select the remaining text (if any)
	range.start_buffer = curpos.buffer;
	range.start_position = curpos.position;
	range.end_buffer = endpos.buffer;
	range.end_position = endpos.position;
	// now we can be in one of three situations:
	// 1) the deleted text and selection were of identical size, as such we are done
	// 2) we replaced the entire selection, but there is still more text to be inserted
	// 3) we replaced part of the selection, but there is still more text to be deleted
	if (current_position < replacement_text.size()) {
		assert(curpos == endpos);
		// case (2) haven't finished inserting everything
		// insert the remaining text
		InsertText(cursors, replacement_text.substr(current_position), curpos.buffer, curpos.position);
	} else if (curpos != endpos) {
		// case (3) haven't finished deleting everything
		// delete the remaining text
		DeleteText(cursors, range);
	} else {
		// case (1), finished everything
		return;
	}
}

std::string TextFile::CutText(std::vector<Cursor>& cursors) {
	std::string text = CopyText(cursors);
	if (!Cursor::CursorsContainSelection(cursors)) {
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			it->SelectLine();
		}
		Cursor::NormalizeCursors(cursors);
	}
	this->DeleteCharacter(cursors, PGDirectionLeft);
	return text;
}

std::string TextFile::CopyText(std::vector<Cursor>& cursors) {
	std::string text = "";
	if (!is_loaded) return text;
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

void TextFile::PasteText(std::vector<Cursor>& cursors, std::string& text) {
	if (!is_loaded) return;

	TextDelta* delta = new PGReplaceText(text);
	PerformOperation(cursors, delta);
}

void TextFile::RegexReplace(std::vector<Cursor>& cursors, PGRegexHandle regex, std::string& replacement) {
	if (!is_loaded) return;

	TextDelta* delta = PGRegexReplace::CreateRegexReplace(replacement, regex);
	PerformOperation(cursors, delta);
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
			assert(buffers[i]->next == buffers[i + 1]);
			// only the final buffer can end with a non-newline character
			assert(buffers[i]->buffer[buffers[i]->current_size - 1] == '\n');
		}
		if (i > 0) {
			assert(buffers[i]->prev == buffers[i - 1]);
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
			assert(buffers[i]->next == buffers[i + 1]);
			// only the final buffer can end with a non-newline character
			assert(buffers[i]->buffer[buffers[i]->current_size - 1] == '\n');
		}
		if (i > 0) {
			assert(buffers[i]->prev == buffers[i - 1]);
		}
		assert(buffers[i]->current_size < buffers[i]->buffer_size);
	}
#endif
}

void TextFile::DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new RemoveSelection(direction, PGDeltaRemoveCharacter);
	PerformOperation(cursors, delta);
}

void TextFile::DeleteWord(std::vector<Cursor>& cursors, PGDirection direction) {
	if (!is_loaded) return;

	TextDelta* delta = new RemoveSelection(direction, PGDeltaRemoveWord);
	PerformOperation(cursors, delta);
}

void TextFile::AddNewLine(std::vector<Cursor>& cursors) {
	if (!is_loaded) return;
	AddNewLine(cursors, "");
}

void TextFile::AddNewLine(std::vector<Cursor>& cursors, std::string text) {
	if (!is_loaded) return;
	auto newline = std::string("\n");
	PasteText(cursors, newline);
}

void TextFile::SetUnsavedChanges(bool changes) {
	// FIXME:
	/*if (changes != unsaved_changes && textfield) {
		RefreshWindow(textfield->GetWindow());
	}*/
	unsaved_changes = changes;
}

void TextFile::Undo(TextView* view) {
	if (!is_loaded) return;
	if (read_only) return;

	if (this->deltas.size() == 0) return;
	TextDelta* delta = this->deltas.back().get();
	Lock(PGWriteLock);
	VerifyTextfile();
	this->Undo(view, delta);
	InvalidateBuffers(view);
	VerifyTextfile();
	Unlock(PGWriteLock);
	this->redos.push_back(RedoStruct(Cursor::BackupCursors(view->cursors)));
	this->redos.back().delta = std::move(this->deltas.back());
	this->deltas.pop_back();
	if (view->textfield) {
		view->textfield->TextChanged();
	}
	SetUnsavedChanges(saved_undo_count != deltas.size());
	InvalidateParsing();
}

void TextFile::Redo(TextView* view) {
	if (!is_loaded) return;
	if (read_only) return;
	if (this->redos.size() == 0) return;
	RedoStruct& redo = this->redos.back();
	TextDelta* delta = redo.delta.get();
	current_task = nullptr;
	// lock the blocks
	Lock(PGWriteLock);
	view->RestoreCursors(redo.cursors);
	VerifyTextfile();
	// perform the operation
	PerformOperation(view->cursors, delta, true);
	// release the locks again
	InvalidateBuffers(view);
	VerifyTextfile();
	Unlock(PGWriteLock);
	this->deltas.push_back(std::move(redo.delta));
	this->redos.pop_back();
	SetUnsavedChanges(saved_undo_count != deltas.size());
	if (view->textfield) {
		view->textfield->TextChanged();
	}
	// invalidate any lines for parsing
	InvalidateParsing();
}

void TextFile::AddDelta(TextDelta* delta) {
	if (!is_loaded) return;
	redos.clear();
	this->deltas.push_back(std::unique_ptr<TextDelta>(delta));
}

void TextFile::PerformOperation(std::vector<Cursor>& cursors, TextDelta* delta) {
	if (read_only) {
		return;
	}
	// set the current_task to the nullptr, this will cause any active syntax highlighting to end
	// this prevents long syntax highlighting tasks (e.g. highlighting a long document) from 
	// locking us out of editing until they're finished
	current_task = nullptr;
	// this should be an assertion
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	// lock the blocks
	Lock(PGWriteLock);
	VerifyTextfile();
	// perform the operation
	bool success = PerformOperation(cursors, delta, false);
	// release the locks again
	InvalidateBuffers(cursors[0].file);
	VerifyTextfile();
	Unlock(PGWriteLock);
	if (!success) return;
	AddDelta(delta);
	if (deltas.size() == saved_undo_count) {
		saved_undo_count = -1;
	}
	SetUnsavedChanges(true);
	/*
	FIXME:
	if (this->textfield) {
		this->textfield->TextChanged();
	}*/
	// invalidate any lines for parsing
	InvalidateParsing();
}

bool TextFile::PerformOperation(std::vector<Cursor>& cursors, TextDelta* delta, bool redo) {
	switch (delta->type) {
		case PGDeltaReplaceText:
		{
			PGReplaceText* replace = (PGReplaceText*)delta;
			for (size_t i = 0; i < cursors.size(); i++) {
				if (!redo) {
					if (!cursors[i].SelectionIsEmpty()) {
						replace->removed_text.push_back(cursors[i].GetText());
					} else {
						replace->removed_text.push_back("");
					}
				}
				ReplaceText(cursors, replace->text, i);
				if (!redo) {
					replace->stored_cursors.push_back(Cursor::BackupCursor(cursors, i));
				}
			}
			break;
		}
		case PGDeltaRegexReplace:
		{
			PGRegexReplace* replace = (PGRegexReplace*)delta;
			for (size_t i = 0; i < cursors.size(); i++) {
				assert(!cursors[i].SelectionIsEmpty());
				if (!redo) {
					replace->removed_text.push_back(cursors[i].GetText());
				}
				std::string replaced_text = cursors[i].GetText();
				PGRegexMatch match = PGMatchRegex(replace->regex, cursors[i].GetCursorSelection(), PGDirectionRight);
				assert(match.matched);
				std::string replacement_text = "";
				for (size_t k = 0; k < replace->groups.size(); k++) {
					replacement_text += replace->groups[k].first;
					if (replace->groups[k].second >= 0) {
						replacement_text += match.groups[replace->groups[k].second].GetString();
					}
				}
				ReplaceText(cursors, replacement_text, i);
				if (!redo) {
					replace->added_text_size.push_back(replacement_text.size());
					replace->stored_cursors.push_back(Cursor::BackupCursor(cursors, i));
				}
			}
			break;
		}
		case PGDeltaRemoveText:
		{
			RemoveText* remove = (RemoveText*)delta;
			assert(Cursor::CursorsContainSelection(cursors));
			for (size_t i = 0; i < cursors.size(); i++) {
				if (!cursors[i].SelectionIsEmpty()) {
					if (!redo) {
						remove->removed_text.push_back(cursors[i].GetText());
						remove->stored_cursors.push_back(Cursor::BackupCursor(cursors, i));
					}
					DeleteSelection(cursors, i);
				} else if (!redo) {
					remove->removed_text.push_back("");
					remove->stored_cursors.push_back(Cursor::BackupCursor(cursors, i));
				}
			}
			break;
		}
		case PGDeltaRemoveLine:
		{
			RemoveSelection* remove = (RemoveSelection*)delta;
			if (!redo) {
				remove->stored_cursors = Cursor::BackupCursors(cursors);
			}
			for (int i = 0; i < cursors.size(); i++) {
				if (remove->direction == PGDirectionLeft) {
					cursors[i].SelectStartOfLine();
				} else {
					cursors[i].SelectEndOfLine();
				}
			}
			if (!Cursor::CursorsContainSelection(cursors)) {
				return false;
			}
			Cursor::NormalizeCursors(cursors);
			if (!redo) {
				remove->next = std::unique_ptr<TextDelta>(new RemoveText());
			}
			return PerformOperation(cursors, remove->next.get(), redo);
		}
		case PGDeltaRemoveWord:
		{
			RemoveSelection* remove = (RemoveSelection*)delta;
			if (!redo) {
				remove->stored_cursors = Cursor::BackupCursors(cursors);
			}
			if (!Cursor::CursorsContainSelection(cursors)) {
				for (auto it = cursors.begin(); it != cursors.end(); it++) {
					it->OffsetSelectionWord(remove->direction);
				}
			}
			if (!Cursor::CursorsContainSelection(cursors)) {
				return false;
			}
			Cursor::NormalizeCursors(cursors);
			if (!redo) {
				remove->next = std::unique_ptr<TextDelta>(new RemoveText());
			}
			return PerformOperation(cursors, remove->next.get(), redo);
		}
		case PGDeltaRemoveCharacter:
		{
			RemoveSelection* remove = (RemoveSelection*)delta;
			if (!redo) {
				remove->stored_cursors = Cursor::BackupCursors(cursors);
			}
			if (!Cursor::CursorsContainSelection(cursors)) {
				for (auto it = cursors.begin(); it != cursors.end(); it++) {
					it->OffsetSelectionCharacter(remove->direction);
				}
			}
			if (!Cursor::CursorsContainSelection(cursors)) {
				return false;
			}
			Cursor::NormalizeCursors(cursors);
			if (!redo) {
				remove->next = std::unique_ptr<TextDelta>(new RemoveText());
			}
			return PerformOperation(cursors, remove->next.get(), redo);
		}
		case PGDeltaAddEmptyLine:
		{
			InsertLineBefore* ins = (InsertLineBefore*)delta;
			if (!redo) {
				ins->stored_cursors = Cursor::BackupCursors(cursors);
			}
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				if (ins->direction == PGDirectionLeft) {
					it->OffsetStartOfLine();
				} else {
					it->OffsetEndOfLine();
				}
			}
			Cursor::NormalizeCursors(cursors);
			if (!redo) {
				ins->next = std::unique_ptr<TextDelta>(new PGReplaceText("\n"));
			}
			return PerformOperation(cursors, ins->next.get(), redo);
		}
		case PGDeltaAddTextPosition:
		{
			AddTextPosition* add = (AddTextPosition*)delta;
			if (!redo) {
				add->stored_cursors = Cursor::BackupCursors(cursors);
			}
			for (lng i = add->data.size() - 1; i >= 0; i--) {
				PGTextBuffer* buffer = GetBuffer(add->data[i].line);
				lng position = buffer->GetBufferLocationFromCursor(add->data[i].line, add->data[i].character);
				InsertText(cursors, add->data[i].text, buffer, position);
			}
			return true;
		}
		case PGDeltaRemoveTextPosition:
		{
			RemoveTextPosition* remove = (RemoveTextPosition*)delta;
			if (!redo) {
				remove->stored_cursors = Cursor::BackupCursors(cursors);
			}
			remove->removed_text.resize(remove->data.size());
			for (lng i = remove->data.size() - 1; i >= 0; i--) {
				PGTextBuffer* start_buffer = GetBuffer(remove->data[i].start_line);
				lng start_position = start_buffer->GetBufferLocationFromCursor(remove->data[i].start_line, remove->data[i].start_position);
				PGTextBuffer* end_buffer = GetBuffer(remove->data[i].end_line);
				lng end_position = end_buffer->GetBufferLocationFromCursor(remove->data[i].end_line, remove->data[i].end_position);
				PGTextRange range = PGTextRange(start_buffer, start_position, end_buffer, end_position);
				if (!redo) {
					Cursor c = Cursor(nullptr, range);
					if (c.SelectionIsEmpty()) {
						remove->removed_text[i] = "";
					} else {
						remove->removed_text[i] = c.GetText();
					}
				}
				DeleteText(cursors, range);
			}
			return true;
		}
		case PGDeltaReplaceTextPosition:
		{
			ReplaceTextPosition* remove = (ReplaceTextPosition*)delta;
			if (!redo) {
				remove->stored_cursors = Cursor::BackupCursors(cursors);
			}
			remove->removed_text.resize(remove->data.size());
			for (lng i = remove->data.size() - 1; i >= 0; i--) {
				PGTextBuffer* start_buffer = GetBuffer(remove->data[i].start_line);
				lng start_position = start_buffer->GetBufferLocationFromCursor(remove->data[i].start_line, remove->data[i].start_position);
				PGTextBuffer* end_buffer = GetBuffer(remove->data[i].end_line);
				lng end_position = end_buffer->GetBufferLocationFromCursor(remove->data[i].end_line, remove->data[i].end_position);
				PGTextRange range = PGTextRange(start_buffer, start_position, end_buffer, end_position);
				if (!redo) {
					Cursor c = Cursor(nullptr, range);
					if (c.SelectionIsEmpty()) {
						remove->removed_text[i] = "";
					} else {
						remove->removed_text[i] = c.GetText();
					}
				}
				ReplaceText(cursors, range, remove->replacement_text[i]);
			}
			return true;
		}
		default:
			assert(0);
			return false;
	}
	return true;
}

void TextFile::Undo(std::vector<Cursor>& cursors, PGReplaceText& delta, size_t i) {
	lng offset = delta.text.size();
	cursors[i].OffsetSelectionPosition(-offset);
	auto beginpos = cursors[i].BeginCursorPosition();
	auto endpos = beginpos;
	ReplaceText(cursors, delta.removed_text[i], i);
	// select the replaced text
	endpos.Offset(delta.removed_text[i].size());
	cursors[i].start_buffer = endpos.buffer;
	cursors[i].start_buffer_position = endpos.position;
	cursors[i].end_buffer = beginpos.buffer;
	cursors[i].end_buffer_position = beginpos.position;
}


void TextFile::Undo(std::vector<Cursor>& cursors, PGRegexReplace& delta, size_t i) {
	lng offset = delta.added_text_size[i];
	cursors[i].OffsetSelectionPosition(-offset);
	auto beginpos = cursors[i].BeginCursorPosition();
	auto endpos = beginpos;
	ReplaceText(cursors, delta.removed_text[i], i);
	// select the replaced text
	endpos.Offset(delta.removed_text[i].size());
	cursors[i].start_buffer = endpos.buffer;
	cursors[i].start_buffer_position = endpos.position;
	cursors[i].end_buffer = beginpos.buffer;
	cursors[i].end_buffer_position = beginpos.position;
}

void TextFile::Undo(std::vector<Cursor>& cursors, RemoveText& delta, std::string& text, size_t i) {
	if (text.size() > 0) {
		InsertLines(cursors, text, i);
	}
}

void TextFile::Undo(TextView* view, TextDelta* delta) {
	switch (delta->type) {
		case PGDeltaReplaceText:
		{
			LockMutex(view->lock.get());
			view->ClearCursors();
			PGReplaceText* replace = (PGReplaceText*)delta;
			// we perform undo's in reverse order
			view->cursors.resize(delta->stored_cursors.size());
			for (int i = 0; i < delta->stored_cursors.size(); i++) {
				int index = delta->stored_cursors.size() - (i + 1);
				view->cursors[index] = view->RestoreCursor(delta->stored_cursors[index]);
				Undo(view->cursors, *replace, index);
			}
			UnlockMutex(view->lock.get());
			break;
		}
		case PGDeltaRegexReplace:
		{
			LockMutex(view->lock.get());
			view->ClearCursors();
			PGRegexReplace* replace = (PGRegexReplace*)delta;
			// we perform undo's in reverse order
			view->cursors.resize(delta->stored_cursors.size());
			for (int i = 0; i < delta->stored_cursors.size(); i++) {
				int index = delta->stored_cursors.size() - (i + 1);
				view->cursors[index] = view->RestoreCursor(delta->stored_cursors[index]);
				Undo(view->cursors, *replace, index);
			}
			UnlockMutex(view->lock.get());
			break;
		}
		case PGDeltaRemoveText:
		{
			RemoveText* remove = (RemoveText*)delta;
			LockMutex(view->lock.get());
			view->ClearCursors();
			view->cursors.resize(delta->stored_cursors.size());
			for (int i = 0; i < delta->stored_cursors.size(); i++) {
				int index = delta->stored_cursors.size() - (i + 1);
				view->cursors[index] = view->RestoreCursorPartial(delta->stored_cursors[index]);
				Undo(view->cursors, *remove, remove->removed_text[index], index);
				view->cursors[index] = view->RestoreCursor(delta->stored_cursors[index]);
			}
			UnlockMutex(view->lock.get());
			break;
		}
		case PGDeltaRemoveLine:
		case PGDeltaRemoveCharacter:
		case PGDeltaRemoveWord:
		{
			RemoveSelection* remove = (RemoveSelection*)delta;
			assert(remove->next);
			Undo(view, remove->next.get());
			view->RestoreCursors(remove->stored_cursors);
			return;
		}
		case PGDeltaAddEmptyLine:
		{
			InsertLineBefore* ins = (InsertLineBefore*)delta;
			assert(ins->next);
			Undo(view, ins->next.get());
			view->RestoreCursors(ins->stored_cursors);
			return;
		}
		case PGDeltaAddTextPosition:
		{
			AddTextPosition* add = (AddTextPosition*)delta;
			LockMutex(view->lock.get());
			for (lng i = add->data.size() - 1; i >= 0; i--) {
				PGTextBuffer* start_buffer = GetBuffer(add->data[i].line);
				lng start_position = start_buffer->GetBufferLocationFromCursor(add->data[i].line, add->data[i].character);
				DeleteText(view->cursors, PGTextRange(start_buffer, start_position, start_buffer, start_position + add->data[i].text.size()));
			}
			view->ActuallyRestoreCursors(add->stored_cursors);
			UnlockMutex(view->lock.get());
			return;
		}
		case PGDeltaRemoveTextPosition:
		{
			RemoveTextPosition* remove = (RemoveTextPosition*)delta;
			LockMutex(view->lock.get());
			for (lng i = remove->data.size() - 1; i >= 0; i--) {
				PGTextBuffer* start_buffer = GetBuffer(remove->data[i].start_line);
				lng start_position = start_buffer->GetBufferLocationFromCursor(remove->data[i].start_line, remove->data[i].start_position);
				InsertText(view->cursors, remove->removed_text[i], start_buffer, start_position);
			}
			view->ActuallyRestoreCursors(remove->stored_cursors);
			UnlockMutex(view->lock.get());
			return;
		}
		case PGDeltaReplaceTextPosition:
		{
			ReplaceTextPosition* remove = (ReplaceTextPosition*)delta;
			LockMutex(view->lock.get());
			for (lng i = remove->data.size() - 1; i >= 0; i--) {
				PGTextBuffer* start_buffer = GetBuffer(remove->data[i].start_line);
				lng start_position = start_buffer->GetBufferLocationFromCursor(remove->data[i].start_line, remove->data[i].start_position);
				PGTextBuffer* end_buffer = start_buffer;
				lng end_position = start_position + remove->replacement_text[i].size();
				PGTextRange range = PGTextRange(start_buffer, start_position, end_buffer, end_position);
				ReplaceText(view->cursors, range, remove->removed_text[i]);
			}
			view->ActuallyRestoreCursors(remove->stored_cursors);
			UnlockMutex(view->lock.get());
			return;
		}
		default:
			assert(0);
			break;
	}
	if (delta->next) {
		Undo(view, delta->next.get());
	}
}

bool TextFile::WriteToFile(PGFileHandle file, PGEncoderHandle encoder, const char* text, lng size, char** output_text, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {
	if (size <= 0) return true;
	if (encoding != PGEncodingUTF8) {
		// first convert the text
		lng result_size = PGConvertText(encoder, text, size, output_text, output_size, intermediate_buffer, intermediate_size);
		if (result_size < 0) {
			return false;
		}
		text = *output_text;
		size = result_size;
	}
	panther::WriteToFile(file, text, size);
	return true;
}

void TextFile::SaveChanges() {
	if (!is_loaded) return;
	if (this->FileInMemory()) return;

	saved_undo_count = deltas.size();
	SetUnsavedChanges(false);
	this->Lock(PGReadLock);
	PGLineEnding line_ending = lineending;
	if (line_ending != PGLineEndingWindows && line_ending != PGLineEndingMacOS && line_ending != PGLineEndingUnix) {
		line_ending = GetSystemLineEnding();
	}

	PGFileError error;
	PGFileHandle handle = panther::OpenFile(this->path, PGFileReadWrite, error);
	if (!handle) {
		this->Unlock(PGReadLock);
		return;
	}

	if (encoding == PGEncodingUTF8BOM) {
		// first write the BOM
		unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
		panther::WriteToFile(handle, (const char*)bom, 3);
	}
	PGEncoderHandle encoder = nullptr;
	if (encoding != PGEncodingUTF8) {
		encoder = PGCreateEncoder(PGEncodingUTF8, encoding);
	}
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;
	char* output_buffer = nullptr;
	lng output_size = 0;

	lng position = 0;
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		char* line = (*it)->buffer;
		lng prev_position = 0;
		lng i = 0;
		lng end = (*it) == buffers.back() ? (*it)->current_size - 1 : (*it)->current_size;
		for (i = 0; i < end; i++) {
			if (line[i] == '\n') {
				// new line
				this->WriteToFile(handle, encoder, line + prev_position, i - prev_position, &output_buffer, &output_size, &intermediate_buffer, &intermediate_size);
				switch (line_ending) {
					case PGLineEndingWindows:
						this->WriteToFile(handle, encoder, "\r\n", 2, &output_buffer, &output_size, &intermediate_buffer, &intermediate_size);
						break;
					case PGLineEndingMacOS:
						this->WriteToFile(handle, encoder, "\r", 1, &output_buffer, &output_size, &intermediate_buffer, &intermediate_size);
						break;
					case PGLineEndingUnix:
						this->WriteToFile(handle, encoder, "\n", 1, &output_buffer, &output_size, &intermediate_buffer, &intermediate_size);
						break;
					default:
						assert(0);
						break;
				}
				prev_position = i + 1;
			}
		}
		if (prev_position < (*it)->current_size) {
			assert((*it) == buffers.back());
			this->WriteToFile(handle, encoder, line + prev_position, i - prev_position, &output_buffer, &output_size, &intermediate_buffer, &intermediate_size);
		}
	}
	if (intermediate_buffer) {
		free(intermediate_buffer);
		intermediate_buffer = nullptr;
	}
	if (output_buffer) {
		free(output_buffer);
		output_buffer = nullptr;
	}
	if (encoder) {
		PGDestroyEncoder(encoder);
	}
	this->Unlock(PGReadLock);
	panther::CloseFile(handle);
	UpdateModificationTime();
	// FIXME:
	//if (textfield) textfield->SelectionChanged();
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

std::string TextFile::GetText() {
	std::string text = "";
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		text += std::string((*it)->buffer, (*it)->current_size - 1);
		if (*it != buffers.back()) {
			text += "\n";
		}
	}
	return text;
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

	std::vector<Cursor> cursors;
	RemoveTextPosition* remove = new RemoveTextPosition();
	for (auto iterator = TextLineIterator(this, (lng)0);; iterator++) {
		TextLine line = iterator.GetLine();
		if (!line.IsValid()) break;
		char* data = line.GetLine();
		lng length = line.GetLength();
		int spaces = 0;

		int start = length;
		int end = length;

		bool found_replacement = false;
		for (lng i = length - 1; i >= 0; i--) {
			if (data[i] == ' ' || data[i] == '\t') {
				start = i;
			} else {
				break;
			}
		}
		if (start != end) {
			lng linenumber = iterator.GetCurrentLineNumber();
			cursors.push_back(Cursor(nullptr, PGCursorRange(linenumber, start, linenumber, end)));
		}
	}
	if (remove->data.size() == 0) {
		delete remove;
		return;
	}
	PerformOperation(cursors, remove);
}

PGTextRange TextFile::FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap, std::shared_ptr<Task> current_task) {
	PGTextBuffer* start_buffer = buffers[PGTextBuffer::GetBuffer(buffers, start_line)];
	PGTextBuffer* end_buffer = buffers[PGTextBuffer::GetBuffer(buffers, end_line)];
	lng start_position = start_buffer->GetBufferLocationFromCursor(start_line, start_character);
	lng end_position = end_buffer->GetBufferLocationFromCursor(end_line, end_character);
	return FindMatch(regex_handle, direction, start_buffer, start_position, end_buffer, end_position, wrap, current_task);
}

PGTextRange TextFile::FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap, std::shared_ptr<Task> current_task) {
	// we start "outside" of the current selection
	// e.g. if we go right, we start at the end and continue right
	// if we go left, we start at the beginning and go left
	PGTextBuffer* begin_buffer = direction == PGDirectionLeft ? start_buffer : end_buffer;
	lng begin_position = direction == PGDirectionLeft ? start_position : end_position;
	int offset = direction == PGDirectionLeft ? -1 : 1;
	PGTextBuffer* end = direction == PGDirectionLeft ? buffers.front() : buffers.back();
	PGTextRange bounds = direction == PGDirectionLeft ?
		PGTextRange(end, 0, begin_buffer, begin_position) :
		PGTextRange(begin_buffer, begin_position, buffers.back(), end->current_size - 1);
	while (true) {
		PGRegexMatch match = PGMatchRegex(regex_handle, bounds, direction);
		if (match.matched) {
			return match.groups[0];
		}
		// no match was found
		if (wrap) {
			// if wrap is enabled, search the entire buffer this time
			// the reason we search everything now is because the regex can match anything
			// (e.g. the entire block of text), in which we need to supply all the text
			// otherwise a match might not be found while one exists
			bounds = PGTextRange(buffers.front(), 0, buffers.back(), buffers.back()->current_size);
			wrap = false;
		} else {
			break;
		}
	}
	return PGTextRange();
}

#define TEXTFILE_BUFFER_THRESHOLD 1000000
TextFile::PGStoreFileType TextFile::WorkspaceFileStorage() {
	lng buffer_size = 0;
	for (auto it = buffers.begin(); it != buffers.end(); it++) {
		buffer_size += (*it)->current_size;
	}

	if (buffer_size < TEXTFILE_BUFFER_THRESHOLD) {
		// the entire buffer fits within the threshold we have set
		return PGStoreFileBuffer;
	}

	if (path.size() == 0) {
		// this file has no file associated with it
		// thus we have to save the buffer
		// but the buffer is too big
		return PGFileTooLarge;
	}
	return PGFileTooLarge;

	// FIXME: writing deltas not supported yet

	// if we have a file associated with it we can just save the deltas
	// check the size of the deltas
	size_t delta_size = 0;
	for (auto it = deltas.begin(); it != deltas.end(); it++) {
		delta_size += (*it)->SerializedSize();
	}

	if (delta_size < TEXTFILE_BUFFER_THRESHOLD) {
		// deltas fit within the threshold, store the deltas
		return PGStoreFileDeltas;
	}
	// both deltas and buffer are too big; can't store file
	return PGFileTooLarge;
}

void TextFile::SetSettings(PGTextFileSettings settings) {
	this->settings = settings;
	if (is_loaded) {
		Lock(PGWriteLock);
		ApplySettings(settings);
		Unlock(PGWriteLock);
	}
}

void TextFile::ApplySettings(PGTextFileSettings& settings) {
	if (settings.line_ending != PGLineEndingUnknown) {
		this->lineending = settings.line_ending;
		settings.line_ending = PGLineEndingUnknown;
	}
	if (settings.language && settings.language != this->language) {
		this->language = settings.language;
		this->highlighter = this->language ? std::unique_ptr<SyntaxHighlighter>(this->language->CreateHighlighter()) : nullptr;
		if (this->is_loaded) {
			for (auto it = buffers.begin(); it != buffers.end(); it++) {
				(*it)->parsed = false;
			}
			this->InvalidateParsing();
		}
	}
	if (settings.encoding != PGEncodingUnknown) {
		this->encoding = settings.encoding;
	}
	if (settings.name.size() > 0) {
		this->name = settings.name;
	}
}

void TextFile::AddFindMatches(std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng start_line) {
	std::string text;
	lng current_line = this->linecount;
	if (current_find_file != filename) {
		text = "\nFile: " + filename + "\n";
		current_find_file = filename;
		current_line++;
	} else {
		text = std::string(std::to_string(start_line).size(), '.') + "\n";
	}
	std::vector<bool> line_is_match(lines.size(), false);
	for (auto it = matches.begin(); it != matches.end(); it++) {
		assert(it->start_line - start_line >= 0 && it->start_line - start_line < line_is_match.size());
		line_is_match[it->start_line - start_line] = true;
	}
	lng linecount = 0;
	for (auto it = lines.begin(); it != lines.end(); it++) {
		bool is_match_line = line_is_match[linecount];
		text += std::to_string(start_line + linecount) + (is_match_line ? "> " : ": ") + *it + "\n";
		linecount++;
	}
	this->Lock(PGWriteLock);
	std::vector<Cursor> cursors;
	cursors.push_back(Cursor(nullptr, PGTextRange(buffers.back(), buffers.back()->current_size - 1, buffers.back(), buffers.back()->current_size - 1)));
	this->InsertLines(cursors, text, 0);
	// FIXME: do something with the actual matches
	/*
	for (auto it = matches.begin(); it != matches.end(); it++) {
		PGCursorRange data = *it;
		data.start_position += std::to_string(data.start_line).size() + 2;
		data.end_position += std::to_string(data.start_line).size() + 2;
		data.start_line += current_line - start_line;
		data.end_line += current_line - start_line;
		//Cursor c = Cursor(nullptr, data.start_line, data.start_position, data.end_line, data.end_position);
		//matches.push_back(c.GetCursorSelection());
	}*/
	InvalidateBuffers(nullptr);
	VerifyTextfile();
	this->Unlock(PGWriteLock);

	this->InvalidateParsing();
}

void TextFile::FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data) {
	PGTextBuffer* current_buffer = buffers.front();
	lng current_position = 0;
	std::vector<PGCursorRange> matches;
	lng start_line = -1;
	lng end_line = -1;
	while (true) {
		PGTextRange match = FindMatch(regex_handle, PGDirectionRight, current_buffer, current_position, current_buffer, current_position, false, nullptr);
		if (!info->task->active) {
			// task is no longer active, cancel the search
			return;
		}
		if (match.start_buffer == nullptr) {
			// no more matches found
			// if there are any matches stored, report them now
			if (matches.size() > 0) {
				std::vector<std::string> context;
				for (lng current_line = start_line; current_line < end_line; current_line++) {
					TextLine line = GetLine(current_line);
					context.push_back(std::string(line.GetLine(), line.GetLength()));
				}
				callback(data, path.size() == 0 ? name : path, context, matches, start_line);
			}
			return;
		}
		// found a match
		// get the position of the match in the file
		lng line = 0, character = 0;
		lng cursor_end_line = 0, cursor_end_character = 0;
		match.start_buffer->GetCursorFromBufferLocation(match.start_position, line, character);
		match.end_buffer->GetCursorFromBufferLocation(match.end_position, cursor_end_line, cursor_end_character);
		PGCursorRange range = PGCursorRange(line, character, cursor_end_line, cursor_end_character);
		if (start_line < 0) {
			// there is no previous match
			// set the start_line and end_line
			start_line = std::max((lng)0, line - context_lines);
			end_line = std::min(linecount, cursor_end_line + context_lines + 1);
			matches.push_back(range);
		} else {
			// there is already a context from a previous match
			// first check if this match is part of that context
			if (line - context_lines <= end_line) {
				// the line is part of the previous context
				// we extend the current context and add the current match
				end_line = cursor_end_line + context_lines;
				matches.push_back(range);
			} else {
				// the line is not part of the previous context
				// report on the previously found context first
				std::vector<std::string> context;
				for (lng current_line = start_line; current_line < end_line; current_line++) {
					TextLine line = GetLine(current_line);
					context.push_back(std::string(line.GetLine(), line.GetLength()));
				}
				callback(data, path.size() == 0 ? name : path, context, matches, start_line);

				// now create a new context for the current match
				matches.clear();
				start_line = std::max((lng)0, line - context_lines);
				end_line = std::min(linecount, cursor_end_line + context_lines + 1);
				matches.push_back(range);
			}

		}
		current_buffer = match.end_buffer;
		current_position = match.end_position;
	}
	//textfield->SearchMatchesChanged();
}

void TextFile::FindAllMatchesAsync(PGGlobSet whitelist, ProjectExplorer* explorer, PGRegexHandle regex_handle, int context_lines, bool ignore_binary) {
	FindAllInformation* info = new FindAllInformation();
	info->textfile = this;
	info->regex_handle = regex_handle;
	info->ignore_binary = ignore_binary;
	info->context_lines = context_lines;
	info->whitelist = whitelist;
	info->explorer = explorer;
	info->notification = GetControlManager(explorer)->statusbar->AddNotification(
		PGStatusInProgress, 
		"Finding \"" + PGGetRegexPattern(regex_handle) + "\" In Files", "Finding in file...", true);


	this->find_task = std::shared_ptr<Task>(new Task([](std::shared_ptr<Task> task, void* data) {
		FindAllInformation* info = (FindAllInformation*)data;
		info->explorer->IterateOverFiles([](PGFile f, void* data, lng filenr, lng total_files) -> bool {
			FindAllInformation* info = (FindAllInformation*)data;
			if (info->whitelist && !PGGlobSetMatches(info->whitelist, f.path.c_str())) {
				return true;
			}	
			if (!info->task->active) {
				return false;
			}
			info->notification->SetText("Searching File \"" + f.path + "\"");
			info->notification->SetProgress((double)filenr / (double) total_files);

			lng size;
			PGFileError error = PGFileSuccess;
			// FIXME: use streaming textfile instead
			auto file = TextFile::OpenTextFile(f.path, error, true, info->ignore_binary);
			if (!file) {
				return true;
			}
			file->FindMatchesWithContext(info, info->regex_handle, info->context_lines, [](void* data, std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng start_line) {
				FindAllInformation* info = (FindAllInformation*)data;
				if (!info->task->active) {
					return;
				}
				info->textfile->AddFindMatches(filename, lines, matches, start_line);
			}, info);
			if (!info->task->active) {
				return false;
			}
			return true;
		}, info);
		if (info->regex_handle) {
			PGDeleteRegex(info->regex_handle);
		}
		if (info->whitelist) {
			PGDestroyGlobSet(info->whitelist);
		}
		if (info->notification) {
			GetControlManager(info->explorer)->statusbar->RemoveNotification(info->notification);
		}
		delete info;
	}, info));
	info->task = find_task;
	Scheduler::RegisterTask(this->find_task, PGTaskUrgent);
}

PGTextFileSettings TextFile::GetSettings() {
	PGTextFileSettings settings;
	settings.line_ending = GetLineEnding();
	settings.name = FileInMemory() ? this->name : "";
	return settings;
}

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
	cursors.push_back(new Cursor(this));
	this->path = "";
	this->name = std::string("untitled");
	this->text_lock = CreateMutex();
	this->lines.push_back(new TextLine(this, "", 0));
	is_loaded = true;
	unsaved_changes = true;
}

TextFile::TextFile(BasicTextField* textfield, std::string path, bool immediate_load) : textfield(textfield), highlighter(nullptr), path(path) {
	cursors.push_back(new Cursor(this));
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
	}
	LockMutex(text_lock);
	DestroyMutex(text_lock);
	for (auto it = lines.begin(); it != lines.end(); it++) {
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

void TextFile::RunHighlighter(Task* task, TextFile* textfile) {
	std::vector<TextBlock>& parsed_blocks = textfile->parsed_blocks;
	for (lng i = 0; i < textfile->GetMaximumBlocks(); i++) {
		if (!parsed_blocks[i].parsed) {
			// if we encounter a non-parsed block, parse it and any subsequent blocks that have to be parsed
			lng current_block = i;
			while (current_block < textfile->GetMaximumBlocks()) {
				if (textfile->current_task != task) {
					// a new syntax highlighting task has been activated
					// stop parsing with possibly outdated information
					parsed_blocks[current_block].parsed = false;
					return;
				}
				textfile->Lock(PGWriteLock);
				PGParseErrors errors;
				PGParserState oldstate = parsed_blocks[current_block].state;
				PGParserState state = current_block == 0 ? textfile->highlighter->GetDefaultState() : parsed_blocks[current_block - 1].state;
				lng last_line = current_block == parsed_blocks.size() - 1 ? textfile->lines.size() : std::min((lng)textfile->lines.size(), (lng)parsed_blocks[current_block + 1].line_start);
				for (lng i = parsed_blocks[current_block].line_start; i < last_line; i++) {
					TextLine* textline = textfile->lines[i];
					state = textfile->highlighter->IncrementalParseLine(*textline, i, state, errors);
				}
				parsed_blocks[current_block].parsed = true;
				parsed_blocks[current_block].state = textfile->highlighter->CopyParserState(state);
				bool equivalent = !oldstate ? false : textfile->highlighter->StateEquivalent(parsed_blocks[current_block].state, oldstate);
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

void TextFile::RefreshCursors() {
	if (!textfield) return;
	textfield->RefreshCursors();
}

int TextFile::GetLineHeight() {
	if (!textfield) return -1;
	return textfield->GetLineHeight();
}

void TextFile::InvalidateParsing(lng line) {
	assert(is_loaded);
	std::vector<lng> lines(1);
	lines.push_back(line);
	InvalidateParsing(lines);
}

void TextFile::InvalidateParsing(std::vector<lng>& invalidated_lines) {
	assert(is_loaded);
	if (!highlighter) return;
	// add any necessary blocks
	lng maximum_line = parsed_blocks.back().line_start + TEXTBLOCK_SIZE;
	while (maximum_line <= lines.size()) {
		parsed_blocks.push_back(TextBlock(maximum_line));
		maximum_line += TEXTBLOCK_SIZE;
	}
	// now set all of the invalidated blocks to unparsed
	for (size_t i = 0; i < invalidated_lines.size(); i++) {
		parsed_blocks[invalidated_lines[i] / TEXTBLOCK_SIZE].parsed = false;
	}
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
			lines.push_back(new TextLine(this, prev, (ptr - prev) - offset));
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
	if (lines.size() == 0) {
		lineending = GetSystemLineEnding();
	}

	panther::DestroyFileContents(base);
	loaded = 1;

	if (highlighter) {
		// we parse the first 10 blocks before opening the textfield for viewing
		// (heuristic: probably should be dependent on highlight speed/amount of text/etc)
		// anything else we schedule for highlighting in a separate thread
		// first create all the textblocks
		lng current_line = 0;
		while (current_line <= lines.size()) {
			parsed_blocks.push_back(TextBlock(current_line));
			current_line += TEXTBLOCK_SIZE;
		}
		// parse the first 10 blocks
		PGParseErrors errors;
		PGParserState state = highlighter->GetDefaultState();
		TextBlock* textblock = nullptr;
		lng line_count = std::min((lng)lines.size(), (lng)TEXTBLOCK_SIZE * 10);
		for (lng i = 0; i < line_count; i++) {
			if (i % TEXTBLOCK_SIZE == 0) {
				if (textblock) {
					textblock->state = highlighter->CopyParserState(state);
					textblock->parsed = true;
				}
				textblock = &parsed_blocks[i / TEXTBLOCK_SIZE];
			}
			state = highlighter->IncrementalParseLine(*lines[i], i, state, errors);
			if (i == line_count - 1) {
				textblock->state = highlighter->CopyParserState(state);
				textblock->parsed = true;
			}
		}
		highlighter->DeleteParserState(state);
		if (lines.size() > TEXTBLOCK_SIZE * 10) {
			this->current_task = new Task((PGThreadFunctionParams)RunHighlighter, (void*) this);
			Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
		}
	}
	is_loaded = true;
	UnlockMutex(text_lock);
}

// FIXME: "file has been modified without us being the one that modified it"
TextLine* TextFile::GetLine(lng linenumber) {
	if (!is_loaded) return nullptr;
	if (linenumber < 0 || linenumber >= (lng)lines.size())
		return nullptr;
	return lines[linenumber];
}

lng TextFile::GetLineCount() {
	if (!is_loaded) return 0;
	return lines.size();
}

void TextFile::SetMaxLineWidth(lng new_width) {
	if (new_width < 0) {
		lng max = 0;
		for (auto it = lines.begin(); it != lines.end(); it++) {
			max = std::max((lng)(*it)->line.size(), max);
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

void TextFile::InsertText(std::string text) {
	if (!is_loaded) return;
	// FIXME: merge delta if it already exists in sublime-text mode (so undo undoes multiple inserts at once)
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
		delta->AddDelta(new AddText(*it, (*it)->EndLine(), (*it)->EndPosition(), text));
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::OffsetLineOffset(lng offset) {
	lineoffset_y = lineoffset_y + offset;
	lineoffset_y = std::max(std::min(lineoffset_y, (lng)lines.size() - 1), (lng)0);
}

void TextFile::SetCursorLocation(lng line, lng character) {
	ClearExtraCursors();
	cursors[0]->SetCursorLocation(line, character);
	if (textfield) textfield->SelectionChanged();
}

void TextFile::SetCursorLocation(lng start_line, lng start_character, lng end_line, lng end_character) {
	ClearExtraCursors();
	cursors[0]->SetCursorStartLocation(end_line, end_character);
	cursors[0]->SetCursorEndLocation(start_line, start_character);
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
			if (direction == PGDirectionLeft) {
				(*it)->SetCursorLocation((*it)->BeginLine(), (*it)->BeginPosition());
			} else {
				(*it)->SetCursorLocation((*it)->EndLine(), (*it)->EndPosition());
			}
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
	this->cursors.push_back(new Cursor(this, lines.size() - 1, lines.back()->GetLength()));
	this->cursors.back()->end_character = 0;
	this->cursors.back()->end_line = 0;
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
		intervals.push_back(Interval((*it)->BeginLine(), (*it)->EndLine(), *it));
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
	assert(offset == 1 || offset == -1); // we only support single line moves
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if ((*it)->start_line + offset < 0 || (*it)->start_line + offset >= (lng)lines.size() ||
			(*it)->end_line + offset < 0 || (*it)->end_line + offset >= (lng)lines.size()) {
			// we do not move any lines if any of the movements are not possible
			return;
		}
	}
	// we keep track of all the line intervals that we have to move up/down
	std::vector<Interval> intervals = GetCursorIntervals();
	MultipleDelta* delta = new MultipleDelta();
	// now each of the intervals are separate
	// we must perform a RemoveLine and AddLine operation for each interval
	// we remove the line before (or after) the interval, and add it back after (or before) the interval
	// before or after depends on the offset
	for (auto it = intervals.begin(); it != intervals.end(); it++) {
		SwapLines* swap = new SwapLines(it->start_line, offset, *GetLine(it->start_line));
		for (lng j = it->start_line + 1; j <= it->end_line; j++) {
			swap->lines.push_back(*GetLine(j));
		}
		for (auto it2 = it->cursors.begin(); it2 != it->cursors.end(); it2++) {
			swap->cursors.push_back(*it2);
			swap->stored_cursors.push_back(**it2);
		}
		delta->AddDelta(swap);
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::DeleteLines() {
	if (!is_loaded) return;
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors contain a selection, delete only the selection
		DeleteCharacter(PGDirectionLeft);
		return;
	}
	// otherwise, delete each of the lines
	// first get the set of intervals covered by the cursors
	MultipleDelta* delta = new MultipleDelta();
	std::vector<Interval> intervals = GetCursorIntervals();
	for (auto it = intervals.begin(); it != intervals.end(); it++) {
		RemoveLines* remove = new RemoveLines(it->cursors.front(), it->start_line);
		for (lng i = it->start_line; i <= it->end_line; i++) {
			remove->AddLine(*lines[i]);
		}
		for (auto it2 = it->cursors.begin() + 1; it2 != it->cursors.end(); it2++) {
			delta->AddDelta(new CursorDelta(*it2, it->cursors.front()->start_line, 0));
		}
		delta->AddDelta(remove);
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::DeleteLine(PGDirection direction) {
	if (!is_loaded) return;
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors contain a selection, delete only the selection
		DeleteCharacter(PGDirectionLeft);
		return;
	}
	MultipleDelta* delta = new MultipleDelta();
	bool delete_lines = true;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (!(((*it)->start_character == 0 && direction == PGDirectionLeft) ||
			((*it)->start_character == lines[(*it)->start_line]->GetLength() && direction == PGDirectionRight))) {
			delete_lines = false;
			break;
		}
	}
	if (delete_lines) {
		DeleteCharacter(direction);
		return;
	}
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		bool delete_line = true;
		for (auto it2 = cursors.begin(); it2 != cursors.end(); it2++) {
			if (it == it2) continue;
			if ((*it2)->start_line == (*it)->start_line &&
				(((*it2)->start_character > (*it)->start_character && direction == PGDirectionLeft) ||
					((*it2)->start_character < (*it)->start_character && direction == PGDirectionRight))) {
				delete_line = false;
				break;
			}
		}
		if (delete_line) {
			if (direction == PGDirectionLeft) {
				delta->AddDelta(new RemoveText(*it, (*it)->start_line, (*it)->start_character, (*it)->start_character));
			} else {
				delta->AddDelta(new RemoveText(*it, (*it)->start_line, lines[(*it)->start_line]->GetLength(), lines[(*it)->start_line]->GetLength() - (*it)->start_character));
			}
		} else {
			delta->AddDelta(new CursorDelta(*it, (*it)->start_line, (*it)->start_character));
		}
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::AddEmptyLine(PGDirection direction) {
	if (!is_loaded) return;
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		bool add_newline = true;
		for (auto it2 = cursors.begin(); it2 != cursors.end(); it2++) {
			if (it == it2) continue;
			if ((*it2)->start_line == (*it)->start_line && (*it2)->start_character > (*it)->start_character) {
				add_newline = false;
				break;
			}
		}
		lng line = (*it)->start_line;
		if (direction == PGDirectionLeft) {
			line--;
		}
		if (add_newline) {
			delta->AddDelta(new AddLine(*it, line, lines[line]->GetLength(), ""));
		} else {
			delta->AddDelta(new CursorDelta(*it, line + 1, 0));
		}
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
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
			text += this->GetLine((*it)->SelectedLine())->GetLine();
		}
	}
	return text;
}

void TextFile::PasteText(std::string text) {
	if (!is_loaded) return;
	std::vector<std::string> pasted_lines;
	lng start = 0;
	lng current = 0;
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
		InsertText(pasted_lines[0]);
	} else {
		AddNewLines(pasted_lines, false);
	}
}


void
TextFile::DeleteCharacter(MultipleDelta* delta, Cursor* it, PGDirection direction, bool delete_selection, bool include_cursor) {
	if (!is_loaded) return;
	if (delete_selection) {
		if (!it->SelectionIsEmpty()) {
			// delete with selection
			RemoveLines *remove_lines = new RemoveLines(nullptr, it->BeginLine() + 1);
			for (lng i = it->BeginLine(); i <= it->EndLine(); i++) {
				if (i == it->BeginLine()) {
					if (i == it->EndLine()) {
						delete remove_lines;
						delta->AddDelta(new RemoveText(include_cursor ? &*it : nullptr, i, it->EndPosition(), it->EndPosition() - it->BeginPosition()));
					} else {
						delta->AddDelta(new RemoveText(include_cursor ? &*it : nullptr, i, lines[i]->GetLength(), lines[i]->GetLength() - it->BeginPosition()));
					}
				} else if (i == it->EndLine()) {
					assert(it->EndPosition() <= lines[i]->GetLength());
					// remove part of the last line
					std::string text = std::string(lines[i]->GetLine() + it->EndPosition(), lines[i]->GetLength() - it->EndPosition());
					remove_lines->AddLine(*lines[i]);
					remove_lines->last_line_offset = it->EndPosition();
					delta->AddDelta(remove_lines);
					delta->AddDelta(new AddText(nullptr, it->BeginLine(), it->BeginPosition(), text));
				} else {
					// remove the entire line
					remove_lines->AddLine(*lines[i]);
				}
			}
		} else {
			if (include_cursor) {
				delta->AddDelta(new CursorDelta(it, it->start_line, it->start_character));
			}
		}
	} else {
		lng characternumber = it->SelectedPosition();
		lng linenumber = it->SelectedLine();
		if (direction == PGDirectionLeft && characternumber == 0) {
			if (linenumber > 0) {
				// merge the line with the previous line
				std::string line = std::string(lines[linenumber]->GetLine(), lines[linenumber]->GetLength());
				delta->AddDelta(new RemoveLine(include_cursor ? &*it : nullptr, linenumber, *lines[linenumber]));
				TextDelta *addText = new AddText(nullptr, linenumber - 1, lines[linenumber - 1]->GetLength(), line);
				delta->AddDelta(addText);
			}
		} else if (direction == PGDirectionRight && characternumber == lines[linenumber]->GetLength()) {
			if (linenumber < GetLineCount() - 1) {
				// merge the next line with the current line
				std::string line = std::string(lines[linenumber + 1]->GetLine(), lines[linenumber + 1]->GetLength());
				delta->AddDelta(new RemoveLine(include_cursor ? &*it : nullptr, linenumber + 1, *lines[linenumber + 1]));
				TextDelta *addText = new AddText(nullptr, linenumber, lines[linenumber]->GetLength(), line);
				delta->AddDelta(addText);
			}
		} else {
			// FIXME: merge delta if it already exists in sublime-text mode
			RemoveText* remove;
			if (direction == PGDirectionLeft) {
				remove = new RemoveText(include_cursor ? &*it : nullptr,
					linenumber,
					characternumber,
					characternumber - utf8_prev_character(lines[linenumber]->GetLine(), characternumber));
			} else {
				int offset = utf8_character_length(lines[linenumber]->GetLine()[characternumber]);
				remove = new RemoveText(include_cursor ? &*it : nullptr,
					linenumber,
					characternumber + offset,
					offset);
			}
			delta->AddDelta(remove);
		}
	}
}
void
TextFile::DeleteCharacter(MultipleDelta* delta, PGDirection direction) {
	if (!is_loaded) return;
	bool delete_selection = CursorsContainSelection(cursors);
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		DeleteCharacter(delta, *it, direction, delete_selection);
	}
}

void TextFile::DeleteCharacter(PGDirection direction) {
	if (!is_loaded) return;
	MultipleDelta* delta = new MultipleDelta();
	DeleteCharacter(delta, direction);
	if (delta->deltas.size() == 0) {
		delete delta;
		return;
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::DeleteWord(PGDirection direction) {
	if (!is_loaded) return;
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if ((*it)->SelectionIsEmpty() && (direction == PGDirectionLeft ? (*it)->SelectedPosition() > 0 : (*it)->SelectedPosition() < lines[(*it)->SelectedLine()]->GetLength())) {
			int offset = direction == PGDirectionLeft ? -1 : 1;
			char* line = lines[(*it)->SelectedLine()]->GetLine();
			lng length = lines[(*it)->SelectedLine()]->GetLength();
			lng index = direction == PGDirectionLeft ? (*it)->SelectedPosition() + offset : (*it)->SelectedPosition();
			// a word is a series of characters of equal type (punctuation, spaces or everything else)
			// we scan the string to find out where the word ends 
			PGCharacterClass type = GetCharacterClass(line[index]);
			for (index += offset; index >= 0 && index < length; index += offset) {
				if (GetCharacterClass(line[index]) != type) {
					index -= direction == PGDirectionLeft ? offset : 0;
					break;
				}
			}
			index = std::min(std::max(index, (lng)0), length);
			delta->AddDelta(new RemoveText(*it, (*it)->SelectedLine(), std::max(index, (*it)->SelectedPosition()), std::abs((*it)->SelectedPosition() - index)));
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
	Cursor::NormalizeCursors(this, cursors);
}

void TextFile::AddNewLine() {
	if (!is_loaded) return;
	AddNewLine("");
}

void TextFile::AddNewLine(std::string text) {
	if (!is_loaded) return;
	std::vector<std::string> lines;
	lines.push_back(text);
	AddNewLines(lines, true);
}

void TextFile::AddNewLines(std::vector<std::string>& added_text, bool first_is_newline) {
	if (!is_loaded) return;
	MultipleDelta* delta = new MultipleDelta();
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors select text, we delete that text before inserting the characters
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (!(*it)->SelectionIsEmpty()) {
				DeleteCharacter(delta, *it, PGDirectionLeft, true, false);
			}
		}
		// FIXME: I don't know if this is necessary
		/*for (int i = 0; i < delta->deltas.size(); i++) {
			if (delta->deltas[i]->TextDeltaType() == PGDeltaAddText) {
				delta->deltas.erase(delta->deltas.begin() + i);
				i--;
			}
		}*/
	}
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		lng characternumber = (*it)->EndPosition();
		lng linenumber = (*it)->EndLine();
		lng length = lines[linenumber]->GetLength();
		lng current_line = (*it)->BeginLine();

		if (added_text.size() == 1) {
			delta->AddDelta(new AddLine(*it, current_line, (*it)->BeginPosition(), added_text.front()));
			continue;
		}
		std::vector<std::string> text = added_text;
		if (!first_is_newline) {
			assert(added_text.size() > 1); // if added_text == 1 and first_is_newline is false, use InsertText
			delta->AddDelta(new AddText(nullptr, (*it)->BeginLine(), (*it)->BeginPosition(), added_text.front()));
			text.erase(text.begin());
		}
		delta->AddDelta(new AddLines(*it, (*it)->BeginLine(), (*it)->BeginPosition(), text));
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(this, cursors);
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
	this->ClearCursors();
	std::vector<lng> invalidated_lines;
	Lock(PGWriteLock);
	this->Undo(delta, invalidated_lines);
	Unlock(PGWriteLock);
	InvalidateParsing(invalidated_lines);
	if (this->textfield) {
		std::vector<TextLine*> invalidated_textlines;
		for (auto it = invalidated_lines.begin(); it != invalidated_lines.end(); it++) {
			invalidated_textlines.push_back(lines[*it]);
		}
		this->textfield->TextChanged(invalidated_textlines);
	}
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	this->deltas.pop_back();
	this->redos.push_back(delta);
}

void TextFile::Redo() {
	if (!is_loaded) return;
	if (this->redos.size() == 0) return;
	TextDelta* delta = this->redos.back();
	this->ClearCursors();
	this->Redo(delta);
	this->PerformOperation(delta, false);
	this->redos.pop_back();
	this->deltas.push_back(delta);
	// FIXME is this necessary?
	//Cursor::NormalizeCursors(this, cursors);
}

void TextFile::AddDelta(TextDelta* delta) {
	if (!is_loaded) return;
	for (auto it = redos.begin(); it != redos.end(); it++) {
		delete *it;
	}
	redos.clear();
	this->deltas.push_back(delta);
}

static void ShiftDelta(lng& linenr, lng& characternr, lng shift_lines, lng shift_lines_line, lng shift_lines_character, lng shift_characters, lng shift_characters_line, lng shift_characters_character, lng last_line_offset) {
	if (linenr == shift_characters_line &&
		characternr >= shift_characters_character) {
		characternr += shift_characters;
	} else if (linenr == shift_characters_line &&
		characternr < 0) {
		// special case: see below explanation
		characternr = shift_characters_character + std::abs(characternr + 1);
	}
	if (shift_lines != 0) {
		if (linenr == (shift_lines_line + std::abs(shift_lines) - 1) && shift_lines < 0) {
			// special case: multi-line deletion that ends with deleting part of a line
			// this is done by removing the entire last line and then adding part of the line back
			// removing and readding text messes with cursors on that line,
			// to make the cursor go back to the proper position, 
			// we store the position of this delta on the original line as a negative number when removing it
			// when performing the AddText we do a special case for negative numbers
			// to shift the cursor position to its original position in the text again
			characternr = -(characternr - last_line_offset) - 1;
			linenr = shift_lines_line - 1;
		} else if (linenr >= shift_lines_line) {
			linenr += shift_lines;
		}
	}
}

void TextFile::PerformOperation(TextDelta* delta, bool adjust_delta) {
	// set the current_task to the nullptr, this will cause any active syntax highlighting to end
	// this prevents long syntax highlighting tasks (e.g. highlighting a long document) from 
	// locking us out of editing until they're finished
	SetUnsavedChanges(true);
	current_task = nullptr;
	std::vector<lng> invalidated_lines;
	// lock the blocks
	Lock(PGWriteLock);
	// perform the operation
	PerformOperation(delta, invalidated_lines, adjust_delta);
	// release the locks again
	Unlock(PGWriteLock);
	if (this->textfield) {
		std::vector<TextLine*> invalidated_textlines;
		for (auto it = invalidated_lines.begin(); it != invalidated_lines.end(); it++) {
			invalidated_textlines.push_back(lines[*it]);
		}
		this->textfield->TextChanged(invalidated_textlines);
	}
	// invalidate any lines for parsing
	InvalidateParsing(invalidated_lines);
}

void TextFile::PerformOperation(TextDelta* delta, std::vector<lng>& invalidated_lines, bool adjust_delta) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr]->AddDelta(this, delta);
		invalidated_lines.push_back(add->linenr);
		if (add->cursor) {
			add->cursor->end_character = add->cursor->start_character = add->characternr + add->text.size();
			add->cursor->end_line = add->cursor->start_line = add->linenr;
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr]->AddDelta(this, delta);
		invalidated_lines.push_back(remove->linenr);
		if (remove->cursor) {
			remove->cursor->start_character = remove->cursor->end_character = (remove->characternr - remove->charactercount);
			remove->cursor->end_line = remove->cursor->start_line = remove->linenr;
		}
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		lng length = lines[add->linenr]->GetLength();
		add->extra_text = std::string(add->line);
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr]->GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(nullptr, add->linenr, length, length - add->characternr);
			lines[add->linenr]->AddDelta(this, add->remove_text);
			invalidated_lines.push_back(add->linenr);
		}
		lines.insert(lines.begin() + (add->linenr + 1), new TextLine(this, (char*)add->extra_text.c_str(), (lng)add->extra_text.size()));
		invalidated_lines.push_back(add->linenr + 1);
		if (add->cursor) {
			add->cursor->end_line = add->cursor->start_line = add->linenr + 1;
			add->cursor->end_character = add->cursor->start_character = add->line.size();
		}
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		lng length = lines[add->linenr]->GetLength();
		lng linenr = add->linenr + 1;
		for (auto it = add->lines.begin(); (it + 1) != add->lines.end(); it++) {
			lines.insert(lines.begin() + linenr, new TextLine(this, (char*)it->c_str(), (lng)it->size()));
			invalidated_lines.push_back(linenr);
			linenr++;
		}
		add->extra_text = add->lines.back();
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr]->GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(nullptr, add->linenr, length, length - add->characternr);
			lines[add->linenr]->AddDelta(this, add->remove_text);
			invalidated_lines.push_back(add->linenr);
		}
		lines.insert(lines.begin() + linenr, new TextLine(this, (char*)add->extra_text.c_str(), (lng)add->extra_text.size()));
		invalidated_lines.push_back(linenr);
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
			remove->cursor->end_character = remove->cursor->start_character = this->lines[remove->cursor->start_line]->GetLength();
		}
		this->lines.erase(this->lines.begin() + remove->linenr);
		invalidated_lines.push_back(remove->linenr);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		if (remove->cursor) {
			remove->cursor->end_line = remove->cursor->start_line = remove->linenr;
			remove->cursor->end_character = remove->cursor->start_character = 0;
		}
		this->lines.erase(this->lines.begin() + remove->linenr, this->lines.begin() + remove->linenr + remove->lines.size());
		invalidated_lines.push_back(remove->linenr);
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
	case PGDeltaSwapLines: {
		SwapLines* swap = (SwapLines*)delta;
		if (swap->offset == -1) {
			this->lines.insert(this->lines.begin() + swap->linenr + swap->lines.size(), this->lines[swap->linenr - 1]);
			this->lines.erase(this->lines.begin() + swap->linenr - 1);
		} else {
			assert(swap->offset == 1);
			this->lines.insert(this->lines.begin() + swap->linenr, this->lines[swap->linenr + swap->lines.size()]);
			this->lines.erase(this->lines.begin() + swap->linenr + swap->lines.size() + 1);
		}
		for (auto it = swap->cursors.begin(); it != swap->cursors.end(); it++) {
			(*it)->start_line += swap->offset;
			(*it)->end_line += swap->offset;
		}
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (auto it = multi->deltas.begin(); it != multi->deltas.end(); it++) {
			assert((*it)->TextDeltaType() != PGDeltaMultiple); // nested multiple deltas not supported
			PerformOperation(*it, invalidated_lines, adjust_delta);
			if (!adjust_delta) continue;

			lng shift_lines = 0;
			lng shift_lines_line = (*it)->linenr;
			lng shift_lines_character = (*it)->characternr;
			lng shift_characters = 0;
			lng shift_characters_line = (*it)->linenr;
			lng shift_characters_character = (*it)->characternr;
			lng last_line_offset = 0;
			switch ((*it)->TextDeltaType()) {
			case PGDeltaRemoveLine:
				shift_lines = -1;
				break;
			case PGDeltaRemoveManyLines:
				shift_lines = -((lng)((RemoveLines*)(*it))->lines.size());
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
				shift_characters = (lng)((AddText*)(*it))->text.size();
				break;
			case PGDeltaRemoveText:
				shift_characters = -((lng)((RemoveText*)(*it))->charactercount);
				break;
			default:
				break;
			}
			for (auto it2 = it + 1; it2 != multi->deltas.end(); it2++) {
				TextDelta* delta = *it2;
				ShiftDelta(delta->linenr, delta->characternr, shift_lines, shift_lines_line, shift_lines_character, shift_characters, shift_characters_line, shift_characters_character, last_line_offset);
			}
			if ((*it)->TextDeltaType() != PGDeltaRemoveManyLines) {
				last_line_offset = 0;
			}
		}
		break;
	}
	default:
		break;
	}
}

void TextFile::Undo(TextDelta* delta, std::vector<lng>& invalidated_lines) {
	assert(delta);
	CursorDelta* cursor_delta = dynamic_cast<CursorDelta*>(delta);
	if (cursor_delta && cursor_delta->cursor) {
		cursors.push_back(new Cursor(this));
		cursors.back()->RestoreCursor(cursor_delta->stored_cursor);
	}
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr]->PopDelta(this);
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr]->PopDelta(this);
		invalidated_lines.push_back(remove->linenr);
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		if (add->remove_text) {
			lines[add->linenr]->RemoveDelta(this, add->remove_text);
			add->remove_text = nullptr;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1));
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		if (add->remove_text) {
			lines[add->linenr]->RemoveDelta(this, add->remove_text);
			add->remove_text = nullptr;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1), this->lines.begin() + (add->linenr + 1 + add->lines.size()));
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		this->lines.insert(this->lines.begin() + remove->linenr, new TextLine(remove->line));
		invalidated_lines.push_back(remove->linenr);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		lng index = remove->linenr;
		for (auto it = remove->lines.begin(); it != remove->lines.end(); it++, index++) {
			this->lines.insert(this->lines.begin() + index, new TextLine(*it));
			invalidated_lines.push_back(index);
		}
		break;
	}
	case PGDeltaSwapLines: {
		SwapLines* swap = (SwapLines*)delta;
		if (swap->offset == 1) {
			this->lines.insert(this->lines.begin() + swap->linenr + swap->lines.size() + 1, this->lines[swap->linenr]);
			this->lines.erase(this->lines.begin() + swap->linenr);
		} else {
			assert(swap->offset == -1);
			this->lines.insert(this->lines.begin() + swap->linenr - 1, this->lines[swap->linenr + swap->lines.size() - 1]);
			this->lines.erase(this->lines.begin() + swap->linenr + swap->lines.size());
		}
		invalidated_lines.push_back(swap->linenr - 1);
		invalidated_lines.push_back(swap->linenr);

		swap->cursors.clear();
		for (auto it = swap->stored_cursors.begin(); it != swap->stored_cursors.end(); it++) {
			Cursor* cursor = new Cursor(this);
			swap->cursors.push_back(cursor);
			cursors.push_back(cursor);
			cursor->RestoreCursor(*it);
		}
		break;
	}
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (lng index = multi->deltas.size() - 1; index >= 0; index--) {
			Undo(multi->deltas[index], invalidated_lines);
		}
		break;
	}
	default:
		break;
	}
}

void TextFile::Redo(TextDelta* delta) {
	assert(delta);
	CursorDelta* cursor_delta = dynamic_cast<CursorDelta*>(delta);
	if (cursor_delta && cursor_delta->cursor) {
		cursor_delta->cursor = new Cursor(this);
		cursors.push_back(cursor_delta->cursor);
		cursor_delta->cursor->RestoreCursor(cursor_delta->stored_cursor);
	}
	if (delta->TextDeltaType() == PGDeltaSwapLines) {
		SwapLines* swap = (SwapLines*)delta;
		swap->cursors.clear();
		for (auto it = swap->stored_cursors.begin(); it != swap->stored_cursors.end(); it++) {
			Cursor* cursor = new Cursor(this);
			swap->cursors.push_back(cursor);
			cursors.push_back(cursor);
			cursor->RestoreCursor(*it);
		}
	}

	switch (delta->TextDeltaType()) {
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (auto it = multi->deltas.begin(); it != multi->deltas.end(); it++) {
			Redo(*it);
		}
		break;
	}
	default:
		break;
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
	for (auto it = lines.begin(); it != lines.end(); it++) {
		lng length = (*it)->GetLength();
		char* line = (*it)->GetLine();
		panther::WriteToFile(handle, line, length);

		if (it + 1 != lines.end()) {
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

std::vector<Cursor> TextFile::BackupCursors() {
	std::vector<Cursor> backup;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		backup.push_back(Cursor(**it));
	}
	return backup;
}

void TextFile::RestoreCursors(std::vector<Cursor>& backup) {
	ClearCursors();
	for (auto it = backup.begin(); it != backup.end(); it++) {
		cursors.push_back(new Cursor(*it));
	}
	Cursor::NormalizeCursors(this, cursors, false);
}

PGFindMatch TextFile::FindMatch(std::string text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex) {
	if (!match_case) {
		text = utf8_tolower(text);
	}

	// we start "outside" of the current selection
	// e.g. if we go right, we start at the end and continue right
	// if we go left, we start at the beginning and go left
	lng begin_line = direction == PGDirectionLeft ? start_line : end_line;
	lng begin_character = direction == PGDirectionLeft ? start_character : end_character;
	int offset = direction == PGDirectionLeft ? -1 : 1;
	lng end = direction == PGDirectionLeft ? 0 : lines.size() - 1;
	lng i = begin_line;
	if (!regex) {
		bool wrapped_search = false;
		while(true) {
			for (; i != end; i += offset) {
				std::string line = match_case ? lines[i]->GetString() : utf8_tolower(lines[i]->GetString());
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
				i = direction == PGDirectionLeft ? lines.size() - 1 : 0;
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
		while(true) {
			for (; i != end; i += offset) {
				std::string& line = lines[i]->GetString();
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
					match.start += addition;
					match.end += addition;
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
				i = direction == PGDirectionLeft ? lines.size() - 1 : 0;
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
	char* error_message = nullptr;
	textfile->Lock(PGReadLock);
	PGFindMatch first_match = textfile->FindMatch(text, direction, start_line, start_character, end_line, end_character, &error_message, match_case, wrap, regex);
	textfile->Unlock(PGReadLock);
	// we should never fail here, because we already have compiled the regex
	assert(error_message == nullptr);
	textfile->Lock(PGWriteLock);
	if (first_match.start_line < 0) {
		return; // no matches found
	} else {
		textfile->SetCursorLocation(first_match.start_line, first_match.start_character, first_match.end_line, first_match.end_character);
	}
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
		match = textfile->FindMatch(text, direction, match.start_line, match.start_character, match.end_line, match.end_character, &error_message, match_case, wrap, regex);
		textfile->Unlock(PGReadLock);
		assert(error_message == nullptr);
		if (match.start_line == first_match.start_line &&
			match.start_character == first_match.start_character &&
			match.end_line == first_match.end_line &&
			match.end_character == first_match.end_character) {
			// the current match equals the first match, we are done looking
			return;
		}
		textfile->Lock(PGWriteLock);
		textfile->matches.push_back(match);
		textfile->Unlock(PGWriteLock);
		textfile->textfield->Invalidate();
	}
}

void TextFile::ClearMatches() {
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

	this->ClearMatches();

	this->find_task = new Task([](Task* t, void* in) {
		FindInformation* info = (FindInformation*)in;
		RunTextFinder(t, info->textfile, info->pattern, info->direction, info->start_line,
			info->start_character, info->end_line, info->end_character, info->match_case,
			info->wrap, info->regex);
		delete info;
	}, (void*)info);
	Scheduler::RegisterTask(this->find_task, PGTaskUrgent);
}

void TextFile::FindMatch(std::string text, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, char** error_message, bool match_case, bool wrap, bool regex, lng& selected_match) {
	PGFindMatch match;
	if (selected_match >= 0) {
		// if FindAll has been performed, we already have all the matches
		// simply select the next match, rather than searching again
		Lock(PGWriteLock);
		if (direction == PGDirectionLeft) {
			selected_match = selected_match == 0 ? matches.size() - 1 : selected_match - 1;
		} else {
			selected_match = selected_match == matches.size() - 1 ? 0 : selected_match + 1;
		}

		match = matches[selected_match];
		if (match.start_character >= 0) {
			this->SetCursorLocation(match.start_line, match.start_character, match.end_line, match.end_character);
		}
		Unlock(PGWriteLock);
		return;
	}
	// otherwise, search only for the next match in either direction
	*error_message = nullptr;
	Lock(PGReadLock);
	match = this->FindMatch(text, direction, 
		GetActiveCursor()->BeginLine(), GetActiveCursor()->BeginPosition(), 
		GetActiveCursor()->EndLine(), GetActiveCursor()->EndPosition(),
		error_message, match_case, wrap, regex);
	Unlock(PGReadLock);
	if (!(*error_message)) {
		Lock(PGWriteLock);
		if (match.start_character >= 0) {
			SetCursorLocation(match.start_line, match.start_character, match.end_line, match.end_character);
		}
		matches.clear();
		matches.push_back(match);
		Unlock(PGWriteLock);
	}
}

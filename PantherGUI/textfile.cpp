
#include "textfield.h"
#include "textfile.h"
#include "text.h"
#include <algorithm>
#include <fstream>
#include "scheduler.h"
#include "unicode.h"

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

TextFile::TextFile(TextField* textfield) : textfield(textfield), highlighter(nullptr) {
	cursors.push_back(new Cursor(this));
	this->path = "";
	this->name = std::string("untitled");
	this->text_lock = CreateMutex();
	this->lines.push_back(new TextLine("", 0));
	is_loaded = true;
	unsaved_changes = true;
}

TextFile::TextFile(TextField* textfield, std::string path, bool immediate_load) : textfield(textfield), highlighter(nullptr), path(path) {
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
				textfile->Lock();
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
				textfile->Unlock();
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

void TextFile::Lock() {
	assert(is_loaded);
	LockMutex(text_lock);
}

void TextFile::Unlock() {
	assert(is_loaded);
	UnlockMutex(text_lock);
}

void TextFile::OpenFile(std::string path) {
	lng size = 0;
	char* base = (char*)PGmmap::ReadFile(path, size);
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
			lines.push_back(new TextLine(prev, (ptr - prev) - offset));
			loaded = ((double)(ptr - base)) / size;

			prev = ptr + 1;
			offset = 0;
			
			if (pending_delete) {
				PGmmap::DestroyFileContents(base);
				UnlockMutex(text_lock);
				return;
			}

		}
		ptr += character_offset;
	}
	if (lines.size() == 0) {
		lineending = GetSystemLineEnding();
	}
	lines.push_back(new TextLine(prev, (ptr - prev) - offset));

	PGmmap::DestroyFileContents(base);
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
	InsertText(std::string((char*) u.character, u.length));
}

void TextFile::InsertText(std::string text) {
	if (!is_loaded) return;
	// FIXME: merge delta if it already exists in sublime-text mode
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
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it != cursors.begin()) {
			text += NEWLINE_CHARACTER;
		}
		text += (*it)->GetText();
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
	Lock();
	this->Undo(delta, invalidated_lines);
	Unlock();
	InvalidateParsing(invalidated_lines);
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

// Determine need to be locked to perform a specific operation
// Returns true if all blocks need to be locked (because of 
static bool LockOperation(TextDelta* delta, std::vector<lng> blocks) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		lng block = add->linenr / TEXTBLOCK_SIZE;
		if (std::find(blocks.begin(), blocks.end(), block) != blocks.end()) {
			blocks.push_back(block);
		}
		return false;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		lng block = remove->linenr / TEXTBLOCK_SIZE;
		if (std::find(blocks.begin(), blocks.end(), block) != blocks.end()) {
			blocks.push_back(block);
		}
		return false;
	}
	case PGDeltaAddLine:
	case PGDeltaAddManyLines:
	case PGDeltaRemoveLine:
	case PGDeltaRemoveManyLines:
	case PGDeltaSwapLines:
		return true;
	case PGDeltaCursorMovement:
		return false;
	case PGDeltaMultiple: {
		MultipleDelta* multi = (MultipleDelta*)delta;
		assert(multi->deltas.size() > 0);
		for (auto it = multi->deltas.begin(); it != multi->deltas.end(); it++) {
			assert((*it)->TextDeltaType() != PGDeltaMultiple); // nested multiple deltas not supported
			if (LockOperation(delta, blocks)) {
				return true;
			}
		}
		return false;
	}
	default:
		return false;
	}
}

void TextFile::PerformOperation(TextDelta* delta, bool adjust_delta) {
	// set the current_task to the nullptr, this will cause any active syntax highlighting to end
	// this prevents long syntax highlighting tasks (e.g. highlighting a long document) from 
	// locking us out of editing
	SetUnsavedChanges(true);
	current_task = nullptr;
	std::vector<lng> invalidated_lines;
	// lock the blocks
	Lock();
	// perform the operation
	PerformOperation(delta, invalidated_lines, adjust_delta);
	// release the locks again
	Unlock();
	// invalidate any lines for parsing
	InvalidateParsing(invalidated_lines);
}

void TextFile::PerformOperation(TextDelta* delta, std::vector<lng>& invalidated_lines, bool adjust_delta) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr]->AddDelta(delta);
		invalidated_lines.push_back(add->linenr);
		if (add->cursor) {
			add->cursor->end_character = add->cursor->start_character = add->characternr + add->text.size();
			add->cursor->end_line = add->cursor->start_line = add->linenr;
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr]->AddDelta(delta);
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
			lines[add->linenr]->AddDelta(add->remove_text);
			invalidated_lines.push_back(add->linenr);
		}
		lines.insert(lines.begin() + (add->linenr + 1), new TextLine((char*)add->extra_text.c_str(), (lng)add->extra_text.size()));
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
			lines.insert(lines.begin() + linenr, new TextLine((char*)it->c_str(), (lng)it->size()));
			invalidated_lines.push_back(linenr);
			linenr++;
		}
		add->extra_text = add->lines.back();
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr]->GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(nullptr, add->linenr, length, length - add->characternr);
			lines[add->linenr]->AddDelta(add->remove_text);
			invalidated_lines.push_back(add->linenr);
		}
		lines.insert(lines.begin() + linenr, new TextLine((char*)add->extra_text.c_str(), (lng)add->extra_text.size()));
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
		this->lines[add->linenr]->PopDelta();
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr]->PopDelta();
		invalidated_lines.push_back(remove->linenr);
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		if (add->remove_text) {
			lines[add->linenr]->RemoveDelta(add->remove_text);
			add->remove_text = nullptr;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1));
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		if (add->remove_text) {
			lines[add->linenr]->RemoveDelta(add->remove_text);
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
	if (this->path == "") return; // FIXME: in-memory file
	SetUnsavedChanges(false);
	this->Lock();
	PGLineEnding line_ending = lineending;
	if (line_ending != PGLineEndingWindows && line_ending != PGLineEndingMacOS && line_ending != PGLineEndingUnix) {
		line_ending = GetSystemLineEnding();
	}
	// FIXME: respect file encoding
	PGFileHandle handle = PGmmap::OpenFile(this->path, PGFileReadWrite);
	lng position = 0;
	for (auto it = lines.begin(); it != lines.end(); it++) {
		lng length = (*it)->GetLength();
		char* line = (*it)->GetLine();
		PGmmap::WriteToFile(handle, line, length);

		if (it + 1 != lines.end()) {
			switch (line_ending) {
			case PGLineEndingWindows:
				PGmmap::WriteToFile(handle, "\r\n", 2);
				break;
			case PGLineEndingMacOS:
				PGmmap::WriteToFile(handle, "\r", 1);
				break;
			case PGLineEndingUnix:
				PGmmap::WriteToFile(handle, "\n", 1);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	this->Unlock();
	PGmmap::CloseFile(handle);
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


#include "textfield.h"
#include "textfile.h"
#include "text.h"
#include <algorithm>
#include <fstream>
#include "scheduler.h"
#include "xml.h"
#include "c.h"

TextFile::TextFile(std::string path) : textfield(nullptr), base(nullptr), highlighter(nullptr), current_task(nullptr) {
	OpenFile(path);
}

TextFile::TextFile(std::string path, TextField* textfield) : textfield(textfield), base(nullptr), highlighter(nullptr), current_task(nullptr) {
	highlighter = new CHighlighter();
	OpenFile(path);
	if (highlighter) {
		// we parse the first 10 blocks before opening the textfield for viewing
		// (heuristic: probably should be dependent on highlight speed/amount of text/etc)
		// anything else we schedule for highlighting in a separate thread
		// first create all the textblocks
		ssize_t current_line = 0;
		while (current_line <= lines.size()) {
			parsed_blocks.push_back(TextBlock(current_line));
			block_locks.push_back(CreateMutex());
			current_line += TEXTBLOCK_SIZE;
		}
		// parse the first 10 blocks
		PGParseErrors errors;
		PGParserState state = highlighter->GetDefaultState();
		TextBlock* textblock = nullptr;
		ssize_t line_count = std::min((ssize_t) lines.size(), (ssize_t) TEXTBLOCK_SIZE * 10);
		for (ssize_t i = 0; i <= line_count; i++) {
			if (i % TEXTBLOCK_SIZE == 0) {
				if (textblock) {
					textblock->state = highlighter->CopyParserState(state);
					textblock->parsed = true;
				}
				textblock = &parsed_blocks[i / TEXTBLOCK_SIZE];
			}
			state = highlighter->IncrementalParseLine(lines[i], i, state, errors);
			if (i == line_count) {
				textblock->state = highlighter->CopyParserState(state);
				textblock->parsed = true;
			}
		}
		highlighter->DeleteParserState(state);
		if (lines.size() > TEXTBLOCK_SIZE * 10) {
			this->current_task = new Task((PGThreadFunctionParams) RunHighlighter, (void*) this);
			Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
		}
	}
}

TextFile::~TextFile() {
	if (base) {
		mmap::DestroyFileContents(base);
	}
	if (current_task) {
		current_task->active = false;
	}
	for (auto it = block_locks.begin(); it != block_locks.end(); it++) {
		DestroyMutex(*it);
	}
}

void TextFile::RunHighlighter(Task* task, TextFile* textfile) {
	std::vector<TextBlock>& parsed_blocks = textfile->parsed_blocks;
	for (ssize_t i = 0; i < textfile->GetMaximumBlocks(); i++) {
		if (!parsed_blocks[i].parsed) {
			// if we encounter a non-parsed block, parse it and any subsequent blocks that have to be parsed
			ssize_t current_block = i;
			while (current_block < textfile->GetMaximumBlocks()) {
				textfile->LockBlock(current_block);
				PGParseErrors errors;
				PGParserState oldstate = parsed_blocks[current_block].state;
				PGParserState state = current_block == 0 ? textfile->highlighter->GetDefaultState() : parsed_blocks[current_block - 1].state;
				ssize_t last_line = current_block == parsed_blocks.size() - 1 ? textfile->lines.size() : std::min((ssize_t) textfile->lines.size(), (ssize_t) parsed_blocks[current_block + 1].line_start);
				for (ssize_t i = parsed_blocks[current_block].line_start; i < last_line; i++) {
					state = textfile->highlighter->IncrementalParseLine(textfile->lines[i], i, state, errors);
				}
				parsed_blocks[current_block].parsed = true;
				parsed_blocks[current_block].state =  textfile->highlighter->CopyParserState(state);
				bool equivalent = !oldstate ? false : textfile->highlighter->StateEquivalent(parsed_blocks[current_block].state, oldstate);
				if (oldstate) {
					textfile->highlighter->DeleteParserState(oldstate);
				}
				textfile->UnlockBlock(current_block);
				if (textfile->current_task != task) {
					// a new syntax highlighting task has been activated
					// stop parsing with possibly outdated information
					parsed_blocks[current_block].parsed = false;
					return;
				}
				if (equivalent) {
					break;
				}
				current_block++;
			}
		}
	}
	textfile->current_task = nullptr;
}

void TextFile::InvalidateParsing(ssize_t line) {
	std::vector<ssize_t> lines(1);
	lines.push_back(line);
	InvalidateParsing(lines);
}

void TextFile::InvalidateParsing(std::vector<ssize_t>& invalidated_lines) {
	// add any necessary blocks
	ssize_t maximum_line = parsed_blocks.back().line_start + TEXTBLOCK_SIZE;
	while (maximum_line <= lines.size()) {
		parsed_blocks.push_back(TextBlock(maximum_line));
		maximum_line += TEXTBLOCK_SIZE;
	}
	// now set all of the invalidated blocks to unparsed
	for (size_t i = 0; i < invalidated_lines.size(); i++) {
		parsed_blocks[invalidated_lines[i] / TEXTBLOCK_SIZE].parsed = false;
	}
	this->current_task = new Task((PGThreadFunctionParams) RunHighlighter, (void*) this);
	Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
}

#include "logger.h"

void TextFile::LockBlock(ssize_t block) {
	std::string s = "Lock block " + std::to_string(block);
	Logger::GetInstance()->WriteLogMessage(s);
	LockMutex(block_locks[block]);
}

void TextFile::UnlockBlock(ssize_t block) {
	UnlockMutex(block_locks[block]);
}

void TextFile::OpenFile(std::string path) {
	this->path = path;
	this->base = (char*)mmap::ReadFile(path);

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
			current_line->length = (ptr - current_line->line) - offset;
			lines.push_back(TextLine(ptr + 1, 0));
			current_line = &lines.back();
			offset = 0;
		}
		ptr++;
	}
	if (lines.size() == 1) {
		lineending = GetSystemLineEnding();
	}
	current_line->length = (ptr - current_line->line) - offset;
}

std::string TextFile::GetText() {
	return std::string(base);
}

// FIXME: "file has been modified without us being the one that modified it"
// FIXME: use memory mapping on big files and only parse initial lines
TextLine* TextFile::GetLine(ssize_t linenumber) {
	// FIXME: account for deltas
	if (linenumber < 0 || linenumber >= (ssize_t)lines.size())
		return nullptr;
	return &lines[linenumber];
}

ssize_t TextFile::GetLineCount() {
	// FIXME: if file has not been entirely loaded (i.e. READONLY) then read 
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

void TextFile::InsertText(char character, std::vector<Cursor*>& cursors) {
	InsertText(std::string(1, character), cursors);
}

void TextFile::InsertText(std::string text, std::vector<Cursor*>& cursors) {
	// FIXME: merge delta if it already exists
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
		delta->AddDelta(new AddText(*it, (*it)->EndLine(), (*it)->EndCharacter(), text));
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

struct Interval {
	ssize_t start_line;
	ssize_t end_line;
	std::vector<Cursor*> cursors;
	Interval(ssize_t start, ssize_t end, Cursor* cursor) : start_line(start), end_line(end) { cursors.push_back(cursor); }
};

std::vector<Interval> TextFile::GetCursorIntervals(std::vector<Cursor*>& cursors) {
	std::vector<Interval> intervals;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		intervals.push_back(Interval((*it)->BeginLine(), (*it)->EndLine(), *it));
	}
	// if we get here we know the movement is possible
	// first merge the different intervals so each interval is "standalone" (i.e. not adjacent to another interval)
	for (ssize_t i = 0; i < (ssize_t)intervals.size(); i++) {
		Interval& interval = intervals[i];
		for (ssize_t j = i + 1; j < (ssize_t)intervals.size(); j++) {
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

void TextFile::MoveLines(std::vector<Cursor*>& cursors, int offset) {
	assert(offset == 1 || offset == -1); // we only support single line moves
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if ((*it)->start_line + offset < 0 || (*it)->start_line + offset >= (ssize_t)lines.size() ||
			(*it)->end_line + offset < 0 || (*it)->end_line + offset >= (ssize_t)lines.size()) {
			// we do not move any lines if any of the movements are not possible
			return;
		}
	}
	// we keep track of all the line intervals that we have to move up/down
	std::vector<Interval> intervals = GetCursorIntervals(cursors);
	MultipleDelta* delta = new MultipleDelta();
	// now each of the intervals are separate
	// we must perform a RemoveLine and AddLine operation for each interval
	// we remove the line before (or after) the interval, and add it back after (or before) the interval
	// before or after depends on the offset
	for (auto it = intervals.begin(); it != intervals.end(); it++) {
		SwapLines* swap = new SwapLines(it->start_line, offset, *GetLine(it->start_line));
		for (ssize_t j = it->start_line + 1; j <= it->end_line; j++) {
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
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::DeleteLines(std::vector<Cursor*>& cursors) {
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors contain a selection, delete only the selection
		DeleteCharacter(cursors, PGDirectionLeft);
		return;
	}
	// otherwise, delete each of the lines
	// first get the set of intervals covered by the cursors
	MultipleDelta* delta = new MultipleDelta();
	std::vector<Interval> intervals = GetCursorIntervals(cursors);
	for (auto it = intervals.begin(); it != intervals.end(); it++) {
		RemoveLines* remove = new RemoveLines(it->cursors.front(), it->start_line);
		for (ssize_t i = it->start_line; i <= it->end_line; i++) {
			remove->AddLine(lines[i]);
		}
		for (auto it2 = it->cursors.begin() + 1; it2 != it->cursors.end(); it2++) {
			delta->AddDelta(new CursorDelta(*it2, it->cursors.front()->start_line, 0));
		}
		delta->AddDelta(remove);
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::DeleteLine(std::vector<Cursor*>& cursors, PGDirection direction) {
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors contain a selection, delete only the selection
		DeleteCharacter(cursors, PGDirectionLeft);
		return;
	}
	MultipleDelta* delta = new MultipleDelta();
	bool delete_lines = true;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (!(((*it)->start_character == 0 && direction == PGDirectionLeft) ||
			((*it)->start_character == lines[(*it)->start_line].GetLength() && direction == PGDirectionRight))) {
			delete_lines = false;
			break;
		}
	}
	if (delete_lines) {
		DeleteCharacter(cursors, direction);
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
				delta->AddDelta(new RemoveText(*it, (*it)->start_line, lines[(*it)->start_line].GetLength(), lines[(*it)->start_line].GetLength() - (*it)->start_character));
			}
		} else {
			delta->AddDelta(new CursorDelta(*it, (*it)->start_line, (*it)->start_character));
		}
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::AddEmptyLine(std::vector<Cursor*>& cursors, PGDirection direction) {
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
		ssize_t line = (*it)->start_line;
		if (direction == PGDirectionLeft) {
			line--;
		}
		if (add_newline) {
			delta->AddDelta(new AddLine(*it, line, lines[line].GetLength(), ""));
		} else {
			delta->AddDelta(new CursorDelta(*it, line + 1, 0));
		}
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

std::string TextFile::CopyText(std::vector<Cursor*>& cursors) {
	std::string text = "";
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it != cursors.begin()) {
			text += NEWLINE_CHARACTER;
		}
		text += (*it)->GetText();
	}
	return text;
}

void TextFile::PasteText(std::vector<Cursor*>& cursors, std::string text) {
	std::vector<std::string> pasted_lines;
	ssize_t start = 0;
	ssize_t current = 0;
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
		InsertText(pasted_lines[0], cursors);
	} else {
		AddNewLines(cursors, pasted_lines, false);
	}
}


void
TextFile::DeleteCharacter(MultipleDelta* delta, Cursor* it, PGDirection direction, bool delete_selection, bool include_cursor) {
	if (delete_selection) {
		if (!it->SelectionIsEmpty()) {
			// delete with selection
			RemoveLines *remove_lines = new RemoveLines(nullptr, it->BeginLine() + 1);
			for (ssize_t i = it->BeginLine(); i <= it->EndLine(); i++) {
				if (i == it->BeginLine()) {
					if (i == it->EndLine()) {
						delete remove_lines;
						delta->AddDelta(new RemoveText(include_cursor ? &*it : nullptr, i, it->EndCharacter(), it->EndCharacter() - it->BeginCharacter()));
					} else {
						delta->AddDelta(new RemoveText(include_cursor ? &*it : nullptr, i, lines[i].GetLength(), lines[i].GetLength() - it->BeginCharacter()));
					}
				} else if (i == it->EndLine()) {
					assert(it->EndCharacter() <= lines[i].GetLength());
					// remove part of the last line
					std::string text = std::string(lines[i].GetLine() + it->EndCharacter(), lines[i].GetLength() - it->EndCharacter());
					remove_lines->AddLine(lines[i]);
					remove_lines->last_line_offset = it->EndCharacter();
					delta->AddDelta(remove_lines);
					delta->AddDelta(new AddText(nullptr, it->BeginLine(), it->BeginCharacter(), text));
				} else {
					// remove the entire line
					remove_lines->AddLine(lines[i]);
				}
			}
		} else {
			if (include_cursor) {
				delta->AddDelta(new CursorDelta(it, it->start_line, it->start_character));
			}
		}
	} else {
		ssize_t characternumber = it->SelectedCharacter();
		ssize_t linenumber = it->SelectedLine();
		if (direction == PGDirectionLeft && characternumber == 0) {
			if (linenumber > 0) {
				// merge the line with the previous line
				std::string line = std::string(lines[linenumber].GetLine(), lines[linenumber].GetLength());
				delta->AddDelta(new RemoveLine(include_cursor ? &*it : nullptr, linenumber, lines[linenumber]));
				TextDelta *addText = new AddText(nullptr, linenumber - 1, lines[linenumber - 1].GetLength(), line);
				delta->AddDelta(addText);
			}
		} else if (direction == PGDirectionRight && characternumber == lines[linenumber].GetLength()) {
			if (linenumber < GetLineCount() - 1) {
				// merge the next line with the current line
				std::string line = std::string(lines[linenumber + 1].GetLine(), lines[linenumber + 1].GetLength());
				delta->AddDelta(new RemoveLine(include_cursor ? &*it : nullptr, linenumber + 1, lines[linenumber + 1]));
				TextDelta *addText = new AddText(nullptr, linenumber, lines[linenumber].GetLength(), line);
				delta->AddDelta(addText);
			}
		} else {
			// FIXME: merge delta if it already exists
			RemoveText* remove = new RemoveText(include_cursor ? &*it : nullptr,
				linenumber,
				direction == PGDirectionRight ? characternumber + 1 : characternumber,
				1);
			delta->AddDelta(remove);
		}
	}
}
void
TextFile::DeleteCharacter(MultipleDelta* delta, std::vector<Cursor*>& cursors, PGDirection direction) {
	bool delete_selection = CursorsContainSelection(cursors);
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		DeleteCharacter(delta, *it, direction, delete_selection);
	}
}

void TextFile::DeleteCharacter(std::vector<Cursor*>& cursors, PGDirection direction) {
	MultipleDelta* delta = new MultipleDelta();
	DeleteCharacter(delta, cursors, direction);
	if (delta->deltas.size() == 0) {
		delete delta;
		return;
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::DeleteWord(std::vector<Cursor*>& cursors, PGDirection direction) {
	MultipleDelta* delta = new MultipleDelta();
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if ((*it)->SelectionIsEmpty() && (direction == PGDirectionLeft ? (*it)->SelectedCharacter() > 0 : (*it)->SelectedCharacter() < lines[(*it)->SelectedLine()].GetLength())) {
			int offset = direction == PGDirectionLeft ? -1 : 1;
			char* line = lines[(*it)->SelectedLine()].GetLine();
			ssize_t length = lines[(*it)->SelectedLine()].GetLength();
			ssize_t index = direction == PGDirectionLeft ? (*it)->SelectedCharacter() + offset : (*it)->SelectedCharacter();
			// a word is a series of characters of equal type (punctuation, spaces or everything else)
			// we scan the string to find out where the word ends 
			PGCharacterClass type = GetCharacterClass(line[index]);
			for (index += offset; index >= 0 && index < length; index += offset) {
				if (GetCharacterClass(line[index]) != type) {
					index -= direction == PGDirectionLeft ? offset : 0;
					break;
				}
			}
			index = std::min(std::max(index, (ssize_t)0), length);
			delta->AddDelta(new RemoveText(*it, (*it)->SelectedLine(), std::max(index, (*it)->SelectedCharacter()), std::abs((*it)->SelectedCharacter() - index)));
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
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::AddNewLine(std::vector<Cursor*>& cursors) {
	// FIXME: newline with multiple cursors on the same line
	AddNewLine(cursors, "");
}

void TextFile::AddNewLine(std::vector<Cursor*>& cursors, std::string text) {
	std::vector<std::string> lines;
	lines.push_back(text);
	AddNewLines(cursors, lines, true);
}

void TextFile::AddNewLines(std::vector<Cursor*>& cursors, std::vector<std::string>& added_text, bool first_is_newline) {
	MultipleDelta* delta = new MultipleDelta();
	if (CursorsContainSelection(cursors)) {
		// if any of the cursors select text, we delete that text before inserting the characters
		DeleteCharacter(delta, cursors, PGDirectionLeft);
		for (int i = 0; i < delta->deltas.size(); i++) {
			if (delta->deltas[i]->TextDeltaType() == PGDeltaAddText) {
				delta->deltas.erase(delta->deltas.begin() + i);
				i--;
			}
		}
	}
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		ssize_t characternumber = (*it)->EndCharacter();
		ssize_t linenumber = (*it)->EndLine();
		ssize_t length = lines[linenumber].GetLength();
		ssize_t current_line = (*it)->BeginLine();

		if (added_text.size() == 1) {
			delta->AddDelta(new AddLine(*it, current_line, (*it)->BeginCharacter(), added_text.front()));
			continue;
		}
		std::vector<std::string> text = added_text;
		if (!first_is_newline) {
			assert(added_text.size() > 1); // if added_text == 1 and first_is_newline is false, use InsertText
			delta->AddDelta(new AddText(nullptr, (*it)->BeginLine(), (*it)->BeginCharacter(), added_text.front()));
			text.erase(text.begin());
		}
		delta->AddDelta(new AddLines(*it, (*it)->BeginLine(), (*it)->BeginCharacter(), text));
	}
	this->AddDelta(delta);
	PerformOperation(delta);
	Cursor::NormalizeCursors(textfield, cursors);
}

void TextFile::Undo(std::vector<Cursor*>& cursors) {
	if (this->deltas.size() == 0) return;
	TextDelta* delta = this->deltas.back();
	textfield->ClearCursors(cursors);
	std::vector<ssize_t> invalidated_lines;
	this->Undo(delta, invalidated_lines, cursors);
	InvalidateParsing(invalidated_lines);
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	this->deltas.pop_back();
	this->redos.push_back(delta);
}

void TextFile::Redo(std::vector<Cursor*>& cursors) {
	if (this->redos.size() == 0) return;
	TextDelta* delta = this->redos.back();
	textfield->ClearCursors(cursors);
	this->Redo(delta, cursors);
	this->PerformOperation(delta, false);
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

static void ShiftDelta(ssize_t& linenr, ssize_t& characternr, ssize_t shift_lines, ssize_t shift_lines_line, ssize_t shift_lines_character, ssize_t shift_characters, ssize_t shift_characters_line, ssize_t shift_characters_character, ssize_t last_line_offset) {
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
	std::vector<ssize_t> invalidated_lines;
	PerformOperation(delta, invalidated_lines, adjust_delta);
	InvalidateParsing(invalidated_lines);
}

void TextFile::PerformOperation(TextDelta* delta, std::vector<ssize_t>& invalidated_lines, bool adjust_delta) {
	assert(delta);
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr].AddDelta(delta);
		invalidated_lines.push_back(add->linenr);
		if (add->cursor) {
			add->cursor->end_character = add->cursor->start_character = add->characternr + add->text.size();
			add->cursor->end_line = add->cursor->start_line = add->linenr;
		}
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr].AddDelta(delta);
		invalidated_lines.push_back(remove->linenr);
		if (remove->cursor) {
			remove->cursor->start_character = remove->cursor->end_character = (remove->characternr - remove->charactercount);
			remove->cursor->end_line = remove->cursor->start_line = remove->linenr;
		}
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		ssize_t length = lines[add->linenr].GetLength();
		add->extra_text = std::string(add->line);
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr].GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(nullptr, add->linenr, length, length - add->characternr);
			lines[add->linenr].AddDelta(add->remove_text);
			invalidated_lines.push_back(add->linenr);
		}
		lines.insert(lines.begin() + (add->linenr + 1), TextLine((char*)add->extra_text.c_str(), (ssize_t)add->extra_text.size()));
		invalidated_lines.push_back(add->linenr + 1);
		if (add->cursor) {
			add->cursor->end_line = add->cursor->start_line = add->linenr + 1;
			add->cursor->end_character = add->cursor->start_character = add->line.size();
		}
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		ssize_t length = lines[add->linenr].GetLength();
		ssize_t linenr = add->linenr + 1;
		for (auto it = add->lines.begin(); (it + 1) != add->lines.end(); it++) {
			lines.insert(lines.begin() + linenr, TextLine((char*)it->c_str(), (ssize_t)it->size()));
			invalidated_lines.push_back(linenr);
			linenr++;
		}
		add->extra_text = add->lines.back();
		if (add->characternr < length) {
			add->extra_text += std::string(lines[add->linenr].GetLine() + add->characternr, length - add->characternr);
			add->remove_text = new RemoveText(nullptr, add->linenr, length, length - add->characternr);
			lines[add->linenr].AddDelta(add->remove_text);
			invalidated_lines.push_back(add->linenr);
		}
		lines.insert(lines.begin() + linenr, TextLine((char*)add->extra_text.c_str(), (ssize_t)add->extra_text.size()));
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
			remove->cursor->end_character = remove->cursor->start_character = this->lines[remove->cursor->start_line].GetLength();
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

			ssize_t shift_lines = 0;
			ssize_t shift_lines_line = (*it)->linenr;
			ssize_t shift_lines_character = (*it)->characternr;
			ssize_t shift_characters = 0;
			ssize_t shift_characters_line = (*it)->linenr;
			ssize_t shift_characters_character = (*it)->characternr;
			ssize_t last_line_offset = 0;
			switch ((*it)->TextDeltaType()) {
			case PGDeltaRemoveLine:
				shift_lines = -1;
				break;
			case PGDeltaRemoveManyLines:
				shift_lines = -((ssize_t)((RemoveLines*)(*it))->lines.size());
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
				shift_characters = (ssize_t)((AddText*)(*it))->text.size();
				break;
			case PGDeltaRemoveText:
				shift_characters = -((ssize_t)((RemoveText*)(*it))->charactercount);
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
	}
}

void TextFile::Undo(TextDelta* delta, std::vector<ssize_t>& invalidated_lines, std::vector<Cursor*>& cursors) {
	assert(delta);
	CursorDelta* cursor_delta = dynamic_cast<CursorDelta*>(delta);
	if (cursor_delta && cursor_delta->cursor) {
		cursors.push_back(new Cursor(this));
		cursors.back()->RestoreCursor(cursor_delta->stored_cursor);
	}
	switch (delta->TextDeltaType()) {
	case PGDeltaAddText: {
		AddText* add = (AddText*)delta;
		this->lines[add->linenr].PopDelta();
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaRemoveText: {
		RemoveText* remove = (RemoveText*)delta;
		this->lines[remove->linenr].PopDelta();
		invalidated_lines.push_back(remove->linenr);
		break;
	}
	case PGDeltaAddLine: {
		AddLine* add = (AddLine*)delta;
		if (add->remove_text) {
			lines[add->linenr].RemoveDelta(add->remove_text);
			add->remove_text = nullptr;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1));
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaAddManyLines: {
		AddLines* add = (AddLines*)delta;
		if (add->remove_text) {
			lines[add->linenr].RemoveDelta(add->remove_text);
			add->remove_text = nullptr;
		}
		this->lines.erase(this->lines.begin() + (add->linenr + 1), this->lines.begin() + (add->linenr + 1 + add->lines.size()));
		invalidated_lines.push_back(add->linenr);
		break;
	}
	case PGDeltaRemoveLine: {
		RemoveLine* remove = (RemoveLine*)delta;
		this->lines.insert(this->lines.begin() + remove->linenr, remove->line);
		invalidated_lines.push_back(remove->linenr);
		break;
	}
	case PGDeltaRemoveManyLines: {
		RemoveLines* remove = (RemoveLines*)delta;
		ssize_t index = remove->linenr;
		for (auto it = remove->lines.begin(); it != remove->lines.end(); it++, index++) {
			this->lines.insert(this->lines.begin() + index, *it);
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
		for (ssize_t index = multi->deltas.size() - 1; index >= 0; index--) {
			Undo(multi->deltas[index], invalidated_lines, cursors);
		}
		break;
	}
	}
}

void TextFile::Redo(TextDelta* delta, std::vector<Cursor*>& cursors) {
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
			Redo(*it, cursors);
		}
		break;
	}
	}
}

void TextFile::SaveChanges() {
	PGLineEnding line_ending = lineending;
	if (line_ending != PGLineEndingWindows && line_ending != PGLineEndingMacOS && line_ending != PGLineEndingUnix) {
		line_ending = GetSystemLineEnding();
	}
	// FIXME: respect file encoding
	PGFileHandle handle = mmap::OpenFile(this->path, PGFileReadWrite);
	ssize_t position = 0;
	for (auto it = lines.begin(); it != lines.end(); it++) {
		ssize_t length = it->GetLength();
		char* line = it->GetLine();
		mmap::WriteToFile(handle, line, length);

		if (it + 1 != lines.end()) {
			switch (line_ending) {
			case PGLineEndingWindows:
				mmap::WriteToFile(handle, "\r\n", 2);
				break;
			case PGLineEndingMacOS:
				mmap::WriteToFile(handle, "\r", 1);
				break;
			case PGLineEndingUnix:
				mmap::WriteToFile(handle, "\n", 1);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	mmap::CloseFile(handle);
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

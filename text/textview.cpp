
#include "textview.h"
#include "wrappedtextiterator.h"
#include "basictextfield.h"

TextView::TextView(BasicTextField* textfield, std::shared_ptr<TextFile> file) :
	textfield(textfield), file(file), wordwrap(false),
	xoffset(0), yoffset(0, 0), wrap_width(0) {
	lock = std::unique_ptr<PGMutex>(CreateMutex());
}

void TextView::Initialize() {
	PGTextViewSettings settings;
	settings.cursor_data.push_back(PGCursorRange(0, 0, 0, 0));
	this->ApplySettings(settings);
	file->AddTextView(shared_from_this());
}

TextLineIterator* TextView::GetScrollIterator(BasicTextField* textfield, PGVerticalScroll scroll) {
	if (wordwrap) {
		return new WrappedTextLineIterator(this, textfield->GetTextfieldFont(), file.get(),
			scroll, wrap_width);
	}
	return new TextLineIterator(file.get(), scroll.linenumber);
}

TextLineIterator* TextView::GetLineIterator(BasicTextField* textfield, lng linenumber) {
	if (wordwrap) {
		return new WrappedTextLineIterator(this, textfield->GetTextfieldFont(), file.get(),
			PGVerticalScroll(linenumber, 0), wrap_width);
	}
	return new TextLineIterator(file.get(), linenumber);
}

double TextView::GetScrollPercentage(PGVerticalScroll scroll) {
	if (wordwrap) {
		auto buffer = file->GetBuffer(scroll.linenumber);

		double width = buffer->cumulative_width;
		for (lng i = buffer->start_line; i < scroll.linenumber; i++)
			width += buffer->line_lengths[i - buffer->start_line];

		TextLine textline = file->GetLine(scroll.linenumber);
		lng inner_lines = textline.RenderedLines(this, scroll.linenumber, textfield->GetTextfieldFont(), wrap_width);
		width += ((double)scroll.inner_line / (double)inner_lines) * buffer->line_lengths[scroll.linenumber - buffer->start_line];
		return file->GetTotalWidth() == 0 ? 0 : width / file->GetTotalWidth();
	} else {
		return file->GetLineCount() == 0 ? 0 : (double)scroll.linenumber / file->GetLineCount();
	}
}

double TextView::GetScrollPercentage() {
	return (GetScrollPercentage(GetLineOffset()));
}

PGVerticalScroll TextView::GetLineOffset() {
	assert(yoffset.linenumber >= 0 && yoffset.linenumber < file->GetLineCount());
	return yoffset;
}

void TextView::SetLineOffset(lng offset) {
	assert(offset >= 0 && offset < file->GetLineCount());
	yoffset.linenumber = offset;
	yoffset.inner_line = 0;
	yoffset.line_fraction = 0;
}

void TextView::SetLineOffset(PGVerticalScroll scroll) {
	assert(scroll.linenumber >= 0 && scroll.linenumber < file->GetLineCount());
	yoffset.linenumber = scroll.linenumber;
	yoffset.inner_line = scroll.inner_line;
}

void TextView::SetXOffset(lng offset) {
	this->xoffset = offset;
}


void TextView::SetScrollOffset(lng offset) {
	if (!wordwrap) {
		SetLineOffset(offset);
	} else {
		double percentage = (double)offset / (double)GetMaxYScroll();
		double width = percentage * file->GetTotalWidth();
		auto buffer = file->GetBufferFromWidth(width);
		double start_width = buffer->cumulative_width;
		lng line = 0;
		lng max_line = buffer->GetLineCount();
		while (line < max_line) {
			double next_width = start_width + buffer->line_lengths[line];
			if (next_width >= width) {
				break;
			}
			start_width = next_width;
			line++;
		}
		line += buffer->start_line;
		// find position within buffer
		TextLine textline = file->GetLine(line);
		lng inner_lines = textline.RenderedLines(this, line, textfield->GetTextfieldFont(), wrap_width);
		percentage = buffer->line_lengths[line - buffer->start_line] == 0 ? 0 : (width - start_width) / buffer->line_lengths[line - buffer->start_line];
		percentage = std::max(0.0, std::min(1.0, percentage));
		PGVerticalScroll scroll;
		scroll.linenumber = line;
		scroll.inner_line = (lng)(percentage * inner_lines);
		SetLineOffset(scroll);
	}
}

lng TextView::GetMaxYScroll() {
	if (!wordwrap) {
		return file->GetLineCount() - 1;
	} else {
		return std::max((lng)(file->GetTotalWidth() / wrap_width), file->GetLineCount() - 1);
	}
}

PGVerticalScroll TextView::GetVerticalScroll(lng linenumber, lng characternr) {
	if (!wordwrap) {
		return PGVerticalScroll(linenumber, 0);
	} else {
		PGVerticalScroll scroll = PGVerticalScroll(linenumber, 0);
		auto it = GetLineIterator(textfield, linenumber);
		for (;;) {
			it->NextLine();
			if (!it->GetLine().IsValid()) break;
			if (it->GetCurrentLineNumber() != linenumber) break;
			if (it->GetCurrentCharacterNumber() >= characternr) break;
			scroll.inner_line++;
		}
		return scroll;
	}
}

PGVerticalScroll TextView::OffsetVerticalScroll(PGVerticalScroll scroll, double offset) {
	lng lines_offset;
	return OffsetVerticalScroll(scroll, offset, lines_offset);
}

PGVerticalScroll TextView::OffsetVerticalScroll(PGVerticalScroll scroll, double offset, lng& lines_offset) {
	if (offset == 0) {
		lines_offset = 0;
		return scroll;
	}

	// first perform any fractional (less than one line) scrolling
	double partial = offset - (lng)offset;
	offset = (lng)offset;
	scroll.line_fraction += partial;
	if (scroll.line_fraction >= 1) {
		// the fraction is >= 1, we have to advance one actual line now
		lng floor = (lng)scroll.line_fraction;
		scroll.line_fraction -= floor;
		offset += floor;
	} else if (scroll.line_fraction < 0) {
		// the fraction is < 0, we have to go one line back now
		lng addition = (lng)std::ceil(std::abs(scroll.line_fraction));
		scroll.line_fraction += addition;
		offset -= addition;
		if (scroll.linenumber <= 0 && (!wordwrap || scroll.inner_line <= 0)) {
			// if we are at the first line in the file and we go backwards
			// we cannot actually go back one line from this position
			// so we avoid setting the line_fraction back to a value > 0
			scroll.line_fraction = 0;
		}
	}
	// line_fraction should always be a fraction
	assert(scroll.line_fraction >= 0 && scroll.line_fraction <= 1);
	if (!wordwrap) {
		// no wordwrap, simply add offset to the linenumber count
		lng original_linenumber = scroll.linenumber;
		scroll.linenumber += (lng)offset;
		lng max_y_scroll = GetMaxYScroll();
		scroll.linenumber = std::max(std::min(scroll.linenumber, max_y_scroll), (lng)0);
		if (scroll.linenumber >= max_y_scroll) {
			// we cannot have a fraction > 0 if we are at the last line of the file
			scroll.line_fraction = 0;
		}
		lines_offset = std::abs(scroll.linenumber - original_linenumber);
	} else {
		lines_offset = 0;
		lng lines = offset;
		TextLineIterator* it = GetScrollIterator(textfield, scroll);
		if (lines > 0) {
			// move forward by <lines>
			for (; lines != 0; lines--) {
				it->NextLine();
				if (!it->GetLine().IsValid()) {
					break;
				}
				lines_offset++;
			}
		} else {
			// move backward by <lines>
			for (; lines != 0; lines++) {
				it->PrevLine();
				if (!(it->GetLine().IsValid())) {
					break;
				}
				lines_offset++;
			}
		}
		double fraction = scroll.line_fraction;
		scroll = it->GetCurrentScrollOffset();
		it->NextLine();
		if (!it->GetLine().IsValid()) {
			// we cannot have a fraction > 0 if we are at the last line of the file
			fraction = 0;
		}
		scroll.line_fraction = fraction;
	}
	return scroll;
}

void TextView::OffsetLineOffset(double offset) {
	yoffset = OffsetVerticalScroll(yoffset, offset);
}

void TextView::SetCursorLocation(lng line, lng character) {
	LockMutex(lock.get());
	ClearExtraCursors();
	cursors[0].SetCursorLocation(line, character);
	UnlockMutex(lock.get());
	if (textfield) textfield->SelectionChanged();
}

void TextView::SetCursorLocation(lng start_line, lng start_character, lng end_line, lng end_character) {
	LockMutex(lock.get());
	ClearExtraCursors();
	cursors[0].SetCursorLocation(end_line, end_character);
	cursors[0].SetCursorStartLocation(start_line, start_character);
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::SetCursorLocation(PGTextRange range) {
	LockMutex(lock.get());
	ClearExtraCursors();
	cursors[0].SetCursorLocation(range);
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::AddNewCursor(lng line, lng character) {
	LockMutex(lock.get());
	cursors.push_back(Cursor(this, line, character));
	active_cursor = cursors.size() - 1;
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	Cursor::NormalizeCursors(this, cursors, false);
	UnlockMutex(lock.get());
}

void TextView::OffsetLine(lng offset) {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetLine(offset);
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetSelectionLine(lng offset) {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetSelectionLine(offset);
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetCharacter(PGDirection direction) {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it->SelectionIsEmpty()) {
			it->OffsetCharacter(direction);
		} else {
			auto pos = direction == PGDirectionLeft ? it->BeginCharacterPosition() : it->EndCharacterPosition();
			it->SetCursorLocation(pos.line, pos.character);
		}
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetSelectionCharacter(PGDirection direction) {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetSelectionCharacter(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetWord(PGDirection direction) {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetWord(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetSelectionWord(PGDirection direction) {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetSelectionWord(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetStartOfLine() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetStartOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::SelectStartOfLine() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectStartOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetStartOfFile() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetStartOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::SelectStartOfFile() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectStartOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetEndOfLine() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetEndOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::SelectEndOfLine() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectEndOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::OffsetEndOfFile() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetEndOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}

void TextView::SelectEndOfFile() {
	LockMutex(lock.get());
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectEndOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
	UnlockMutex(lock.get());
}


void TextView::RestoreCursors(std::vector<PGCursorRange>& data) {
	LockMutex(lock.get());
	ActuallyRestoreCursors(data);
	UnlockMutex(lock.get());
}

void TextView::ActuallyRestoreCursors(std::vector<PGCursorRange>& data) {
	ClearCursors();
	for (auto it = data.begin(); it != data.end(); it++) {
		cursors.push_back(Cursor(this, *it));
	}
	Cursor::NormalizeCursors(this, cursors);
}

Cursor TextView::RestoreCursor(PGCursorRange data) {
	return Cursor(this, data);
}

Cursor TextView::RestoreCursorPartial(PGCursorRange data) {
	if (data.start_line < data.end_line ||
		(data.start_line == data.end_line && data.start_position < data.end_position)) {
		data.end_line = data.start_line;
		data.end_position = data.start_position;
	} else {
		data.start_line = data.end_line;
		data.start_position = data.end_position;
	}
	return Cursor(this, data);
}

void TextView::ClearExtraCursors() {
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	active_cursor = 0;
}

void TextView::ClearCursors() {
	cursors.clear();
	active_cursor = -1;
}

lng TextView::GetActiveCursorIndex() {
	if (active_cursor < 0)
		active_cursor = 0;
	return active_cursor;
}

Cursor& TextView::GetActiveCursor() {
	if (active_cursor < 0)
		active_cursor = 0;
	return cursors[active_cursor];
}

void TextView::SelectEverything() {
	ClearCursors();
	this->cursors.push_back(Cursor(this, file->GetLineCount() - 1, file->GetLine(file->GetLineCount() - 1).GetLength(), 0, 0));
	if (this->textfield) textfield->SelectionChanged();
}

int TextView::GetLineHeight() {
	if (!textfield) return -1;
	return textfield->GetLineHeight();
}

PGTextViewSettings TextView::GetSettings() {
	PGTextViewSettings settings;
	settings.xoffset = this->xoffset;
	settings.yoffset = this->yoffset;
	settings.wordwrap = this->wordwrap;
	settings.cursor_data = Cursor::BackupCursors(cursors);
	return settings;
}

struct TextViewApplySettings {
	std::weak_ptr<TextView> view;
	PGTextViewSettings settings;
};

void TextView::ApplySettings(PGTextViewSettings& settings) {
	TextViewApplySettings* s = new TextViewApplySettings();
	s->settings = settings;
	s->view = std::weak_ptr<TextView>(shared_from_this());

	file->OnLoaded([](std::shared_ptr<TextFile> textfile, void* data) {
		TextViewApplySettings* s = (TextViewApplySettings*)data;
		auto view = s->view.lock();
		if (view) {
			view->ActuallyApplySettings(s->settings);
		}
	}, [](void* data) {
		TextViewApplySettings* s = (TextViewApplySettings*)data;
		delete s;
	}, s);
}

void TextView::ActuallyApplySettings(PGTextViewSettings& settings) {
	if (settings.xoffset >= 0) {
		this->xoffset = settings.xoffset;
		settings.xoffset = -1;
	}
	if (settings.yoffset.linenumber >= 0) {
		this->yoffset = settings.yoffset;
		this->yoffset.linenumber = std::min(file->GetLineCount() - 1, std::max((lng)0, this->yoffset.linenumber));
		settings.yoffset.linenumber = -1;
	}
	if (settings.wordwrap) {
		this->wordwrap = settings.wordwrap;
		this->wrap_width = -1;
		settings.wordwrap = false;
	}
	LockMutex(lock.get());
	if (settings.cursor_data.size() > 0) {
		this->ClearCursors();
		for (auto it = settings.cursor_data.begin(); it != settings.cursor_data.end(); it++) {
			it->start_line = std::max((lng)0, std::min(file->GetLineCount() - 1, it->start_line));
			it->end_line = std::max((lng)0, std::min(file->GetLineCount() - 1, it->end_line));
			this->cursors.push_back(Cursor(this, it->start_line, it->start_position, it->end_line, it->end_position));
		}
		if (cursors.size() == 0) {
			cursors.push_back(Cursor(this, 0, 0));
		}
		std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
		Cursor::NormalizeCursors(this, this->cursors, false);
	} else if (cursors.size() == 0) {
		cursors.push_back(Cursor(this, 0, 0));
	}
	UnlockMutex(lock.get());
}

bool TextView::IsLastFileView() {
	// FIXME:
	return true;
}

void TextView::InvalidateTextView(bool scroll) {
	yoffset.linenumber = std::min(file->GetLineCount() - 1, std::max((lng)0, yoffset.linenumber));

	matches.clear();
	line_wraps.clear();

	LockMutex(lock.get());
	Cursor::NormalizeCursors(this, cursors, scroll);
	UnlockMutex(lock.get());

	if (textfield) {
		textfield->TextChanged();
	}
}

void TextView::SetWordWrap(bool wordwrap, PGScalar wrap_width) {
	this->wordwrap = wordwrap;
	if (this->wordwrap && !panther::epsilon_equals(this->wrap_width, wrap_width)) {
		// heuristic to determine new scroll position in current line after resize
		this->yoffset.inner_line = (this->wrap_width / wrap_width) * this->yoffset.inner_line;
		this->wrap_width = wrap_width;
		this->xoffset = 0;
	} else if (!this->wordwrap) {
		this->wrap_width = -1;
	}
}

void TextView::VerifyTextView() {
#ifdef PANTHER_DEBUG
	assert(cursors.size() > 0);
	for (int i = 0; i < cursors.size(); i++) {
		Cursor& c = cursors[i];
		if (c.start_buffer == nullptr) continue;
		if (i < cursors.size() - 1) {
			if (cursors[i + 1].start_buffer == nullptr) continue;
			auto end_position = c.EndCursorPosition();
			auto begin_position = cursors[i + 1].BeginCursorPosition();
			if (!(end_position.buffer == begin_position.buffer && end_position.position == begin_position.position)) {
				assert(Cursor::CursorPositionOccursFirst(end_position.buffer, end_position.position,
					begin_position.buffer, begin_position.position));
			}
		}
		assert(c.start_buffer_position >= 0 && c.start_buffer_position < c.start_buffer->current_size);
		assert(c.end_buffer_position >= 0 && c.end_buffer_position < c.end_buffer->current_size);
		//assert(std::find(file->buffers.begin(), file->buffers.end(), c.start_buffer) != file->buffers.end());
		//assert(std::find(file->buffers.begin(), file->buffers.end(), c.end_buffer) != file->buffers.end());
	}
#endif
}

struct FindInformation {
	TextView* view;
	PGRegexHandle handle;
	lng start_line;
	lng start_character;
};

void TextView::RunTextFinder(std::shared_ptr<Task> task, TextView* view, PGRegexHandle regex_handle, lng current_line, lng current_character, bool select_first_match) {
	view->finished_search = false;
	view->matches.clear();

	if (!regex_handle) return;

	view->file->Lock(PGReadLock);
	bool found_initial_match = !select_first_match;
	PGTextBuffer* selection_buffer = view->file->GetBuffer(current_line);
	lng selection_position = selection_buffer->GetBufferLocationFromCursor(current_line, current_character);
	PGTextPosition position = PGTextPosition(selection_buffer, selection_position);


	PGTextRange bounds;
	bounds.start_buffer = view->file->GetFirstBuffer();
	bounds.start_position = 0;
	bounds.end_buffer = view->file->GetLastBuffer();
	bounds.end_position = bounds.end_buffer->current_size - 1;
	while (true) {
		/*if (textfile->find_task != task) {
			// a new find task has been activated
			// stop searching for an outdated find query
			PGDeleteRegex(regex_handle);
			return;
		}*/
		PGRegexMatch match = PGMatchRegex(regex_handle, bounds, PGDirectionRight);
		if (!match.matched) {
			break;
		}
		view->matches.push_back(match.groups[0]);

		if (!found_initial_match && match.groups[0].startpos() >= position) {
			found_initial_match = true;
			view->selected_match = view->matches.size() - 1;
			view->SetCursorLocation(match.groups[0]);
		}

		if (bounds.start_buffer == match.groups[0].end_buffer &&
			bounds.start_position == match.groups[0].end_position)
			break;
		bounds.start_buffer = match.groups[0].end_buffer;
		bounds.start_position = match.groups[0].end_position;


		/*if (textfile->find_task != task) {
			// a new find task has been activated
			// stop searching for an outdated find query
			PGDeleteRegex(regex_handle);
			return;
		}
		if (!start_buffer && !end_buffer) {
			textfile->finished_search = true;
			break;
		}*/
	}
	view->finished_search = true;
	view->file->Unlock(PGReadLock);
#ifdef PANTHER_DEBUG
	// FIXME: check if matches overlap?
	/*for (lng i = 0; i < textfile->matches.size() - 1; i++) {
	if (textfile->matches[i].start_line == textfile->matches[i].start_line) {

	}
	}*/
#endif
	if (regex_handle) PGDeleteRegex(regex_handle);
	if (view->textfield) {
		view->textfield->SearchMatchesChanged();
		view->textfield->Invalidate();
	}
}

void TextView::ClearMatches() {
	matches.clear();
	if (textfield) {
		textfield->SearchMatchesChanged();
		textfield->Invalidate();
	}
}

void TextView::FindAllMatches(PGRegexHandle handle, bool select_first_match, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap) {
	if (!file->IsLoaded()) return;
	if (!handle) {
		matches.clear();
		if (textfield) {
			textfield->SelectionChanged();
			textfield->SearchMatchesChanged();
		}
		return;
	}

	FindInformation* info = new FindInformation();
	info->view = this;
	info->handle = handle;
	info->start_line = start_line;
	info->start_character = start_character;

	RunTextFinder(nullptr, this, handle, start_line, start_character, select_first_match);
	/*

	this->find_task = new Task([](Task* t, void* in) {
	FindInformation* info = (FindInformation*)in;
	RunTextFinder(t, info->textfile, info->pattern, PGDirectionRight,
	info->end_line, info->end_character,
	info->start_line, info->start_character, info->match_case,
	true, info->regex);
	delete info;
	}, (void*)info);
	Scheduler::RegisterTask(this->find_task, PGTaskUrgent);*/
}

bool TextView::FindMatch(PGRegexHandle handle, PGDirection direction, bool wrap, bool include_selection) {
	if (!file->IsLoaded()) return false;
	PGTextRange match;
	if (selected_match >= 0) {
		// if FindAll has been performed, we should already have all the matches
		// simply select the next match, rather than searching again
		if (!finished_search) {
			// the search is still in progress: we might not have found the proper match to select
			if (direction == PGDirectionLeft && selected_match == 0) {
				// our FindPrev will wrap us to the end, however, there might be matches
				// that we still have not found, hence we have to wait
				while (!finished_search && selected_match == 0);
			} else if (direction == PGDirectionRight && selected_match == matches.size() - 1) {
				// our FindNext will wrap us back to the start
				// however, there might still be matches after us
				// hence we have to wait
				while (!finished_search && selected_match == matches.size() - 1);
			}
		}
		if (matches.size() == 0) {
			return false;
		}
		if (!include_selection) {
			if (direction == PGDirectionLeft) {
				selected_match = selected_match == 0 ? matches.size() - 1 : selected_match - 1;
			} else {
				selected_match = selected_match == matches.size() - 1 ? 0 : selected_match + 1;
			}
		}

		match = matches[selected_match];
		if (match.start_buffer != nullptr) {
			this->SetCursorLocation(match);
		}
		return true;
	}
	if (!handle) {
		matches.clear();
		if (textfield) textfield->SelectionChanged();
		return false;
	}
	// otherwise, search only for the next match in either direction
	file->Lock(PGReadLock);
	auto begin = GetActiveCursor().BeginCursorPosition();
	auto end = GetActiveCursor().EndCursorPosition();
	match = file->FindMatch(handle, direction,
		include_selection ? end.buffer : begin.buffer,
		include_selection ? end.position : begin.position,
		include_selection ? begin.buffer : end.buffer,
		include_selection ? begin.position : end.position, wrap, nullptr);
	file->Unlock(PGReadLock);

	if (match.start_buffer != nullptr) {
		SetCursorLocation(match);
	}
	matches.clear();
	if (match.start_buffer != nullptr) {
		matches.push_back(match);
	}
	if (textfield) {
		textfield->SearchMatchesChanged();
	}
	return match.start_buffer != nullptr;

	return false;
}

bool TextView::SelectMatches(bool in_selection) {
	if (matches.size() == 0) return false;

	LockMutex(lock.get());
	if (in_selection) {
		std::vector<Cursor> backup = cursors;
		std::vector<PGTextRange> selections;
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			selections.push_back(it->GetCursorSelection());
		}
		ClearCursors();
		for (auto it = matches.begin(); it != matches.end(); it++) {
			for (auto it2 = selections.begin(); it2 != selections.end(); it2++) {
				if (!(*it < *it2 || *it > *it2)) {
					cursors.push_back(Cursor(this, it->Invert()));
					break;
				}
			}
		}
		if (cursors.size() == 0) {
			cursors = backup;
			UnlockMutex(lock.get());
			return false;
		}
	} else {
		ClearCursors();
		for (auto it = matches.begin(); it != matches.end(); it++) {
			cursors.push_back(Cursor(this, it->Invert()));
		}
	}
	UnlockMutex(lock.get());
	matches.clear();
	active_cursor = 0;
	VerifyTextView();
	if (textfield) textfield->SelectionChanged();
	return true;
}

void TextView::InsertText(char character) {
	file->InsertText(cursors, character);
}

void TextView::InsertText(PGUTF8Character character) {
	file->InsertText(cursors, character);
}

void TextView::InsertText(std::string text) {
	file->InsertText(cursors, text);
}


void TextView::DeleteCharacter(PGDirection direction) {
	file->DeleteCharacter(cursors, direction);
}

void TextView::DeleteWord(PGDirection direction) {
	file->DeleteWord(cursors, direction);
}

void TextView::AddNewLine() {
	file->AddNewLine(cursors);
}

void TextView::AddNewLine(std::string text) {
	file->AddNewLine(cursors, text);
}

void TextView::DeleteLines() {
	file->DeleteLines(cursors);
}

void TextView::DeleteLine(PGDirection direction) {
	file->DeleteLine(cursors, direction);
}

void TextView::AddEmptyLine(PGDirection direction) {
	file->AddEmptyLine(cursors, direction);
}

void TextView::MoveLines(int offset) {
	file->MoveLines(cursors, offset);
}

std::string TextView::CutText() {
	return file->CutText(cursors);
}

std::string TextView::CopyText() {
	return file->CopyText(cursors);
}

void TextView::PasteText(std::string& text) {
	file->PasteText(cursors, text);
}

void TextView::RegexReplace(PGRegexHandle regex, std::string& replacement) {
	file->RegexReplace(cursors, regex, replacement);
}

void TextView::IndentText(PGDirection direction) {
	file->IndentText(cursors, direction);
}

void TextView::Undo() {
	file->Undo(this);
}

void TextView::Redo() {
	file->Redo(this);
}

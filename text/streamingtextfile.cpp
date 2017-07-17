
#include "streamingtextfile.h"

StreamingTextFile::StreamingTextFile() {

}

StreamingTextFile::StreamingTextFile(std::string filename) :
	TextFile(filename) {

}

StreamingTextFile::~StreamingTextFile() {

}

TextLine StreamingTextFile::GetLine(lng linenumber) {
	return TextLine();
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
	assert(0);
	return 0;
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
	assert(0);
	return -1;
}

TextFile::PGStoreFileType StreamingTextFile::WorkspaceFileStorage() {
	return PGFileTooLarge;
}

PGTextRange StreamingTextFile::FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap) {
	assert(0);
	return PGTextRange();
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
	assert(0);
	return nullptr;
}

PGTextBuffer* StreamingTextFile::GetBufferFromWidth(double width) {
	assert(0);
	return nullptr;
}

PGTextBuffer* StreamingTextFile::GetFirstBuffer() {
	assert(0);
	return nullptr;
}

PGTextBuffer* StreamingTextFile::GetLastBuffer() {
	assert(0);
	return nullptr;
}

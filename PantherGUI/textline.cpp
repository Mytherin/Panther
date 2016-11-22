
#include "textline.h"
#include "textdelta.h"


ssize_t TextLine::GetLength(void) {
	if (!deltas) return length; // no deltas, return original length
	if (modified_line) return strlen(modified_line); // deltas have already been computed
	ApplyDeltas();
	return strlen(modified_line);
}

char* TextLine::GetLine(void) {
	if (!deltas) return line; // no deltas, return original line
	if (modified_line) return modified_line; // deltas have already been computed
	ApplyDeltas();
	return modified_line;
}

void TextLine::ApplyDeltas() {
	TextDelta* delta = this->deltas;
	ssize_t length = this->length;
	while (delta) {
		if (delta->TextDeltaType() == PGDeltaAddText) {
			length += ((AddText*)delta)->text.size();
		}
		else if (delta->TextDeltaType() == PGDeltaRemoveText) {
			//length -= ((RemoveText*)delta)->charactercount;
		}
		delta = delta->next;
	}
	modified_line = (char*) malloc(length * sizeof(char) + 1);
	if (!modified_line) {
		// FIXME
		assert(0);
	}

	ssize_t current_length = this->length;
	memcpy(modified_line, line, this->length);
	delta = this->deltas;
	while (delta) {
		if (delta->TextDeltaType() == PGDeltaAddText) {
			AddText* add = (AddText*) delta;
			if (current_length - add->characternr + add->text.size() > 0) {			
				memmove(modified_line + add->characternr + add->text.size(), 
					modified_line + add->characternr, 
					current_length + 1 - (add->characternr + add->text.size()));
			}
			memcpy(modified_line + add->characternr, add->text.c_str(), add->text.size());
			current_length += add->text.size();
		}
		else if (delta->TextDeltaType() == PGDeltaRemoveText) {
			RemoveText* remove = (RemoveText*) delta;
			memmove(modified_line + remove->characternr - remove->charactercount, modified_line + remove->characternr, current_length - remove->characternr);
			current_length -= remove->charactercount;
		}
		delta = delta->next;
	}
	modified_line[current_length] = '\0';
}

void TextLine::AddDelta(TextDelta* delta) {
	// check for invalid delta
	assert(delta->TextDeltaType() == PGDeltaRemoveText || delta->TextDeltaType() == PGDeltaAddText);
	if (this->deltas == NULL) {
		this->deltas = delta;
	} else {
		TextDelta* current = this->deltas;
		while (current->next) current = current->next;
		assert(current != delta);
		current->next = delta;
	}
	if (modified_line) free(modified_line);
	modified_line = NULL;
}

void TextLine::PopDelta() {
	TextDelta *linedelta = this->deltas;
	TextDelta *prev = NULL;
	assert(linedelta);
	while (linedelta->next) {
		prev = linedelta;
		linedelta = linedelta->next;
	}
	if (!prev)
		this->deltas = NULL;
	else
		prev->next = NULL;

	if (this->modified_line) free(this->modified_line);
	this->modified_line = NULL;
}

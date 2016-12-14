
#include "textdelta.h"
#include "textfile.h"
#include <algorithm>

TextLine::TextLine(const TextLine& other) {
	this->line = other.line;
	this->applied_deltas = other.applied_deltas;
	syntax.end = -1;
	syntax.next = nullptr;
}

TextLine::~TextLine() {
	PGSyntax* next = syntax.next;
	syntax.next = nullptr;
	while (next) {
		PGSyntax* tmp = next->next;
		delete next;
		next = tmp;
	}
}

lng TextLine::GetLength(void) {
	return line.size();
}

char* TextLine::GetLine(void) {
	return (char*)line.c_str();
}

void TextLine::AddDelta(TextDelta* delta) {
	// check for invalid delta
	assert(delta->TextDeltaType() == PGDeltaRemoveText || delta->TextDeltaType() == PGDeltaAddText);
	TextDelta* next;
	while (delta) {
		assert(delta->TextDeltaType() == PGDeltaRemoveText || delta->TextDeltaType() == PGDeltaAddText);
		// apply the delta to the text line
		if (delta->TextDeltaType() == PGDeltaAddText) {
			AddText* add = (AddText*) delta;
			line.insert(add->characternr, add->text.c_str(), add->text.size());
		}
		else if (delta->TextDeltaType() == PGDeltaRemoveText) {
			RemoveText* remove = (RemoveText*) delta;
			remove->removed_text = line.substr(remove->characternr - remove->charactercount, remove->charactercount);
			line.erase(remove->characternr - remove->charactercount, remove->charactercount);
		}
		next = delta->next;
		// add the delta to the set of applied deltas
		delta->next = this->applied_deltas;
		this->applied_deltas = delta;
		delta = next;
	}
}

void TextLine::RemoveDelta(TextDelta* delta) {
	// we only support removing the most recently applied delta
	assert(this->applied_deltas == delta);
	TextDelta* current = this->applied_deltas;
	this->applied_deltas = this->applied_deltas->next;
	if (current) {
		UndoDelta(current);
		delete current;
		return;
	}
	// attempt to delete a delta that is not part of the current line
	assert(0);
}

TextDelta* TextLine::PopDelta() {
	// undo the last applied delta
	TextDelta* delta = this->applied_deltas;

	if (!delta) return nullptr;

	this->applied_deltas = delta->next;
	UndoDelta(delta);
	return delta;
}

void TextLine::UndoDelta(TextDelta* delta) {
	delta->next = nullptr;
	// this function should only be called with Add/Remove deltas
	assert(delta->TextDeltaType() == PGDeltaRemoveText || delta->TextDeltaType() == PGDeltaAddText);
	// the delta should be applied and be the last applied delta
	if (delta->TextDeltaType() == PGDeltaRemoveText) {
		RemoveText* remove = (RemoveText*) delta;
		//remove->characternr - remove->charactercount
		line.insert(remove->characternr - remove->charactercount, remove->removed_text.c_str(), remove->removed_text.size());
	}
	else if (delta->TextDeltaType() == PGDeltaAddText) {
		AddText* add = (AddText*) delta;
		line.erase(add->characternr, add->text.size());
	}
}

#include "textdelta.h"
#include "textfile.h"
#include <algorithm>

TextLine::TextLine(const TextLine& other) {
	this->line = other.line;
	this->deltas =  other.deltas;
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
	if (!deltas) return line.size(); // no deltas, return original length
	ApplyDeltas();
	return line.size();
}

char* TextLine::GetLine(void) {
	if (!deltas) return (char*)line.c_str(); // no deltas, return original line
	ApplyDeltas();
	return (char*)line.c_str();
}

void TextLine::ApplyDeltas() {
	TextDelta* delta = this->deltas;
	TextDelta* next;
	this->deltas = nullptr;
	while (delta) {
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

void TextLine::AddDelta(TextDelta* delta) {
	// check for invalid delta
	assert(delta->TextDeltaType() == PGDeltaRemoveText || delta->TextDeltaType() == PGDeltaAddText);
	if (this->deltas == nullptr) {
		this->deltas = delta;
		delta->next = nullptr;
	} else {
		TextDelta* current = this->deltas;
		while (current->next) current = current->next;
		assert(current != delta);
		current->next = delta;
	}
}

void TextLine::RemoveDelta(TextDelta* delta) {
	TextDelta* current = this->deltas;
	TextDelta* prev = nullptr;
	do {
		if (current == delta) {
			if (!prev) {
				this->deltas = current->next;
			} else {
				prev->next = current->next;
			}
			delete current;
			return;
		}
	} while ((prev = current) && (current = current->next) != nullptr);
	// the delta has already been applied, reverse it
	assert(this->applied_deltas == delta);
	current = this->applied_deltas;
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
	TextDelta *delta = this->deltas;
	TextDelta *prev = nullptr;
	if (delta) {
		// there are unapplied deltas, remove the last unapplied delta
		while (delta->next) {
			prev = delta;
			delta = delta->next;
		}
		if (!prev)
			this->deltas = nullptr;
		else
			prev->next = nullptr;
		return delta;
	} else {
		// otherwise, undo the last applied delta
		delta = this->applied_deltas;

		if (!delta) return nullptr;

		this->applied_deltas = delta->next;
		UndoDelta(delta);
		return delta;
	}
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
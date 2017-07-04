#pragma once

#include "textbuffer.h"
#include "textline.h"
#include "textposition.h"

class TextLineIterator {
	friend class TextFile;
	friend class TextView;
public:
	virtual TextLine GetLine();
	friend TextLineIterator& operator++(TextLineIterator& element) {
		element.NextLine();
		return element;
	}
	friend TextLineIterator& operator--(TextLineIterator& element) {
		element.PrevLine();
		return element;
	}
	friend TextLineIterator& operator++(TextLineIterator& element, int i) {
		return ++element;
	}
	friend TextLineIterator& operator--(TextLineIterator& element, int i) {
		return --element;
	}

	virtual lng GetCurrentLineNumber() { return current_line; }
	virtual lng GetCurrentCharacterNumber() { return 0; }
	virtual lng GetInnerLine() { return 0; }
	virtual PGVerticalScroll GetCurrentScrollOffset() { return PGVerticalScroll(current_line, 0); }
	virtual PGTextRange GetCurrentRange() { return PGTextRange(buffer, start_position, buffer, end_position); }

	PGTextBuffer* CurrentBuffer() { return buffer; }
	TextLineIterator(PGTextBuffer* buffer);
protected:
	TextLineIterator(TextFile* textfile, lng current_line);
	TextLineIterator();

	void Initialize(TextFile* tf, lng current_line);

	virtual void PrevLine();
	virtual void NextLine();

	PGTextBuffer* buffer;
	lng start_position = 0, end_position;
	lng current_line;
	TextLine textline;
};

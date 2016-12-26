#pragma once

#include "control.h"
#include "textfile.h"

// The basic textfield class is the base class for all textfields
// it implements some base functionality of textfields, including
// caret flicker, mouse clicking functionality and font handling.

// Note that it is not a complete implementation (the Draw method
// is not implemented). As such, the basic textfield class should
// not be used directly, instead one of the variants that inherits
// from the basic textfield should be used.

enum PGDragType {
	PGDragNone,
	PGDragSelection,
	PGDragSelectionCursors,
	PGDragScrollbar,
	PGDragHorizontalScrollbar,
	PGDragMinimap,
	PGDragHoldScrollArrow,
	PGDragPopupMenu
};

class BasicTextField;

typedef void(*PGTextFieldCallback)(BasicTextField* textfield);

class BasicTextField : public Control {
public:
	BasicTextField(PGWindowHandle, TextFile* textfile);
	~BasicTextField();

	virtual void PeriodicRender(void);
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);

	virtual bool ControlTakesFocus() { return true; }

	virtual PGCursorType GetCursor(PGPoint mouse);

	TextFile& GetTextFile() { return *textfile; }

	bool IsDragging() { return drag_type != PGDragNone; }

	int GetLineHeight();

	void RefreshCursors();

	PGFontHandle GetTextfieldFont() { return textfield_font; }

	virtual PGScalar GetTextfieldWidth() { return this->width; }
	virtual PGScalar GetTextfieldHeight() { return this->height; }
	virtual PGScalar GetMaxXOffset();

	virtual void SelectionChanged();
	virtual void TextChanged();
	virtual void TextChanged(std::vector<TextLine*> lines);

	void OnSelectionChanged(PGControlDataCallback callback, void* data);
	void OnTextChanged(PGControlDataCallback callback, void* data);
protected:
	TextFile* textfile = nullptr;
	PGFontHandle textfield_font = nullptr;

	int display_carets_count = 0;
	bool display_carets = true;

	bool current_focus = true;

	PGScalar text_offset = 0;

	PGDragType drag_type = PGDragNone;
	MouseClickInstance last_click;

	void GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character);
	void GetLineFromPosition(PGScalar y, lng& line);
	void GetCharacterFromPosition(PGScalar x, TextLine* line, lng& character);

	void PerformMouseClick(PGPoint mouse);

	std::vector<std::pair<PGControlDataCallback, void*>> selection_changed_callbacks;
	std::vector<std::pair<PGControlDataCallback, void*>> text_changed_callbacks;
};
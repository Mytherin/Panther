#pragma once

#include "control.h"
#include "textfile.h"
#include "textview.h"

#define FLICKER_CARET_INTERVAL 15

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
	BasicTextField(PGWindowHandle, std::shared_ptr<TextFile> textfile);
	~BasicTextField();

	virtual void Update(void);
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);

	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	void StartDragging(PGMouseButton button, PGDragType drag_type);
	void ClearDragging();

	virtual bool ControlTakesFocus() { return true; }
	
	virtual PGCursorType GetCursor(PGPoint mouse);

	std::shared_ptr<TextView> GetTextView() { return textview; }

	virtual bool IsDragging() { return drag_type != PGDragNone; }
	PGCursorType GetDraggingCursor() { return drag_type == PGDragSelection ? PGCursorIBeam : PGCursorStandard; }

	int GetLineHeight();

	void RefreshCursors();

	PGFontHandle GetTextfieldFont() { return textfield_font; }

	virtual PGScalar GetTextfieldWidth() { return this->width; }
	virtual PGScalar GetTextfieldHeight() { return this->height; }
	virtual PGScalar GetMaxXOffset();

	virtual void SelectionChanged();
	virtual void TextChanged();
	virtual void SearchMatchesChanged() { }

	void PasteHistory();

	// invalidate only the textfield (not the whole control); used to avoid unnecessary redrawing for caret flickering
	virtual void InvalidateTextField();

	void SetFont(PGFontHandle font) { textfield_font = font; }

	PG_CONTROL_KEYBINDINGS;

	virtual PGControlType GetControlType() { return PGControlTypeBasicTextField; }
protected:
	std::shared_ptr<TextView> textview = nullptr;
	PGFontHandle textfield_font = nullptr;

	int display_carets_count = 0;
	bool display_carets = true;
	
	bool prev_loaded = false;
	bool current_focus = true;

	bool support_multiple_lines = false;

	PGScalar text_offset = 0;

	std::map<lng, PGTextRange> minimal_selections;
	PGMouseButton drag_button = PGButtonNone;
	PGDragType drag_type = PGDragNone;

	virtual void GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character);
	virtual void GetLineFromPosition(PGScalar y, lng& line);
	virtual void _GetCharacterFromPosition(PGScalar x, TextLine line, lng& character);

	virtual void GetPositionFromLineCharacter(lng line, lng character, PGScalar& x, PGScalar& y);
	virtual void GetPositionFromLine(lng line, PGScalar& y);
	virtual void _GetPositionFromCharacter(lng pos, TextLine line, PGScalar& x);
};
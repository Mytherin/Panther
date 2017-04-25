#pragma once

#include "control.h"
#include "basictextfield.h"
#include "textfile.h"
#include "textfield.h"

// The simple textfield class is for small, typically temporary textfields
// such as the ones used for searching, or goto linenumbers, etc...
// It only supports a single line, and is backed by an in-memory textfile
// Simple textfields do not have scrollbars or minimaps, and can only scroll
// in the X dimension.


class SimpleTextField : public BasicTextField {
public:
	SimpleTextField(PGWindowHandle, bool multiline = false);
	~SimpleTextField();

	void Draw(PGRendererHandle);
	bool KeyboardButton(PGButton button, PGModifier modifier);

	void SetValidInput(bool valid);
	void SetRenderBackground(bool render) { render_background = render; }

	void OnCancel(PGControlDataCallback callback, void* data) { on_user_cancel.function = callback; on_user_cancel.data = data; }
	void OnConfirm(PGControlDataCallback callback, void* data) { on_user_confirm.function = callback; on_user_confirm.data = data; }
	void OnPrevEntry(PGControlDataCallback callback, void* data) { on_prev_entry.function = callback; on_prev_entry.data = data; }
	void OnNextEntry(PGControlDataCallback callback, void* data) { on_next_entry.function = callback; on_next_entry.data = data; }
	void OnSelectionChanged(PGControlDataCallback callback, void* data) { on_selection_changed.function = callback; on_selection_changed.data = data; }
	void OnTextChanged(PGControlDataCallback callback, void* data) { on_text_changed.function = callback; on_text_changed.data = data; }

	void OnResize(PGSize old_size, PGSize new_size);

	virtual void SelectionChanged();
	virtual void TextChanged();

	void LosesFocus(void);
	void GainsFocus(void);

	std::string GetText();
	void SetText(std::string text);

	PG_CONTROL_KEYBINDINGS;


	virtual PGControlType GetControlType() { return PGControlTypeSimpleTextField; }
protected:
	bool valid_input = true;
	bool render_background = true;
	
	PGFunctionData on_user_cancel;
	PGFunctionData on_user_confirm;
	PGFunctionData on_prev_entry;
	PGFunctionData on_next_entry;
	PGFunctionData on_text_changed;
	PGFunctionData on_selection_changed;
};
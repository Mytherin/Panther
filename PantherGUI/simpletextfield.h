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
	SimpleTextField(PGWindowHandle);
	~SimpleTextField();

	void Draw(PGRendererHandle, PGIRect*);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void SetValidInput(bool valid);

	void OnCancel(PGControlDataCallback callback, void* data) { on_user_cancel.function = callback; on_user_cancel.data = data; }
	void OnConfirm(PGControlDataCallback callback, void* data) { on_user_confirm.function = callback; on_user_confirm.data = data; }
	void OnPrevEntry(PGControlDataCallback callback, void* data) { on_prev_entry.function = callback; on_prev_entry.data = data; }
	void OnNextEntry(PGControlDataCallback callback, void* data) { on_next_entry.function = callback; on_next_entry.data = data; }

	std::string GetText();
	void SetText(std::string text);

	PG_CONTROL_KEYBINDINGS;
protected:
	bool valid_input = true;

	struct PGFunctionData {
		PGControlDataCallback function = nullptr;
		void* data = nullptr;

		PGFunctionData() : function(nullptr), data(nullptr) { }
	};

	PGFunctionData on_user_cancel;
	PGFunctionData on_user_confirm;
	PGFunctionData on_prev_entry;
	PGFunctionData on_next_entry;
};
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
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void SetValidInput(bool valid) { valid_input = valid; }
private:
	bool valid_input = true;
};
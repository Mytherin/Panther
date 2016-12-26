#pragma once

// A composite control that contains both a TextField and a TabControl
// This control exists because both the TabControl and TextField can be controlled at
// the same time using keys (e.g. Ctrl+Tab to move tabs while typing in a textfield)

#include "control.h"
#include "textfield.h"
#include "tabcontrol.h"
#include "windowfunctions.h"

#define TEXT_TAB_HEIGHT 20

class PGContainer : public Control {
public:
	PGContainer(PGWindowHandle window);
	virtual ~PGContainer();

	void MouseWheel(int x, int y, int distance, PGModifier modifier);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);
	bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGIRect*);

	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	void OnResize(PGSize old_size, PGSize new_size);

	Control* GetMouseOverControl(int x, int y);
	Control* GetActiveControl() { return focused_control; }

	PGCursorType GetCursor(PGPoint mouse);
	bool IsDragging();

	void AddControl(Control* control);
	void RemoveControl(Control* control);
protected:
	Control* focused_control = nullptr;

	std::vector<Control*> controls;
};
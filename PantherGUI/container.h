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

	virtual void MouseWheel(int x, int y, int distance, PGModifier modifier);
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	virtual void PeriodicRender(void);
	virtual void Draw(PGRendererHandle, PGIRect*);

	virtual void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	virtual void OnResize(PGSize old_size, PGSize new_size);

	virtual bool ControlTakesFocus() { return true; }

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
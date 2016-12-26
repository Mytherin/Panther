#pragma once

#include "control.h"
#include "textfield.h"

typedef void(*PGMouseCallback)(Control* control, bool, void*);

struct PGMouseRegion {
	bool mouse_inside = false;
	PGIRect* rect = nullptr;
	void* data = nullptr;
	Control* control;
	PGMouseCallback mouse_event;

	PGMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr) : rect(rect), control(control), mouse_event(mouse_event), data(data) { }
};

class ControlManager : public Control {
public:
	ControlManager(PGWindowHandle window);

	void AddControl(Control* control);

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

	Control* GetMouseOverControl(int x, int y);

	void RefreshWindow();
	void RefreshWindow(PGIRect rectangle);

	void OnResize(PGSize old_size, PGSize new_size);

	Control* GetActiveControl() { return focused_control; }

	PGCursorType GetCursor(PGPoint mouse);

	void RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr);
	void UnregisterMouseRegion(PGIRect* rect);
private:
	PGIRect invalidated_area;
	bool invalidated;
	std::vector<Control*> controls;
	Control* focused_control;

	std::vector<PGMouseRegion> regions;
};
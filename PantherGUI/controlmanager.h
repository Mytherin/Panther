#pragma once

#include "container.h"
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

class ControlManager : public PGContainer {
public:
	ControlManager(PGWindowHandle window);

	void PeriodicRender(void);

	void RefreshWindow();
	void RefreshWindow(PGIRect rectangle);

	Control* GetActiveControl() { return focused_control; }
	void RegisterControlForMouseEvents(Control* control);
	void UnregisterControlForMouseEvents(Control* control);

	void RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr);
	void UnregisterMouseRegion(PGIRect* rect);
private:
	PGIRect invalidated_area;
	bool invalidated;

	std::vector<PGMouseRegion> regions;
};
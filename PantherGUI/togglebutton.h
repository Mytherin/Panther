#pragma once

#include "button.h"

typedef void(*PGToggleCallback)(Button*, bool toggled);

class ToggleButton : public Button {
public:
	ToggleButton(PGWindowHandle window, Control* parent, bool toggled);

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnToggle(PGToggleCallback callback) { on_toggle = callback; }
	void Toggle();
	bool IsToggled() { return toggled; }
private:
	PGColor untoggled_color;
	PGColor toggled_color;

	PGToggleCallback on_toggle = nullptr;

	bool toggled;
};
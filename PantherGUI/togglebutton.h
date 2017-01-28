#pragma once

#include "button.h"

typedef void(*PGToggleCallback)(Button*, bool toggled, void* data);

class ToggleButton : public Button {
public:
	ToggleButton(PGWindowHandle window, Control* parent, bool toggled);

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void OnToggle(PGToggleCallback callback, void* data = nullptr) { on_toggle = callback; toggle_data = data; }
	void Toggle();
	bool IsToggled() { return toggled; }
private:
	PGColor untoggled_color;
	PGColor toggled_color;

	PGToggleCallback on_toggle = nullptr;
	void* toggle_data = nullptr;

	bool toggled;
};
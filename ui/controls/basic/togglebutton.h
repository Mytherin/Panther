#pragma once

#include "button.h"

typedef void(*PGToggleCallback)(Button*, bool toggled, void* data);

class ToggleButton : public Button {
public:
	ToggleButton(PGWindowHandle window, Control* parent, bool toggled);

	void Draw(PGRendererHandle renderer);

	void OnToggle(PGToggleCallback callback, void* data = nullptr) { on_toggle = callback; toggle_data = data; }
	void Toggle();
	bool IsToggled() { return toggled; }
	// sets toggled property without triggering the OnToggle events
	void SetToggled(bool toggled) { this->toggled = toggled; this->Invalidate(); }

	static ToggleButton* CreateFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
		std::map<std::string, PGKeyFunction>& functions, PGFontHandle font, std::string text, bool initial_toggle);
	static ToggleButton* CreateFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
		std::map<std::string, PGKeyFunction>& functions, std::map<std::string, PGBitmapHandle>& images, bool initial_toggle);
private:
	PGColor untoggled_color;
	PGColor toggled_color;

	PGToggleCallback on_toggle = nullptr;
	void* toggle_data = nullptr;

	bool toggled;
};
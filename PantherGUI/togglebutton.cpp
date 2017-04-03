
#include "style.h"
#include "togglebutton.h"

ToggleButton::ToggleButton(PGWindowHandle window, Control* parent, bool toggled) : 
	Button(window, parent), toggled(toggled), on_toggle(nullptr) {

	this->OnPressed([](Button* button, void* data) {
		ToggleButton* toggle = dynamic_cast<ToggleButton*>(button);
		toggle->Toggle();
	});

	untoggled_color = PGStyleManager::GetColor(PGColorTabControlBackground);
	toggled_color = PGStyleManager::GetColor(PGColorToggleButtonToggled);
}

void ToggleButton::Draw(PGRendererHandle renderer) {
	background_color = toggled ? toggled_color : untoggled_color;
	Button::Draw(renderer);
}

void ToggleButton::Toggle() {
	toggled = !toggled;
	if (on_toggle) on_toggle(this, toggled, toggle_data);
	this->Invalidate();
}

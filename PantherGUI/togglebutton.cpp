
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

#include "toolbar.h"

static ToggleButton* CreateToggleButtonFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
	std::map<std::string, PGKeyFunction>& functions, bool initial_toggle) {
	assert(functions.count(command_name) > 0);
	ToggleButton* button = new ToggleButton(parent->window, parent, initial_toggle);
	if (!button) {
		return nullptr;
	}
	button->padding = PGPadding(4, 4, 4, 4);
	button->fixed_width = TOOLBAR_HEIGHT - button->padding.left - button->padding.right;
	button->fixed_height = TOOLBAR_HEIGHT - button->padding.top - button->padding.bottom;
	button->background_color = PGColor(0, 0, 0, 0);
	button->background_stroke_color = PGColor(0, 0, 0, 0);
	button->background_color_hover = PGStyleManager::GetColor(PGColorTextFieldSelection);
	button->SetTooltip(tooltip_text);
	button->OnToggle([](Button* b, bool toggled, void* data) {
		((PGKeyFunction)data)(b->parent);
	}, functions[command_name]);
	return button;
}

ToggleButton* ToggleButton::CreateFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
	std::map<std::string, PGKeyFunction>& functions, PGFontHandle font, std::string text, bool initial_toggle) {
	ToggleButton* button = CreateToggleButtonFromCommand(parent, command_name, tooltip_text, functions, initial_toggle);
	if (!button) {
		return nullptr;
	}
	button->SetText(text, font);
	return button;
}

ToggleButton* ToggleButton::CreateFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
	std::map<std::string, PGKeyFunction>& functions, std::map<std::string, PGBitmapHandle>& images, bool initial_toggle) {
	assert(images.count(command_name) > 0);

	ToggleButton* button = CreateToggleButtonFromCommand(parent, command_name, tooltip_text, functions, initial_toggle);
	if (!button) {
		return nullptr;
	}
	button->SetImage(images[command_name]);
	return button;
}



#include "button.h"
#include "controlmanager.h"
#include "style.h"


Button::Button(PGWindowHandle window, std::shared_ptr<Control> parent) : 
	Control(window), fixed_size(true), image(nullptr) {
	this->parent = std::weak_ptr<Control>(parent);
	GetControlManager(this)->RegisterControlForMouseEvents(this);

	this->background_color = PGStyleManager::GetColor(PGColorTabControlBackground);
	this->background_color_hover = PGStyleManager::GetColor(PGColorTabControlHover);
	this->background_color_click = PGStyleManager::GetColor(PGColorTabControlSelected);
	this->background_stroke_color = PGStyleManager::GetColor(PGColorTextFieldCaret);
	this->text_color = PGStyleManager::GetColor(PGColorTabControlText);
}

Button::~Button() {
	auto manager = GetControlManager(this);
	if (manager) {
		manager->UnregisterControlForMouseEvents(this);
	}
}

void Button::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse = PGPoint(x, y);
	if (button == PGLeftMouseButton) {
		Invalidate();
		clicking = true;
	}
}

void Button::MouseMove(int x, int y, PGMouseButton buttons) {
	if (!(buttons & PGLeftMouseButton)) {
		if (clicking) {
			clicking = false;
			Invalidate();
		}
	}
}

void Button::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse = PGPoint(x, y);
	if (button == PGLeftMouseButton) {
		if (clicking) {
			clicking = false;
			Invalidate();
			if (on_pressed) {
				on_pressed(this, pressed_data);
			}
		}
	}
}

void Button::MouseEnter() {
	if (!hovering) {
		hovering = true;
		this->Invalidate();
	}
}

void Button::MouseLeave() {
	if (hovering) {
		hovering = false;
		this->Invalidate();
	}
}

void Button::ResolveSize(PGSize new_size) {
	if (!fixed_size && font) {
		this->fixed_width = MeasureTextWidth(font, text);
	}
	Control::ResolveSize(new_size);
}

void Button::Draw(PGRendererHandle renderer) {
	PGRect render_rectangle = PGRect(X(), Y(), this->width, this->height);

	if (this->clicking) {
		RenderRectangle(renderer, render_rectangle, background_color_click, PGDrawStyleFill);
	} else if (this->hovering) {
		RenderRectangle(renderer, render_rectangle, background_color_hover, PGDrawStyleFill);
	} else {
		RenderRectangle(renderer, render_rectangle, background_color, PGDrawStyleFill);
	}
	RenderRectangle(renderer, render_rectangle, background_stroke_color, PGDrawStyleStroke, 1.0f);
	if (image) {
		RenderImage(renderer, image, PGRect(render_rectangle.x + padding.left, render_rectangle.y + padding.top, render_rectangle.width - padding.left - padding.right, render_rectangle.height - padding.top - padding.bottom));
	}
	if (font) {
		SetTextColor(font, text_color);
		RenderText(renderer, font, text.c_str(), text.size(), render_rectangle.x + render_rectangle.width / 2, render_rectangle.y + (render_rectangle.height - GetTextHeight(font)) / 2 - 1, PGTextAlignHorizontalCenter);
	}
}

void Button::Invalidate() {
	parent.lock()->Invalidate();
}

void Button::SetText(std::string text, PGFontHandle font) {
	this->text = text;
	this->font = font;
}

#include "toolbar.h"
Button* Button::CreateFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
	std::map<std::string, PGKeyFunction>& functions, std::map<std::string, PGBitmapHandle>& images) {
	assert(functions.count(command_name) > 0);
	assert(images.count(command_name) > 0);

	Button* button = new Button(parent->window, parent);
	button->SetImage(images[command_name]);
	button->padding = PGPadding(4, 4, 4, 4);
	button->fixed_width = TOOLBAR_HEIGHT - button->padding.left - button->padding.right;
	button->fixed_height = TOOLBAR_HEIGHT - button->padding.top - button->padding.bottom;
	button->background_color = PGColor(0, 0, 0, 0);
	button->background_stroke_color = PGColor(0, 0, 0, 0);
	button->background_color_hover = PGStyleManager::GetColor(PGColorTextFieldSelection);
	button->SetTooltip(tooltip_text);
	button->OnPressed([](Button* b, void* data) {
		auto parent = b->parent.lock();
		((PGKeyFunction)data)(parent.get());
	}, (void*) functions[command_name]);
	return button;
}

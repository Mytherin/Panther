
#include "button.h"
#include "controlmanager.h"
#include "style.h"


Button::Button(PGWindowHandle window, Control* parent) : 
	Control(window) {
	this->parent = parent;
	GetControlManager(this)->RegisterControlForMouseEvents(this);

	this->background_color = PGStyleManager::GetColor(PGColorTabControlBackground);
	this->background_color_hover = PGStyleManager::GetColor(PGColorTabControlHover);
	this->background_color_click = PGStyleManager::GetColor(PGColorTabControlSelected);
	this->background_stroke_color = PGColor(0, 0, 0);
	this->text_color = PGStyleManager::GetColor(PGColorTabControlText);
}

Button::~Button() {
	GetControlManager(this)->UnregisterControlForMouseEvents(this);
}

void Button::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
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
			if (on_pressed) on_pressed(this);
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

void Button::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGRect render_rectangle = PGRect(X() - rect->x, Y() - rect->y, this->width, this->height);

	if (this->clicking) {
		RenderRectangle(renderer, render_rectangle, background_color_click, PGDrawStyleFill);
	} else if (this->hovering) {
		RenderRectangle(renderer, render_rectangle, background_color_hover, PGDrawStyleFill);
	} else {
		RenderRectangle(renderer, render_rectangle, background_color, PGDrawStyleFill);
	}
	RenderRectangle(renderer, render_rectangle, background_stroke_color, PGDrawStyleStroke);
	if (font) {
		SetTextColor(font, text_color);
		RenderText(renderer, font, text.c_str(), text.size(), render_rectangle.x + render_rectangle.width / 2, render_rectangle.y + render_rectangle.height / 2 - GetTextHeight(font) / 2 - 1, PGTextAlignHorizontalCenter);
	}
}


void Button::SetText(std::string text, PGFontHandle font) {
	this->text = text;
	this->font = font;

}

#include "button.h"
#include "controlmanager.h"


Button::Button(PGWindowHandle window, Control* parent) : 
	Control(window) {
	this->parent = parent;
	((ControlManager*)GetControlManager(window))->RegisterControlForMouseEvents(this);
}

Button::~Button() {
	((ControlManager*)GetControlManager(window))->UnregisterControlForMouseEvents(this);
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
		RenderRectangle(renderer, render_rectangle, PGColor(46, 146, 213), PGDrawStyleFill);
	} else if (this->hovering) {
		RenderRectangle(renderer, render_rectangle, PGColor(30, 138, 210), PGDrawStyleFill);
	}
}

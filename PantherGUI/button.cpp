
#include "button.h"
#include "controlmanager.h"


Button::Button(Control* parent) : parent(parent) {
	((ControlManager*)GetControlManager(parent->window))->RegisterMouseRegion(&region, parent, 
	[](Control* control, bool enter, void* data) {
		Button* b = (Button*)data;
		bool hover = b->hovering;
		if (hover != enter) {
			b->Invalidate();
		}
		b->hovering = enter;
	}, (void*) this);
}

Button::~Button() {
	((ControlManager*)GetControlManager(parent->window))->UnregisterMouseRegion(&region);

}

void Button::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse = PGPoint(x, y);
	if (button == PGLeftMouseButton) {
		if (PGRectangleContains(region, PGPoint(x, y))) {
			Invalidate();
			clicking = true;
		}
	}
}

void Button::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse = PGPoint(x, y);
	if (button == PGLeftMouseButton) {
		if (PGRectangleContains(region, PGPoint(x, y))) {
			if (clicking) {
				clicking = false;
				Invalidate();
				if (on_pressed) on_pressed(this);
			}
		}
	}
}

void Button::DrawBackground(PGRendererHandle renderer, PGIRect* rect) {
	PGRect render_rectangle = PGRect(parent->X() + region.x - rect->x, parent->Y() + region.y - rect->y, region.width, region.height);

	if (this->clicking) {
		RenderRectangle(renderer, render_rectangle, PGColor(46, 146, 213), PGDrawStyleFill);
	} else if (this->hovering) {
		RenderRectangle(renderer, render_rectangle, PGColor(30, 138, 210), PGDrawStyleFill);
	}
}
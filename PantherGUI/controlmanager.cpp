
#include "controlmanager.h"

ControlManager::ControlManager(PGWindowHandle window) : Control(window, false), controls(), focused_control(nullptr) {

}

void ControlManager::AddControl(Control* control) {
	assert(control);
	control->parent = this;
	controls.push_back(control);
	focused_control = control;
}

Control* ControlManager::GetMouseOverControl(int x, int y) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), PGPoint(x, y))) {
			return *it;
		}
	}
	return nullptr;
}

bool ControlManager::KeyboardButton(PGButton button, PGModifier modifier) {
	return focused_control->KeyboardButton(button, modifier);
}

bool ControlManager::KeyboardCharacter(char character, PGModifier modifier) {
	return focused_control->KeyboardCharacter(character, modifier);
}

bool ControlManager::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	return focused_control->KeyboardUnicode(character, modifier);
}

void ControlManager::PeriodicRender(void) {
	PGPoint mouse = GetMousePosition(window);
	PGMouseButton buttons = GetMouseState(window);
	// for any registered mouse regions, check if the mouse status has changed (mouse has entered or left the area)
	for (auto it = regions.begin(); it != regions.end(); it++) {
		bool contains = PGRectangleContains(*(*it).rect, mouse - (*it).control->Position());
		if (!(*it).mouse_inside && contains) {
			// mouse enter
			(*it).mouse_event((*it).control, true);
		} else if ((*it).mouse_inside && !contains) {
			// mouse leave
			(*it).mouse_event((*it).control, false);
		}
		(*it).mouse_inside = contains;
	}
	bool is_dragging = false;
	// for any controls, trigger the periodic render
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->IsDragging()) {
			is_dragging = true;
			// if child the control is listening to MouseMove events, send a MouseMove message 
			(*it)->MouseMove(mouse.x, mouse.y, buttons);
		}
		// trigger the periodic render of the child control
		// this is mostly used for animations
		(*it)->PeriodicRender();
	}
	PGCursorType cursor = PGCursorStandard;
	if (!is_dragging) {
		cursor = GetCursor(mouse);
	}
	SetCursor(this->window, cursor);
	// after the periodic render, render anything that needs to be rerendered (if any)
	if (this->invalidated) {
		RedrawWindow(window);
	} else if (this->invalidated_area.width != 0) {
		RedrawWindow(window, this->invalidated_area);
	}
	this->invalidated = 0;
	this->invalidated_area.width = 0;
}

PGCursorType ControlManager::GetCursor(PGPoint mouse) {
	mouse.x -= this->x;
	mouse.y -= this->y;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), mouse)) {
			return (*it)->GetCursor(mouse);
		}
	}
	return PGCursorNone;
}

void ControlManager::RefreshWindow() {
	this->invalidated = true;
}

void ControlManager::RefreshWindow(PGIRect rectangle) {
	// invalidate a rectangle
	if (this->invalidated_area.width == 0) {
		// if there is no currently invalidated rectangle,
		// the invalidated rectangle is simply the supplied rectangle
		this->invalidated_area = rectangle;
	} else {
		int maxx = this->invalidated_area.x + this->invalidated_area.width;
		int maxy = this->invalidated_area.y + this->invalidated_area.height;

		// otherwise we merge the two rectangles into one big one
		this->invalidated_area.x = std::min(this->invalidated_area.x, rectangle.x);
		this->invalidated_area.y = std::min(this->invalidated_area.y, rectangle.y);
		this->invalidated_area.width = std::max(maxx, rectangle.x + rectangle.width) - this->invalidated_area.x;
		this->invalidated_area.height = std::max(maxy, rectangle.y + rectangle.height) - this->invalidated_area.y;
		assert(this->invalidated_area.x <= rectangle.x &&
			this->invalidated_area.y <= rectangle.y &&
			this->invalidated_area.x + this->invalidated_area.width >= rectangle.x + rectangle.width &&
			this->invalidated_area.y + this->invalidated_area.height >= rectangle.y + rectangle.height);
	}
}

void ControlManager::OnResize(PGSize old_size, PGSize new_size) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->UpdateParentSize(old_size, new_size);
	}
}

void ControlManager::Draw(PGRendererHandle renderer, PGIRect* rect) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (rect && !PGIRectanglesOverlap(PGIRect((*it)->GetRectangle()), *rect)) {
			continue;
		}
		(*it)->Draw(renderer, rect);
	}
}

void ControlManager::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	Control* c = GetMouseOverControl(x, y);
	if (c) c->MouseClick(x, y, button, modifier);
}

void ControlManager::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	Control* c = GetMouseOverControl(x, y);
	if (c) c->MouseDown(x, y, button, modifier);
}

void ControlManager::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	Control* c = GetMouseOverControl(x, y);
	if (c) c->MouseUp(x, y, button, modifier);
}

void ControlManager::MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	assert(0);
	return;
}

void ControlManager::MouseMove(int x, int y, PGMouseButton buttons) {
	Control* c = GetMouseOverControl(x, y);
	if (c) c->MouseMove(x, y, buttons);
}

void ControlManager::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	Control* c = GetMouseOverControl(x, y);
	if (c) c->MouseWheel(x, y, distance, modifier);
}

void ControlManager::RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event) {
	regions.push_back(PGMouseRegion(rect, control, mouse_event));
}

void ControlManager::UnregisterMouseRegion(PGIRect* rect) {
	for (auto it = regions.begin(); it != regions.end(); it++) {
		if ((*it).rect == rect) {
			regions.erase(it);
			return;
		}
	}
	assert(0);
}

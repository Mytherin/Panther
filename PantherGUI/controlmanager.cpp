
#include "controlmanager.h"



ControlManager::ControlManager(PGWindowHandle window) : Control(window, false), controls(), focused_control(nullptr) {

}

void ControlManager::AddControl(Control* control) {
	assert(control);
	controls.push_back(control);
	focused_control = control;
}

Control* ControlManager::GetMouseOverControl(int x, int y) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains(PGRect((*it)->x, (*it)->y, (*it)->width, (*it)->height), PGPoint(x, y))) {
			return *it;
		}
	}
	return nullptr;
}

void ControlManager::KeyboardButton(PGButton button, PGModifier modifier) {
	focused_control->KeyboardButton(button, modifier);
}

void ControlManager::KeyboardCharacter(char character, PGModifier modifier) {
	focused_control->KeyboardCharacter(character, modifier);
}

void ControlManager::KeyboardUnicode(char* character, PGModifier modifier) {
	focused_control->KeyboardUnicode(character, modifier);
}

void ControlManager::PeriodicRender(void) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->PeriodicRender();
	}
	if (this->invalidated) {
		RedrawWindow(window);
	} else if (this->invalidated_area.width != 0) {
		RedrawWindow(window, this->invalidated_area);
	}
	this->invalidated = 0;
	this->invalidated_area.width = 0;
}

void ControlManager::RefreshWindow() {
	this->invalidated = true;
}

void ControlManager::RefreshWindow(PGIRect rectangle) {
	// FIXME do not invalidate entire window here
	this->invalidated = true;
	if (this->invalidated_area.width == 0) {
		this->invalidated_area = rectangle;
	} else {
		this->invalidated_area.x = std::min(this->invalidated_area.x, rectangle.x);
		this->invalidated_area.y = std::min(this->invalidated_area.y, rectangle.y);
		this->invalidated_area.width = std::max(this->invalidated_area.x + this->invalidated_area.width, rectangle.x + rectangle.width) - this->invalidated_area.x;
		this->invalidated_area.height = std::max(this->invalidated_area.y + this->invalidated_area.height, rectangle.y + rectangle.height) - this->invalidated_area.y;
		assert(this->invalidated_area.x <= rectangle.x &&
			this->invalidated_area.y <= rectangle.y &&
			this->invalidated_area.x + this->invalidated_area.width >= rectangle.x + rectangle.width &&
			this->invalidated_area.y + this->invalidated_area.height >= rectangle.y + rectangle.height);
	}
}

void ControlManager::Resize(PGSize old_size, PGSize new_size) {
	int width = new_size.width;
	int height = new_size.height;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		PGAnchor anchor = (*it)->anchor;
		if (anchor & PGAnchorLeft && anchor & PGAnchorRight) {
			(*it)->width = width;
		} else if (anchor & PGAnchorRight) {
			(*it)->width = width - (*it)->x;
		} else if (anchor & PGAnchorLeft) {
			int old = old_size.width - ((*it)->x + (*it)->width);
			(*it)->width = (old - width) + (*it)->x;
		}

		if (anchor & PGAnchorBottom && anchor & PGAnchorTop) {
			(*it)->height = height;
		} else if (anchor & PGAnchorBottom) {
			(*it)->height = height - (*it)->y;
		} else if (anchor & PGAnchorTop) {
			int old = old_size.width - ((*it)->y + (*it)->width);
			(*it)->height = (old - height) + (*it)->y;
		}
	}
}

void ControlManager::Draw(PGRendererHandle renderer, PGIRect* rect) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
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


#include "container.h"
#include "statusbar.h"
#include "simpletextfield.h"


PGContainer::PGContainer(PGWindowHandle window) :
	Control(window) {
}

PGContainer::~PGContainer() {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		delete *it;
	}
}

bool PGContainer::KeyboardButton(PGButton button, PGModifier modifier) {
	FlushRemoves();
	if (focused_control) {
		if (focused_control->KeyboardButton(button, modifier)) {
			return true;
		}
	}
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it) == focused_control) continue;
		if ((*it)->KeyboardButton(button, modifier)) {
			return true;
		}
	}
	return false;
}

bool PGContainer::KeyboardCharacter(char character, PGModifier modifier) {
	FlushRemoves();
	if (focused_control) {
		if (focused_control->KeyboardCharacter(character, modifier)) {
			return true;
		}
	}
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it) == focused_control) continue;
		if ((*it)->KeyboardCharacter(character, modifier)) {
			return true;
		}
	}
	return false;
}

bool PGContainer::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	FlushRemoves();
	if (focused_control) {
		if (focused_control->KeyboardUnicode(character, modifier)) {
			return true;
		}
	}
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it) == focused_control) continue;
		if ((*it)->KeyboardUnicode(character, modifier)) {
			return true;
		}
	}
	return false;
}

void PGContainer::PeriodicRender(void) {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->PeriodicRender();
	}
}

void PGContainer::Draw(PGRendererHandle renderer, PGIRect* rect) {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (rect && !PGIRectanglesOverlap(PGIRect((*it)->X(), (*it)->Y(), (*it)->width, (*it)->height), *rect)) {
			continue;
		}
		(*it)->Draw(renderer, rect);
	}
}

void PGContainer::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	assert(0);
}

void PGContainer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseDown(mouse.x, mouse.y, button, modifier);
			if (c->ControlTakesFocus()) {
				focused_control = c;
			}
			return;
		}
	}
}

void PGContainer::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseUp(mouse.x, mouse.y, button, modifier);
			if (c->ControlTakesFocus()) {
				focused_control = c;
			}
			return;
		}
	}
}

void PGContainer::MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	assert(0);
}

void PGContainer::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseWheel(x, y, distance, modifier);
			return;
		}
	}
}

void PGContainer::MouseMove(int x, int y, PGMouseButton buttons) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (c->IsDragging()) {
			c->MouseMove(mouse.x, mouse.y, buttons);
		}
	}
}

void PGContainer::OnResize(PGSize old_size, PGSize new_size) {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->size_resolved = false;
	}

	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->ResolveSize(new_size);
	}
}

PGCursorType PGContainer::GetCursor(PGPoint mouse) {
	FlushRemoves();
	mouse.x -= this->x;
	mouse.y -= this->y;
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			return c->GetCursor(mouse);
		}
	}
	return PGCursorNone;
}

bool PGContainer::IsDragging() {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->IsDragging()) {
			return true;
		}
	}
	return false;

}

void PGContainer::AddControl(Control* control) {
	FlushRemoves();
	assert(control);
	control->parent = this;
	controls.push_back(control);
	if (control->ControlTakesFocus()) {
		this->focused_control = control;
	}
}

void PGContainer::RemoveControl(Control* control) {
	pending_removes.push_back(control);
}


void PGContainer::FlushRemoves() {
	std::vector<Control*> removals = pending_removes;
	pending_removes.clear();
	for (auto it = removals.begin(); it != removals.end(); it++) {
		ActuallyRemoveControl(*it);
	}
}

void PGContainer::ActuallyRemoveControl(Control* control) {
	assert(control);
	assert(std::find(controls.begin(), controls.end(), control) != controls.end());
	this->controls.erase(std::find(controls.begin(), controls.end(), control));
	if (this->focused_control == control) {
		this->focused_control = nullptr;
		for (auto it = controls.begin(); it != controls.end(); it++) {
			if ((*it)->ControlTakesFocus()) {
				this->focused_control = (*it);
				break;
			}
		}
	}
	bool trigger_resize = false;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->horizontal_anchor == control) {
			trigger_resize = true;
			(*it)->horizontal_anchor = control->horizontal_anchor;
		}
		if ((*it)->vertical_anchor == control) {
			trigger_resize = true;
			(*it)->vertical_anchor = control->vertical_anchor;
		}
	}
	if (trigger_resize)
		this->TriggerResize();
	delete control;
}

Control* PGContainer::GetMouseOverControl(int x, int y) {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), PGPoint(x, y))) {
			return *it;
		}
	}
	return nullptr;
}

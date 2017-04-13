
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
	for (lng i = 0; i < controls.size(); i++) {
		controls[i]->PeriodicRender();
	}
}

void PGContainer::Draw(PGRendererHandle renderer) {
	FlushRemoves();
	if (!dirty) return;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->dirty) {
			(*it)->Draw(renderer);
		}
	}
	Control::Draw(renderer);
}

void PGContainer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseDown(mouse.x, mouse.y, button, modifier, click_count);
			if (c->ControlTakesFocus()) {
				SetFocusedControl(c);
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
				SetFocusedControl(c);
			}
			return;
		}
	}
}

void PGContainer::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseWheel(x, y, hdistance, distance, modifier);
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

void PGContainer::SetFocus(void) {
	if (!this->parent) return;
	PGContainer* container = dynamic_cast<PGContainer*>(this->parent);
	if (!container) return;
	if (container) {
		if (container->focused_control != this) {
			container->SetFocusedControl(this);
		}
	}

}

void PGContainer::LosesFocus(void) {
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		c->LosesFocus();
	}
}

void PGContainer::GainsFocus(void) {
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		c->GainsFocus();
	}
}

bool PGContainer::AcceptsDragDrop(PGDragDropType type) {
	for (lng i = controls.size() - 1; i >= 0; i--) {
		if (controls[i]->AcceptsDragDrop(type)) {
			return true;
		}
	}
	return false;
}

void PGContainer::DragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (c->AcceptsDragDrop(type)) {
			c->DragDrop(type, mouse.x, mouse.y, data);
		}
	}
}

void PGContainer::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (c->AcceptsDragDrop(type)) {
			c->PerformDragDrop(type, mouse.x, mouse.y, data);
		}
	}
}

void PGContainer::ClearDragDrop(PGDragDropType type) {
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (c->AcceptsDragDrop(type)) {
			c->ClearDragDrop(type);
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

void PGContainer::LoadWorkspace(nlohmann::json& j) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->LoadWorkspace(j);
	}
}

void PGContainer::WriteWorkspace(nlohmann::json& j) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->WriteWorkspace(j);
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

PGCursorType PGContainer::GetDraggingCursor() {
	FlushRemoves();
	for (lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (c->IsDragging()) {
			return c->GetDraggingCursor();
		}
	}
	return PGCursorStandard;
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
	pending_additions.push_back(control);
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
	std::vector<Control*> additions = pending_additions;
	pending_additions.clear();
	for (auto it = additions.begin(); it != additions.end(); it++) {
		ActuallyAddControl(*it);
	}

	if (pending_resize) {
		pending_resize = false;
		Control::TriggerResize();
	}
}

void PGContainer::ActuallyAddControl(Control* control) {
	assert(control);
	assert(control->parent == nullptr || control->parent == this);
	control->parent = this;
	controls.push_back(control);
	if (control->ControlTakesFocus()) {
		SetFocusedControl(control);
	}
	this->Invalidate();
}


void PGContainer::SetFocusedControl(Control* c) {
	if (this->focused_control) {
		this->focused_control->LosesFocus();
	}
	this->focused_control = c;
	this->focused_control->GainsFocus();
	this->SetFocus();
}

void PGContainer::ActuallyRemoveControl(Control* control) {
	assert(control);
	assert(std::find(controls.begin(), controls.end(), control) != controls.end());
	this->controls.erase(std::find(controls.begin(), controls.end(), control));
	if (this->focused_control == control) {
		this->focused_control = nullptr;
		for (auto it = controls.begin(); it != controls.end(); it++) {
			if ((*it)->ControlTakesFocus()) {
				SetFocusedControl(*it);
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
		if ((*it)->bottom_anchor == control) {
			trigger_resize = true;
			(*it)->bottom_anchor = control->bottom_anchor;
		}
		if ((*it)->top_anchor == control) {
			trigger_resize = true;
			(*it)->top_anchor = control->top_anchor;
		}
	}
	if (trigger_resize)
		this->TriggerResize();
	delete control;
	this->Invalidate();
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

void PGContainer::Invalidate(bool initial_invalidate) {
	if (!this->dirty) {
		FlushRemoves();
		this->dirty = true;
		for (auto it = controls.begin(); it != controls.end(); it++) {
			(*it)->Invalidate();
		}
	}
	Control::Invalidate(initial_invalidate);
}


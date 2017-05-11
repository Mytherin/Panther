
#include "container.h"
#include "statusbar.h"
#include "simpletextfield.h"


PGContainer::PGContainer(PGWindowHandle window) :
	Control(window), everything_dirty(false) {
}

PGContainer::~PGContainer() {
	for(auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->parent = nullptr;
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
		if (it->get() == focused_control) continue;
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
		if (it->get() == focused_control) continue;
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
		if (it->get() == focused_control) continue;
		if ((*it)->KeyboardUnicode(character, modifier)) {
			return true;
		}
	}
	return false;
}

void PGContainer::Update(void) {
	FlushRemoves();
	for (lng i = 0; i < controls.size(); i++) {
		controls[i]->Update();
	}
}

void PGContainer::Draw(PGRendererHandle renderer) {
	FlushRemoves();
	if (!dirty) return;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->dirty || everything_dirty) {
			if (everything_dirty) {
				(*it)->dirty = true;
				PGContainer* container = dynamic_cast<PGContainer*>(it->get());
				if (container) {
					container->everything_dirty = true;
				}
			}
			(*it)->Draw(renderer);
		}
	}
	everything_dirty = false;
	Control::Draw(renderer);
}

void PGContainer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		Control* c = controls[i].get();
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
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		Control* c = controls[i].get();
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
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		Control* c = controls[i].get();
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseWheel(x, y, hdistance, distance, modifier);
			return;
		}
	}
}

void PGContainer::MouseMove(int x, int y, PGMouseButton buttons) {
	FlushRemoves();
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		Control* c = controls[i].get();
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
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		controls[i]->LosesFocus();
	}
}

void PGContainer::GainsFocus(void) {
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		controls[i]->GainsFocus();
	}
}

bool PGContainer::AcceptsDragDrop(PGDragDropType type) {
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		if (controls[i]->AcceptsDragDrop(type)) {
			return true;
		}
	}
	return false;
}

void PGContainer::DragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		if (controls[i]->AcceptsDragDrop(type)) {
			controls[i]->DragDrop(type, mouse.x, mouse.y, data);
		}
	}
}

void PGContainer::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		if (controls[i]->AcceptsDragDrop(type)) {
			controls[i]->PerformDragDrop(type, mouse.x, mouse.y, data);
		}
	}
}

void PGContainer::ClearDragDrop(PGDragDropType type) {
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		if (controls[i]->AcceptsDragDrop(type)) {
			controls[i]->ClearDragDrop(type);
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
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->LoadWorkspace(j);
	}
}

void PGContainer::WriteWorkspace(nlohmann::json& j) {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->WriteWorkspace(j);
	}
}

PGCursorType PGContainer::GetCursor(PGPoint mouse) {
	FlushRemoves();
	mouse.x -= this->x;
	mouse.y -= this->y;
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		if (PGRectangleContains(controls[i]->GetRectangle(), mouse)) {
			return controls[i]->GetCursor(mouse);
		}
	}
	return PGCursorNone;
}

PGCursorType PGContainer::GetDraggingCursor() {
	FlushRemoves();
	for (lng i = controls.size() - 1LL; i >= 0; i--) {
		if (controls[i]->IsDragging()) {
			return controls[i]->GetDraggingCursor();
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


void PGContainer::AddControl(std::shared_ptr<Control> control) {
	pending_additions.push_back(control);
}

void PGContainer::RemoveControl(std::shared_ptr<Control>& control) {
	control->destroyed = true;
	this->pending_removal = true;
}

void PGContainer::RemoveControl(Control* control) {
	control->destroyed = true;
	this->pending_removal = true;
}

void PGContainer::FlushRemoves() {
	if (pending_removal) {
		pending_resize = true;
		for(size_t i = 0; i < controls.size(); i++) {
			if (controls[i]->destroyed) {
				ActuallyRemoveControl(i);
				i--;
			}
		}
		pending_removal = false;
	}
	if (pending_additions.size() > 0) {
		pending_resize = true;
		for (auto it = pending_additions.begin(); it != pending_additions.end(); it++) {
			ActuallyAddControl(*it);
		}
		pending_additions.clear();
	}
	if (pending_resize) {
		pending_resize = false;
		Control::TriggerResize();
	}
}

void PGContainer::ActuallyAddControl(std::shared_ptr<Control> control) {
	assert(control);
	assert(control->parent == nullptr || control->parent == this);
	control->parent = this;
	controls.push_back(control);
	if (control->ControlTakesFocus()) {
		SetFocusedControl(control.get());
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

void PGContainer::ActuallyRemoveControl(size_t index) {
	assert(index < controls.size());
	Control* control = controls[index].get();
	if (this->focused_control == control) {
		this->focused_control = nullptr;
		for (auto it = controls.begin(); it != controls.end(); it++) {
			if (it->get() != control && (*it)->ControlTakesFocus()) {
				SetFocusedControl(it->get());
				break;
			}
		}
	}
	bool trigger_resize = false;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->left_anchor == control) {
			trigger_resize = true;
			(*it)->left_anchor = control->left_anchor;
		}
		if ((*it)->right_anchor == control) {
			trigger_resize = true;
			(*it)->right_anchor = control->right_anchor;
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
	controls.erase(controls.begin() + index);
	this->Invalidate();
}

Control* PGContainer::GetMouseOverControl(int x, int y) {
	FlushRemoves();
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), PGPoint(x, y))) {
			return it->get();
		}
	}
	return nullptr;
}

void PGContainer::Invalidate(bool initial_invalidate) {
	this->dirty = true;
	if (initial_invalidate) {
		everything_dirty = true;
	}
	Control::Invalidate(initial_invalidate);
}


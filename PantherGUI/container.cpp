
#include "container.h"
#include "statusbar.h"
#include "simpletextfield.h"


PGContainer::PGContainer(PGWindowHandle window, TextFile* file) : Control(window, true) {
	TextField* textfield = new TextField(window, file);
	this->width = 0;
	this->height = TEXT_TAB_HEIGHT;
	textfield->SetAnchor(PGAnchorBottom | PGAnchorLeft | PGAnchorRight);
	textfield->SetPosition(PGPoint(this->x, this->y + TEXT_TAB_HEIGHT));
	textfield->SetSize(PGSize(this->width, this->height - TEXT_TAB_HEIGHT - STATUSBAR_HEIGHT));
	TabControl* tabs = new TabControl(window, textfield);
	tabs->SetAnchor(PGAnchorLeft | PGAnchorRight);
	tabs->SetPosition(PGPoint(this->x, this->y));
	tabs->SetSize(PGSize(this->width, TEXT_TAB_HEIGHT));
	StatusBar* bar = new StatusBar(window, textfield);
	bar->SetAnchor(PGAnchorLeft | PGAnchorRight);
	bar->SetPosition(PGPoint(this->x, this->y + this->height - STATUSBAR_HEIGHT));
	bar->SetSize(PGSize(this->width, STATUSBAR_HEIGHT));

	AddControl(tabs);
	AddControl(textfield);
	AddControl(bar);
}

bool PGContainer::KeyboardButton(PGButton button, PGModifier modifier) {
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
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->PeriodicRender();
	}
}

void PGContainer::Draw(PGRendererHandle renderer, PGIRect* rect) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (rect && !PGIRectanglesOverlap(PGIRect((*it)->X(), (*it)->Y(), (*it)->width, (*it)->height), *rect)) {
			continue;
		}
		(*it)->Draw(renderer, rect);
	}
}

void PGContainer::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for(lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseClick(mouse.x, mouse.y, button, modifier);
		}
	}
}

void PGContainer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for(lng i = controls.size() - 1; i >= 0; i--) {
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
	PGPoint mouse(x - this->x, y - this->y);
	for(lng i = controls.size() - 1; i >= 0; i--) {
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
	PGPoint mouse(x - this->x, y - this->y);
	for(lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			c->MouseWheel(x, y, distance, modifier);
			return;
		}
	}
}

void PGContainer::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	for(lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (c->IsDragging()) {
			c->MouseMove(mouse.x, mouse.y, buttons);
		}
	}
}

void PGContainer::OnResize(PGSize old_size, PGSize new_size) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->UpdateParentSize(old_size, new_size);
	}
}

PGCursorType PGContainer::GetCursor(PGPoint mouse) {
	mouse.x -= this->x;
	mouse.y -= this->y;
	for(lng i = controls.size() - 1; i >= 0; i--) {
		Control* c = controls[i];
		if (PGRectangleContains(c->GetRectangle(), mouse)) {
			return c->GetCursor(mouse);
		}
	}
	return PGCursorNone;
}

bool PGContainer::IsDragging() {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->IsDragging()) {
			return true;
		}
	}
	return false;

}

void PGContainer::AddControl(Control* control) {
	assert(control);
	control->parent = this;
	controls.push_back(control);
	if (control->ControlTakesFocus()) {
		this->focused_control = control;
	}
}

void PGContainer::RemoveControl(Control* control) {
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
	delete control;
}

#include "container.h"
#include "statusbar.h"


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
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->KeyboardButton(button, modifier)) {
			return true;
		}
	}
	return false;
}

bool PGContainer::KeyboardCharacter(char character, PGModifier modifier) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->KeyboardCharacter(character, modifier)) {
			return true;
		}
	}
	return false;
}

bool PGContainer::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	for (auto it = controls.begin(); it != controls.end(); it++) {
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
		if (rect && !PGIRectanglesOverlap(PGIRect((*it)->GetRectangle()), *rect)) {
			continue;
		}
		(*it)->Draw(renderer, rect);
	}
}

void PGContainer::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), mouse)) {
			(*it)->MouseClick(x, y, button, modifier);
		}
	}
}

void PGContainer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), mouse)) {
			(*it)->MouseDown(x, y, button, modifier);
		}
	}
}

void PGContainer::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), mouse)) {
			(*it)->MouseUp(x, y, button, modifier);
		}
	}
}

void PGContainer::MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	assert(0);
}

void PGContainer::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), mouse)) {
			(*it)->MouseWheel(x, y, distance, modifier);
		}
	}
}

void PGContainer::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->IsDragging()) {
			(*it)->MouseMove(mouse.x, mouse.y, buttons);
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
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if (PGRectangleContains((*it)->GetRectangle(), mouse)) {
			return (*it)->GetCursor(mouse);
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
}
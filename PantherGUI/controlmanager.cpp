
#include "controlmanager.h"
#include "findtext.h"
#include "statusbar.h"
#include "textfield.h"
#include "filemanager.h"

ControlManager::ControlManager(PGWindowHandle window) : PGContainer(window) {

}

void ControlManager::PeriodicRender(void) {
	PGPoint mouse = GetMousePosition(window);
	PGMouseButton buttons = GetMouseState(window);

	bool is_dragging = false;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->IsDragging()) {
			is_dragging = true;
			// if child the control is listening to MouseMove events, send a MouseMove message 
			(*it)->MouseMove(mouse.x, mouse.y, buttons);
		}
	}
	// for any registered mouse regions, check if the mouse status has changed (mouse has entered or left the area)
	if (!is_dragging) {
		for (auto it = regions.begin(); it != regions.end(); it++) {
			Control* c = (*it).control;
			if ((*it).rect == nullptr) {
				PGIRect rectangle = PGIRect(c->X(), c->Y(), c->width, c->height);
				bool contains = PGRectangleContains(rectangle, mouse);
				if (!(*it).mouse_inside && contains) {
					c->MouseEnter();
				} else if ((*it).mouse_inside && !contains) {
					c->MouseLeave();
				}
				(*it).mouse_inside = contains;
			} else {
				bool contains = PGRectangleContains(*(*it).rect, mouse - (*it).control->Position());
				if (!(*it).mouse_inside && contains) {
					// mouse enter
					(*it).mouse_event((*it).control, true, (*it).data);
				} else if ((*it).mouse_inside && !contains) {
					// mouse leave
					(*it).mouse_event((*it).control, false, (*it).data);
				}
				(*it).mouse_inside = contains;
			}
		}
	}
	for (auto it = controls.begin(); it != controls.end(); it++) {
		// trigger the periodic render of the child controls
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

bool ControlManager::KeyboardCharacter(char character, PGModifier modifier) {
	if (modifier == PGModifierCtrl) {
		bool replace = false;
		switch (character) {
		case 'O': {
			std::vector<std::string> files = ShowOpenFileDialog(true, false, true);
			for(auto it = files.begin(); it != files.end(); it++) {
				TextFile* file = FileManager::OpenFile(*it);
				if (file) {
					active_tabcontrol->AddTab(file);
				}
			}
			this->Invalidate();
			break;	
		}
		case 'H':
			replace = true;
		case 'F': {
			// Find text
			FindText* view = new FindText(this->window, replace);
			view->SetAnchor(PGAnchorBottom | PGAnchorLeft);
			view->vertical_anchor = statusbar;
			for (auto it = controls.begin(); it != controls.end(); it++) {
				if ((*it)->vertical_anchor == statusbar) {
					(*it)->vertical_anchor = view;
				}
			}
			this->TriggerResize();
			this->AddControl(view);
			this->Invalidate();
			return true;
		}
		}
	}

	if (modifier == PGModifierCtrlShift) {
		switch (character) {
		case 'S':
			std::string filename = ShowSaveFileDialog();
			if (filename.size() != 0) {
				this->active_textfield->GetTextFile().SaveAs(filename);
			}
			break;
		}

	}
	return PGContainer::KeyboardCharacter(character, modifier);
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

void ControlManager::RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data) {
	regions.push_back(PGMouseRegion(rect, control, mouse_event, data));
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

void ControlManager::RegisterControlForMouseEvents(Control* control) {
	regions.push_back(PGMouseRegion(nullptr, control, nullptr, nullptr));
}

void ControlManager::UnregisterControlForMouseEvents(Control* control) {
	for (auto it = regions.begin(); it != regions.end(); it++) {
		if ((*it).control == control) {
			regions.erase(it);
			return;
		}
	}
	assert(0);
}

ControlManager* GetControlManager(Control* c) {
	return (ControlManager*)GetWindowManager(c->window);
}
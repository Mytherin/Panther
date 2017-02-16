
#include "controlmanager.h"
#include "findtext.h"
#include "statusbar.h"
#include "filemanager.h"
#include "tabcontrol.h"
#include "textfield.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(ControlManager);

ControlManager::ControlManager(PGWindowHandle window) : 
	PGContainer(window), active_projectexplorer(nullptr) {
#ifdef PANTHER_DEBUG
	entrance_count = 0;
#endif
}

void ControlManager::PeriodicRender(void) {
	EnterManager();
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
	} else {
		cursor = GetDraggingCursor();
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
	LeaveManager();
}

bool ControlManager::KeyboardCharacter(char character, PGModifier modifier) {
	EnterManager();
	if (this->PressCharacter(ControlManager::keybindings, character, modifier)) {
		LeaveManager();
		return true;
	}
	bool retval = PGContainer::KeyboardCharacter(character, modifier);
	LeaveManager();
	return retval;
}

void ControlManager::RefreshWindow(bool redraw_now) {
	this->invalidated = true;
	if (redraw_now) RedrawWindow(window);
}

void ControlManager::RefreshWindow(PGIRect rectangle, bool redraw_now) {
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
	if (redraw_now) RedrawWindow(window, invalidated_area);
}

void ControlManager::RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data) {
	regions.push_back(PGMouseRegion(rect, control, mouse_event, data));
}

void ControlManager::UnregisterMouseRegion(PGIRect* rect) {
	if (is_destroyed) return;
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
	if (is_destroyed) return;
	for (auto it = regions.begin(); it != regions.end(); it++) {
		if ((*it).control == control) {
			regions.erase(it);
			return;
		}
	}
	assert(0);
}

void ControlManager::DropFile(std::string filename) {
	active_tabcontrol->OpenFile(filename);
}

ControlManager* GetControlManager(Control* c) {
	return (ControlManager*)GetWindowManager(c->window);
}

void ControlManager::ShowFindReplace(PGFindTextType type) {
	// Find text
	FindText* view = new FindText(this->window, type);
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
}

void ControlManager::CreateNewWindow() {
	std::vector<std::shared_ptr<TextFile>> files;
	files.push_back(std::shared_ptr<TextFile>(new TextFile(nullptr)));
	PGWindowHandle new_window = PGCreateWindow(files);
	ShowWindow(new_window);
}

bool ControlManager::CloseControlManager() {
	if (active_tabcontrol) {
		return active_tabcontrol->CloseAllTabs();
	}
	return true;
}

void ControlManager::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = ControlManager::keybindings_noargs;
	noargs["new_window"] = [](Control* c) {
		ControlManager* t = (ControlManager*)c;
		t->CreateNewWindow();
	};
	noargs["close_window"] = [](Control* c) {
		ControlManager* t = (ControlManager*)c;
		PGCloseWindow(t->window);
	};
	std::map<std::string, PGKeyFunctionArgs>& args = ControlManager::keybindings_varargs;
	args["show_find"] = [](Control* c, std::map<std::string, std::string> args) {
		ControlManager* t = (ControlManager*)c;
		PGFindTextType type = PGFindSingleFile;
		if (args.count("type") > 0) {
			if (args["type"] == "findreplace") {
				type = PGFindReplaceSingleFile;
			} else if (args["type"] == "findinfiles") {
				type = PGFindReplaceManyFiles;
			}
		}
		t->ShowFindReplace(type);
	};
}

void ControlManager::EnterManager() {
#ifdef PANTHER_DEBUG
	entrance_count++;
	assert(entrance_count == 1);
#endif
}

void ControlManager::LeaveManager() {
#ifdef PANTHER_DEBUG
	entrance_count--;
	assert(entrance_count == 0);
#endif
}

void ControlManager::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	EnterManager();
	PGContainer::MouseWheel(x, y, hdistance, distance, modifier);
	LeaveManager();
}

bool ControlManager::KeyboardButton(PGButton button, PGModifier modifier) {
	EnterManager();
	bool retval = PGContainer::KeyboardButton(button, modifier);
	LeaveManager();
	return retval;
}

bool ControlManager::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	EnterManager();
	bool retval = PGContainer::KeyboardUnicode(character, modifier);
	LeaveManager();
	return retval;

}

void ControlManager::Draw(PGRendererHandle renderer, PGIRect* rectangle) {
	EnterManager();
	PGContainer::Draw(renderer, rectangle);
	LeaveManager();
}

void ControlManager::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	EnterManager();
	PGPoint mouse(x - this->x, y - this->y);
	time_t time = GetTime();
	if (time - last_click.time < DOUBLE_CLICK_TIME &&
		panther::abs(mouse.x - last_click.x) < 2 &&
		panther::abs(mouse.y - last_click.y) < 2) {
		last_click.clicks = last_click.clicks == 2 ? 0 : last_click.clicks + 1;
	} else {
		last_click.clicks = 0;
	}
	last_click.time = time;
	last_click.x = mouse.x;
	last_click.y = mouse.y;
	PGContainer::MouseDown(x, y, button, modifier, last_click.clicks);
	LeaveManager();
}

void ControlManager::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	EnterManager();
	PGContainer::MouseUp(x, y, button, modifier);
	LeaveManager();
}

void ControlManager::MouseMove(int x, int y, PGMouseButton buttons) {
	EnterManager();
	PGContainer::MouseMove(x, y, buttons);
	LeaveManager();
}


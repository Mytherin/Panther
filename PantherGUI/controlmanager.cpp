
#include "controlmanager.h"
#include "statusbar.h"
#include "findtext.h"
#include "filemanager.h"
#include "tabcontrol.h"
#include "textfield.h"
#include "workspace.h"
#include "toolbar.h"
#include "replaymanager.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(ControlManager);

ControlManager::ControlManager(PGWindowHandle window) :
	PGContainer(window), active_projectexplorer(nullptr), active_findtext(nullptr), 
	is_focused(true), columns(0), rows(0), projectexplorer_width(200)
{
#ifdef PANTHER_DEBUG
	entrance_count = 0;
#endif
}

void ControlManager::Update(void) {
	EnterManager();
	PGPoint mouse = GetMousePosition(window);
	PGMouseButton buttons = GetMouseState(window);

	is_dragging = false;
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
			(*it)->MouseMove(mouse);
		}
	}
	for (auto it = controls.begin(); it != controls.end(); it++) {
		// trigger the periodic render of the child controls
		// this is mostly used for animations
		(*it)->Update();
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

	if (write_workspace_counter > 0) {
		write_workspace_counter--;
		if (write_workspace_counter == 0) {
			PGGetWorkspace(window)->WriteWorkspace();
		}
	}
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

void ControlManager::ShowProjectExplorer(bool visible) {
	if (visible) {
		active_projectexplorer->fixed_width = projectexplorer_width;
		splitter->fixed_width = 4;
	} else {
		projectexplorer_width = active_projectexplorer->width;
		splitter->fixed_width = 0;
		active_projectexplorer->fixed_width = 0;
	}
	this->TriggerResize();
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
	regions.push_back(std::unique_ptr<PGMouseRegion>(new PGSingleMouseRegion(rect, control, mouse_event, data)));
}

void ControlManager::UnregisterMouseRegion(PGIRect* rect) {
	if (is_destroyed) return;
	for (auto it = regions.begin(); it != regions.end(); it++) {
		auto single_region = dynamic_cast<PGSingleMouseRegion*>(it->get());
		if (single_region && single_region->rect == rect) {
			regions.erase(it);
			return;
		}
	}
	assert(0);
}

void ControlManager::RegisterControlForMouseEvents(Control* control) {
	regions.push_back(std::unique_ptr<PGMouseRegion>(new PGSingleMouseRegion(nullptr, control, nullptr, nullptr)));
}

void ControlManager::UnregisterControlForMouseEvents(Control* control) {
	if (is_destroyed) return;
	for (auto it = regions.begin(); it != regions.end(); it++) {
		if ((*it)->control == control) {
			regions.erase(it);
			return;
		}
	}
	assert(0);
}

void ControlManager::RegisterMouseRegionContainer(MouseRegionSet* r, Control* control) {
	regions.push_back(std::unique_ptr<PGMouseRegion>(new PGMouseRegionContainer(r, control)));
}

void ControlManager::UnregisterMouseRegionContainer(MouseRegionSet* r) {
	if (is_destroyed) return;
	for (auto it = regions.begin(); it != regions.end(); it++) {
		auto container = dynamic_cast<PGMouseRegionContainer*>(it->get());
		if (container && container->regions == r) {
			regions.erase(it);
			return;
		}
	}
	assert(0);
}

void ControlManager::LoadWorkspace(nlohmann::json& j) {
	if (j.count("controlmanager") > 0) {
		nlohmann::json& p = j["controlmanager"];
		if (p.count("projectexplorer_width") > 0 && p["projectexplorer_width"].is_number()) {
			PGScalar pwidth = p["projectexplorer_width"];
			if (pwidth >= active_projectexplorer->minimum_width && pwidth <= this->width) {
				projectexplorer_width = pwidth;
				active_projectexplorer->width = active_projectexplorer->fixed_width = projectexplorer_width;
			}
		}
	}
	PGContainer::LoadWorkspace(j);
}

void ControlManager::WriteWorkspace(nlohmann::json& j) {
	j["controlmanager"] = nlohmann::json::object();
	j["controlmanager"]["projectexplorer_width"] = 
		std::find(controls.begin(), controls.end(), active_projectexplorer) == controls.end() ? 
		projectexplorer_width :
		active_projectexplorer->width;
	PGContainer::WriteWorkspace(j);
}

void ControlManager::RegisterGenericMouseRegion(PGMouseRegion* region) {
	regions.push_back(std::unique_ptr<PGMouseRegion>(region));
}


void ControlManager::DropFile(std::string filename) {
	auto flags = PGGetFileFlags(filename);
	if (flags.is_directory) {
		active_projectexplorer->AddDirectory(filename);
	} else {
		active_textfield->GetTabControl()->OpenFile(filename);
	}
}

ControlManager* GetControlManager(Control* c) {
	return GetWindowManager(c->window);
}

void ControlManager::ShowFindReplace(PGFindTextType type) {
	// Find text
	if (active_findtext) {
		active_findtext->SetType(type);
		SetFocusedControl(active_findtext);
		this->Invalidate();
		return;
	}
	PGFindText* view = new PGFindText(this->window, type);
	this->active_findtext = view;
	view->SetAnchor(PGAnchorBottom | PGAnchorLeft);
	view->bottom_anchor = statusbar;
	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)->bottom_anchor == statusbar) {
			(*it)->bottom_anchor = view;
		}
	}
	this->TriggerResize();
	this->AddControl(view);
	this->Invalidate();
}

void ControlManager::SetFocusedControl(Control* c) {
	if (c->GetControlType() == PGControlTypeTextFieldContainer) {
		if (this->active_textfield != c) {
			this->active_textfield = dynamic_cast<TextFieldContainer*>(c)->textfield;
			ActiveTextFieldChanged(c);
		}
	}
	PGContainer::SetFocusedControl(c);
}

void ControlManager::SetTextFieldLayout(int columns, int rows) {
	std::vector<std::shared_ptr<TextFile>> textfiles;
	textfiles.push_back(std::shared_ptr<TextFile>(new TextFile(nullptr)));
	SetTextFieldLayout(columns, rows, textfiles);
}

void ControlManager::SetTextFieldLayout(int columns, int rows, std::vector<std::shared_ptr<TextFile>> initial_files) {
	int total_textfields = columns * rows;
	int current_textfields = textfields.size();
	if (total_textfields < current_textfields) {
		// have to remove textfields
		// all the tabs of the removed textfields will be moved to the first textfield
		for (lng i = textfields.size() - 1; i >= total_textfields; i--) {
			this->RemoveControl(textfields[i]);
		}
		textfields.erase(textfields.begin() + total_textfields, textfields.end());
	} else if (total_textfields > current_textfields) {
		// have to add textfields
		// the new textfields will be empty
		for (lng i = current_textfields; i < total_textfields; i++) {
			std::vector<std::shared_ptr<TextFile>> textfiles = initial_files;
			TextFieldContainer* container = new TextFieldContainer(this->window, textfiles);
			this->AddControl(container);
			textfields.push_back(container);
		}
	}
	Control* topmost_control = this->toolbar;
	Control* bottom_control = this->active_findtext ? (Control*)this->active_findtext : (Control*)statusbar;
	Control* leftmost_control = this->active_projectexplorer ? this->splitter : nullptr;

	// setup the splitters
	// we have (rows - 1) horizontal splitters, and (columns - 1) vertical splitters
	int splitter_count = (rows - 1) + (columns - 1);
	if (splitter_count > splitters.size()) {
		// have to add splitters
		for (int i = splitters.size(); i < splitter_count; i++) {
			Splitter *splitter = new Splitter(this->window, true);
			splitters.push_back(splitter);
			this->AddControl(splitter);
		}
	} else if (splitter_count < splitters.size()) {
		// have to remove splitters
		for (lng i = splitters.size() - 1; i >= splitter_count; i--) {
			this->RemoveControl(splitters[i]);
		}
		splitters.erase(splitters.begin() + splitter_count, splitters.end());
	}
	// set up the splitters
	for (int i = 0; i < columns - 1; i++) {
		// for each column, we have one horizontal splitter
		splitters[i]->SetAnchor(PGAnchorBottom | PGAnchorLeft);
		splitters[i]->bottom_anchor = bottom_control;
		splitters[i]->top_anchor = topmost_control;
		splitters[i]->left_anchor = nullptr;
		splitters[i]->horizontal = true;
		splitters[i]->fixed_width = 4;
		splitters[i]->percentage_width = -1;
		splitters[i]->percentage_height = 1;
		splitters[i]->fixed_height = -1;
	}
	int base = columns - 1;
	for (int i = 0; i < rows - 1; i++) {
		// for each row, we have one vertical splitter
		splitters[base + i]->SetAnchor(PGAnchorBottom | PGAnchorLeft);
		splitters[base + i]->left_anchor = leftmost_control;
		splitters[base + i]->bottom_anchor = nullptr;
		splitters[base + i]->horizontal = false;
		splitters[base + i]->fixed_height = 4;
		splitters[base + i]->percentage_height = -1;
		splitters[base + i]->percentage_width = 1;
		splitters[base + i]->fixed_width = -1;
	}

	int current_column = 0, current_row = 0;
	for (lng i = 0; i < textfields.size(); i++) {
		TextFieldContainer* container = textfields[i];
		container->SetAnchor(PGAnchorBottom | PGAnchorLeft);
		container->percentage_height = 1.0 / (rows - (rows - current_row) + 1);
		container->percentage_width = 1.0 / (columns - current_column);
		assert(current_row == rows - 1 || i + columns < textfields.size());
		// set up the vertical anchor of the container
		container->top_anchor = topmost_control;
		if (current_row != rows - 1) {
			Splitter* splitter = splitters[base + current_row];
			// the vertical anchor is the splitter below this textfield
			if (splitter->bottom_anchor) {
				splitter->additional_anchors.push_back(splitter->bottom_anchor);
			}
			splitter->bottom_anchor = (Control*)textfields[i + columns];
			container->bottom_anchor = splitter;
		} else {
			// final row, the vertical anchor is the bottom-most control
			container->bottom_anchor = bottom_control;
		}
		assert(current_column == 0 || i > 0);
		// set up the horizontal anchor of the container
		if (current_column != 0) {
			Splitter* splitter = splitters[current_column - 1];
			// the horizontal anchor is the splitter to the left of this textfield
			if (splitter->left_anchor) {
				splitter->additional_anchors.push_back(splitter->left_anchor);
			}
			splitter->left_anchor = (Control*)textfields[i - 1];
			container->left_anchor = splitter;
		} else {
			// left-most row, the horizontal anchor is the left-most control
			container->left_anchor = leftmost_control;
		}

		current_column++;
		if (current_column == columns) {
			current_column = 0;
			current_row++;
		}
	}

	this->rows = rows;
	this->columns = columns;
	this->TriggerResize();
	this->Invalidate();
}

void ControlManager::CreateNewWindow() {
	std::vector<std::shared_ptr<TextFile>> files;
	PGWindowHandle new_window = PGCreateWindow(PGGetWorkspace(window), files);
	ShowWindow(new_window);
}

bool ControlManager::CloseControlManager() {
	if (active_textfield) {
		return active_textfield->GetTabControl()->CloseAllTabs();
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
	args["set_textfield_layout"] = [](Control* c, std::map<std::string, std::string> args) {
		ControlManager* t = (ControlManager*)c;
		int columns = 1, rows = 1;
		if (args.count("columns") > 0) {
			columns = atol(args["columns"].c_str());
			columns = panther::clamp(columns, 1, 3);
		}
		if (args.count("rows") > 0) {
			rows = atol(args["rows"].c_str());
			rows = panther::clamp(rows, 1, 3);
		}
		t->SetTextFieldLayout(columns, rows);
	};
}

void ControlManager::EnterManager() {
#ifdef PANTHER_DEBUG
	entrance_count++;
	//assert(entrance_count == 1);
#endif
}

void ControlManager::LeaveManager() {
#ifdef PANTHER_DEBUG
	entrance_count--;
	//assert(entrance_count == 0);
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

void ControlManager::Draw(PGRendererHandle renderer) {
	EnterManager();
	PGContainer::Draw(renderer);
	LeaveManager();
}

void ControlManager::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	EnterManager();
	PGPoint mouse(x - this->x, y - this->y);
	time_t time = PGGetTime();
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


void ControlManager::LosesFocus(void) {
	EnterManager();
	is_focused = false;
	PGContainer::LosesFocus();
	LeaveManager();
}

void ControlManager::GainsFocus(void) {
	EnterManager();
	is_focused = true;
	PGContainer::GainsFocus();
	LeaveManager();
}

static void UnregisterCallback(std::vector<std::pair<PGControlDataCallback, void*>>& arr, PGControlDataCallback callback, void* data) {
	for (lng i = 0; i < arr.size(); i++) {
		if (arr[i].first == callback && arr[i].second == data) {
			arr.erase(arr.begin() + i);
			return;
		}
	}
}

static void TriggerCallback(std::vector<std::pair<PGControlDataCallback, void*>>& arr, Control* control) {
	for (auto it = arr.begin(); it != arr.end(); it++) {
		(it->first)(control, it->second);
	}
}

void ControlManager::OnSelectionChanged(PGControlDataCallback callback, void* data) {
	this->selection_changed_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void ControlManager::UnregisterOnSelectionChanged(PGControlDataCallback callback, void* data) {
	UnregisterCallback(selection_changed_callbacks, callback, data);
}

void ControlManager::OnTextChanged(PGControlDataCallback callback, void* data) {
	this->text_changed_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void ControlManager::UnregisterOnTextChanged(PGControlDataCallback callback, void* data) {
	UnregisterCallback(text_changed_callbacks, callback, data);
}

void ControlManager::OnActiveTextFieldChanged(PGControlDataCallback callback, void* data) {
	this->active_textfield_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void ControlManager::UnregisterOnActiveTextFieldChanged(PGControlDataCallback callback, void* data) {
	UnregisterCallback(active_textfield_callbacks, callback, data);
}

void ControlManager::OnActiveFileChanged(PGControlDataCallback callback, void* data) {
	this->active_file_callbacks.push_back(std::pair<PGControlDataCallback, void*>(callback, data));
}

void ControlManager::UnregisterOnActiveFileChanged(PGControlDataCallback callback, void* data) {
	UnregisterCallback(active_file_callbacks, callback, data);
}

void ControlManager::TextChanged(Control *control) {
	TriggerCallback(text_changed_callbacks, control);
	this->InvalidateWorkspace();
}

void ControlManager::SelectionChanged(Control *control) {
	TriggerCallback(selection_changed_callbacks, control);
	this->InvalidateWorkspace();
}

void ControlManager::ActiveTextFieldChanged(Control *control) {
	TriggerCallback(active_textfield_callbacks, control);
	this->InvalidateWorkspace();
}

void ControlManager::ActiveFileChanged(Control *control) {
	TextField* field = dynamic_cast<TextField*>(control);
	assert(field);
	std::string text = field->GetTextFile().GetName() + std::string(" - Panther");
	SetWindowTitle(this->window, text.c_str());
	TriggerCallback(active_file_callbacks, control);
	this->InvalidateWorkspace();
}

void ControlManager::InvalidateWorkspace() {
	if (write_workspace_counter == 0) {
		write_workspace_counter = 50;
	}
}

void PGSingleMouseRegion::MouseMove(PGPoint mouse) {
	Control* c = this->control;
	if (this->rect == nullptr) {
		PGIRect rectangle = PGIRect(c->X(), c->Y(), c->width, c->height);
		bool contains = PGRectangleContains(rectangle, mouse);
		if (contains && !this->mouse_inside) {
			c->MouseEnter();
		} else if (this->mouse_inside && !contains) {
			c->MouseLeave();
		}
		this->mouse_inside = contains;
	} else {
		bool contains = PGRectangleContains(*this->rect, mouse - this->control->Position());
		if (!this->mouse_inside && contains) {
			// mouse enter
			this->mouse_event(this->control, true, this->data);
		} else if (this->mouse_inside && !contains) {
			// mouse leave
			this->mouse_event(this->control, false, this->data);
		}
		this->mouse_inside = contains;
	}
}

void PGMouseRegionContainer::MouseMove(PGPoint mouse) {
	Control* c = this->control;
	PGIRect rectangle = PGIRect(c->X(), c->Y(), c->width, c->height);
	bool contains = PGRectangleContains(rectangle, mouse);
	if (contains || (!contains && currently_contained)) {
		this->currently_contained = contains;
		for (auto it = this->regions->begin(); it != this->regions->end(); it++) {
			(*it)->MouseMove(mouse);
		}
	}
}

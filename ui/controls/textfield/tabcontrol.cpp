
#include "tabcontrol.h"
#include "filemanager.h"
#include "style.h"
#include "settings.h"
#include "controlmanager.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(TabControl);

#define MAX_TAB_WIDTH 150.0f
#define BASE_TAB_OFFSET 30.0f
#define EXTRA_TAB_WIDTH 30.0f
#define TAB_SPACING 15.0f

#define SAVE_CHANGES_TITLE "Save Changes?"
#define SAVE_CHANGES_DIALOG "The current file has unsaved changes. Save changes before closing?"

void TabControl::ClearEmptyFlag(Control* c, void* data) {
	TextField* tf = dynamic_cast<TextField*>(c);
	TabControl* tb = static_cast<TabControl*>(data);
	if (tf == tb->GetTextField()) {
		if (tb->temporary_textfile != nullptr && 
			tf->GetTextView().get() == tb->temporary_textfile.get()) {
			// if the user changes text in the temporary file,
			// open the temporary file as permanent file
			tb->OpenTemporaryFileAsActualFile();
		} else if (tb->is_empty) {
			tb->is_empty = false;
		}
	}
}

TabControl::TabControl(PGWindowHandle window, TextField* textfield, std::vector<std::shared_ptr<TextView>> files) :
	Control(window), active_tab(0), textfield(textfield), dragging_tab(nullptr, -1),
	active_tab_hidden(false), drag_tab(false), current_id(0), temporary_textfile(nullptr), current_selection(-1),
	target_scroll(0), scroll_position(0), drag_data(nullptr), temporary_tab_width(0), is_empty(false) {
	if (textfield) {
		textfield->SetTabControl(this);
	}
	if (files.size() == 0) {
		is_empty = true;
		files.push_back(make_shared_control<TextView>(textfield, std::make_shared<TextFile>()));
	}
	
	for (auto it = files.begin(); it != files.end(); it++) {
		auto ptr = FileManager::OpenFile((*it)->file);
		this->tabs.push_back(OpenTab(*it));
	}
	if (textfield->GetTextView() == nullptr) {
		textfield->SetTextView(tabs.front().view);
	}

	this->font = PGCreateFont(PGFontTypeUI);
	SetTextFontSize(this->font, 12);
	file_icon_height = (this->height - 2) * 0.6f;
	file_icon_width = file_icon_height * 0.8f;

	tab_padding = 5;

	auto cm = GetControlManager(this);
	cm->OnTextChanged(ClearEmptyFlag, this);

	cm->RegisterGenericMouseRegion(new PGTabMouseRegion(this));
}

TabControl::~TabControl() {
	auto manager = GetControlManager(this);
	if (manager) {
		manager->UnregisterOnTextChanged(ClearEmptyFlag, this);
		manager->UnregisterControlForMouseEvents(this);
	}
}

Tab TabControl::OpenTab(std::shared_ptr<TextView> view) {
	return Tab(view, current_id++);
}

static bool MovePositionTowards(PGScalar& current, PGScalar target, PGScalar speed) {
	PGScalar original_position = current;
	PGScalar offset = (target - current) / 3;
	if ((current < target && current + offset > target) ||
		(panther::abs(current + offset - target) < 1)) {
		current = target;
	} else {
		current = current + offset;
	}
	return panther::abs(original_position - current) > 0.1;
}

void TabControl::Update() {
	bool invalidate = false;
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		if (MovePositionTowards((*it).x, (*it).target_x, 3)) {
			invalidate = true;
		}
		index++;
	}
	if (MovePositionTowards(scroll_position, target_scroll, 3)) {
		invalidate = true;
	}
	if (invalidate) {
		this->Invalidate();
	}
}

void TabControl::RenderTab(PGRendererHandle renderer, Tab& tab, PGScalar& position_x, PGScalar x, PGScalar y, PGTabType type) {
	TextFile* file = tab.view->file.get();
	assert(file);
	std::string filename = file->GetName();

	PGScalar tab_height = this->height - 2;

	PGScalar current_x = tab.x;

	PGPolygon polygon;
	polygon.points.push_back(PGPoint(x + current_x, y + tab_height));
	polygon.points.push_back(PGPoint(x + current_x + 8, y + 4));
	polygon.points.push_back(PGPoint(x + current_x + tab.width + 15, y + 4));
	polygon.points.push_back(PGPoint(x + current_x + tab.width + 23, y + tab_height));
	polygon.closed = true;

	PGColor background_color = PGStyleManager::GetColor(PGColorTabControlBackground);
	if (tab.hover) {
		background_color = PGStyleManager::GetColor(PGColorTabControlHover);
	} else if (type == PGTabTypeSelected) {
		background_color = PGStyleManager::GetColor(PGColorTabControlSelected);
	} else if (type == PGTabTypeTemporary) {
		background_color = PGStyleManager::GetColor(PGColorTabControlTemporary);
	}

	RenderPolygon(renderer, polygon, background_color);
	polygon.closed = false;
	RenderPolygon(renderer, polygon, PGStyleManager::GetColor(PGColorTabControlBorder), 2);

	current_x += 15;
	std::string ext = file->GetExtension();

	std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

	RenderFileIcon(renderer, font, ext.c_str(), x + current_x, y + (height - file_icon_height) / 2, file_icon_width, file_icon_height,
		file->GetLanguage() ? file->GetLanguage()->GetColor() : PGColor(255, 255, 255), PGStyleManager::GetColor(PGColorTabControlBackground), PGStyleManager::GetColor(PGColorTabControlBorder));

	current_x += file_icon_width + 2.5f;
	if (file->HasUnsavedChanges()) {
		SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlUnsavedText));
	} else {
		SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlText));
	}

	SetTextStyle(font, PGTextStyleBold);
	lng render_start = 0, render_end = filename.size();
	auto widths = CumulativeCharacterWidths(font, filename.c_str(), filename.size(), 0, MAX_TAB_WIDTH - (file_icon_width + 2.5f + 20), render_start, render_end);
	render_end = std::min(render_end, (lng)widths.size());

	RenderText(renderer, font, filename.c_str(), render_end, x + current_x + tab_padding, y + 6);
	current_x += (tab.width - 25);
	if (tab.button_hover) {
		RenderRectangle(renderer, PGRect(x + current_x - 2, y + 11, 12, 12), PGStyleManager::GetColor(PGColorTabControlUnsavedText), PGDrawStyleFill);
	}
	RenderLine(renderer, PGLine(PGPoint(x + current_x, y + 13), PGPoint(x + current_x + 8, y + 21)), PGStyleManager::GetColor(PGColorTabControlText), 1);
	RenderLine(renderer, PGLine(PGPoint(x + current_x, y + 21), PGPoint(x + current_x + 8, y + 13)), PGStyleManager::GetColor(PGColorTabControlText), 1);

	position_x += tab.width + TAB_SPACING;
}

void TabControl::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();
	PGScalar position_x = 0;
	PGColor color = PGStyleManager::GetColor(PGColorMainMenuBackground);
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), color, PGDrawStyleFill);
	bool parent_has_focus = this->parent.lock()->ControlHasFocus();

	bool rendered = false;
	int index = 0;

	max_scroll = 10 + GetTabPosition(tabs.size());
	max_scroll = std::max(max_scroll - (this->width - temporary_tab_width), 0.0f);

	scroll_position = std::min((PGScalar)max_scroll, std::max(0.0f, scroll_position));
	SetScrollPosition(target_scroll);

	SetRenderBounds(renderer, PGRect(x, y, this->width - temporary_tab_width, this->height));
	if (!is_empty) {
		for (auto it = tabs.begin(); it != tabs.end(); it++) {
			if (!active_tab_hidden || index != active_tab) {
				if (dragging_tab.view != nullptr && position_x + it->width / 2 > dragging_tab.x + scroll_position && !rendered) {
					rendered = true;
					position_x += MeasureTabWidth(dragging_tab) + TAB_SPACING;
				}
				it->target_x = position_x;
				if (panther::epsilon_equals(it->x, -1)) {
					it->x = position_x;
				}
				it->width = MeasureTabWidth(*it);
				if (position_x + (it->width + EXTRA_TAB_WIDTH) - scroll_position >= 0) {
					RenderTab(renderer, *it, position_x, x - scroll_position, y, dragging_tab.view == nullptr && temporary_textfile == nullptr && index == active_tab ? PGTabTypeSelected : PGTabTypeNone);
					if (position_x - scroll_position > this->width - temporary_tab_width) {
						// finished rendering
						break;
					}
				} else {
					position_x += it->width + TAB_SPACING;
					it->width = 0;
				}
			}
			index++;
		}
	}

	if (dragging_tab.view != nullptr) {
		position_x = dragging_tab.x;
		RenderTab(renderer, dragging_tab, position_x, x, y, PGTabTypeSelected);
	}
	ClearRenderBounds(renderer);

	if (temporary_textfile) {
		Tab tab = Tab(temporary_textfile, -1);
		tab.width = MeasureTabWidth(tab);
		position_x = this->width - (tab.width + EXTRA_TAB_WIDTH) - BASE_TAB_OFFSET;
		tab.x = position_x;
		tab.target_x = position_x;
		RenderTab(renderer, tab, position_x, x, y, PGTabTypeTemporary);
	}

	if (scroll_position > 0) {
		// render gradient at start if we have scrolled
		RenderGradient(renderer, PGRect(x, y, 10, this->height), PGColor(0, 0, 0, 255), PGColor(0, 0, 0, 0));
	}
	if (index != tabs.size()) {
		// did not render all tabs
		RenderGradient(renderer, PGRect(x + this->width - temporary_tab_width - 4, y, 4, this->height), PGColor(0, 0, 0, 0), PGColor(0, 0, 0, 128));
	}

	RenderLine(renderer, PGLine(x, y + this->height - 2, x + this->width, y + this->height - 1),
		temporary_textfile ? PGStyleManager::GetColor(PGColorTabControlTemporary) : (parent_has_focus ? PGStyleManager::GetColor(PGColorTabControlSelected) : PGStyleManager::GetColor(PGColorTabControlBackground)), 2);
	Control::Draw(renderer);
}

PGScalar TabControl::MeasureTabWidth(Tab& tab) {
	TextFile* file = tab.view->file.get();
	assert(file);
	std::string filename = file->GetName();
	return std::min(MAX_TAB_WIDTH, file_icon_width + 25 + MeasureTextWidth(font, filename.c_str(), filename.size()));
}

bool TabControl::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(TabControl::keybindings, button, modifier)) {
		return true;
	}
	return false;
}

void TabControl::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (buttons & PGLeftMouseButton) {
		if (drag_tab) {
			if (mouse.x >= -dragging_tab.width && mouse.x - drag_offset <= this->width && mouse.y >= -100 & mouse.y <= 100) {
				dragging_tab.x = mouse.x - drag_offset;
				dragging_tab.target_x = dragging_tab.x;
				if (mouse.x <= 20) {
					SetScrollPosition(target_scroll - 20);
				} else if (mouse.x >= this->width - temporary_tab_width) {
					SetScrollPosition(target_scroll + 20);
				}
				this->Invalidate();
			} else {
				drag_tab = false;

				PGBitmapHandle bitmap = nullptr;

				bitmap = CreateBitmapFromSize(MeasureTabWidth(tabs[active_tab]) + EXTRA_TAB_WIDTH, this->height);
				PGRendererHandle renderer = CreateRendererForBitmap(bitmap);
				PGScalar position_x = 0;
				PGScalar stored_x = tabs[active_tab].x;
				tabs[active_tab].x = 0;
				RenderTab(renderer, tabs[active_tab], position_x, 0, 0, PGTabTypeNone);
				tabs[active_tab].x = stored_x;
				DeleteRenderer(renderer);

				if (!drag_data) {
					// if no drag data exists, then this is the tab control that "owns" the dragging tab
					drag_data = new TabDragDropStruct(tabs[active_tab].view, this, drag_offset);
				}
				drag_data->accepted = nullptr;

				PGStartDragDrop(window, bitmap, [](PGPoint mouse, void* d) {
					if (!d) return;
					TabDragDropStruct* data = (TabDragDropStruct*)d;
					if (data->accepted == nullptr) {
						// the data was dropped and nobody accepted it
						// create a new window at the location of the mouse
						std::vector<std::shared_ptr<TextView>> textfiles;
						textfiles.push_back(data->view);
						PGWindowHandle new_window = PGCreateWindow(PGGetWorkspace(data->tabs->window), mouse, textfiles);
						ShowWindow(new_window);
						// close the tab in the original tab control
						data->tabs->active_tab_hidden = false;
						if (data->tabs->tabs.size() == 1) data->tabs->is_empty = true;
						data->tabs->ActuallyCloseTab(data->view);
						delete data;
					}
				}, drag_data, sizeof(TabDragDropStruct));
				drag_data = nullptr;
				dragging_tab.view = nullptr;
				dragging_tab.x = -1;
				dragging_tab.target_x = -1;
			}
		}
	} else {
		if (drag_tab) {
			this->MouseUp(x, y, PGLeftMouseButton, PGModifierNone);
		}
	}
}

void TabControl::SetScrollPosition(PGScalar position) {
	target_scroll = std::min((PGScalar)max_scroll, std::max(0.0f, position));
}

bool TabControl::AcceptsDragDrop(PGDragDropType type) {
	return type == PGDragDropTabs;
}

void TabControl::DragDrop(PGDragDropType type, int x, int y, void* data) {
	TabDragDropStruct* tab_data = (TabDragDropStruct*)data;
	PGPoint mouse(x - this->x, y - this->y);
	assert(type == PGDragDropTabs);
	if (drag_tab || tab_data->accepted != nullptr) return;
	Tab tab = Tab(tab_data->view, -1);
	dragging_tab.width = MeasureTabWidth(tab);
	if (mouse.x >= -dragging_tab.width && mouse.x - tab_data->drag_offset <= this->width && mouse.y >= -100 & mouse.y <= 100) {
		// the tab was dragged in range of this tab control
		// attach the tab to this tab control and add it as the dragging tab
		tab_data->accepted = this;
		// cancel the current drag-drop operation
		PGCancelDragDrop(window);
		dragging_tab.view = tab_data->view;
		dragging_tab.x = -1;
		dragging_tab.target_x = -1;
		drag_offset = tab_data->drag_offset;
		// we save the tab_data, because we need to know the tabcontrol that owns the dragging tab
		// so we can close the tab there if we accept the tab
		this->drag_data = tab_data;
		this->drag_tab = true;
	} else {
		dragging_tab.x = -1;
		dragging_tab.view = nullptr;
	}
	this->Invalidate();
}

void TabControl::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	assert(type == PGDragDropTabs);
	return;
	this->Invalidate();
}

void TabControl::ClearDragDrop(PGDragDropType type) {
	assert(type == PGDragDropTabs);
	if (!drag_tab) {
		dragging_tab.x = -1;
		dragging_tab.view = nullptr;
		this->Invalidate();
	}
}

void TabControl::LoadWorkspace(nlohmann::json& j) {
	if (j.count("tabs") > 0) {
		nlohmann::json& tb = j["tabs"];
		if (tb.count("views") > 0) {
			nlohmann::json& f = tb["views"];
			if (f.is_array() && f.size() > 0) {
				ClearTabs();
				for (auto it = f.begin(); it != f.end(); ++it) {
					if (!it->is_object()) continue;

					PGTextViewSettings settings;

					size_t file_index = 0;
					if (it->count("file_index") > 0 && (*it)["file_index"].is_number_integer()) {
						file_index = (*it)["file_index"].get<size_t>();
					}
					std::shared_ptr<TextFile> file = FileManager::GetFileByIndex(file_index);
					if (!file) {
						continue;
					}

					auto view = make_shared_control<TextView>(textfield, file);

					Cursor::LoadCursors(*it, settings.cursor_data);

					if (it->count("xoffset") > 0) {
						// if xoffset is specified, set the xoffset of the file
						auto xoffset = (*it)["xoffset"];
						if (xoffset.is_number_float()) {
							settings.xoffset = xoffset;
						}
					} else {
						// if xoffset is not specified, word wrap is enabled
						settings.wordwrap = true;
					}
					if (it->count("yoffset") > 0) {
						auto yoffset = (*it)["yoffset"];
						// yoffset can be either a single number (linenumber)
						// or an array of two numbers ([linenumber, inner_line])
						if (yoffset.is_array()) {
							if (yoffset[0].is_number()) {
								settings.yoffset.linenumber = yoffset[0];
							}
							if (yoffset.size() > 1 && yoffset[1].is_number()) {
								settings.yoffset.inner_line = yoffset[1];
							}
						} else if (yoffset.is_number()) {
							settings.yoffset.linenumber = yoffset;
							settings.yoffset.inner_line = 0;
						}
					}

					view->ApplySettings(settings);
					this->tabs.push_back(OpenTab(view));
				}
			}
		}
		if (tabs.size() == 0) {
			is_empty = true;
			auto textfile = std::make_shared<TextFile>();
			FileManager::OpenFile(textfile);
			tabs.push_back(OpenTab(make_shared_control<TextView>(textfield, textfile)));
		}
		if (tb.count("selected_tab") > 0 && tb["selected_tab"].is_number()) {
			// if the selected tab is specified, switch to the selected tab
			lng selected_tab = tb["selected_tab"];
			selected_tab = std::max((lng)0, std::min((lng)tabs.size() - 1, selected_tab));
			SwitchToTab(tabs[selected_tab].view);
		}
		assert(tabs.size() > 0);
		j.erase(j.find("tabs"));
	}
}

void TabControl::WriteWorkspace(nlohmann::json& j) {
	nlohmann::json& tb = j["tabs"];
	nlohmann::json& f = tb["views"];
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		TextView* view = it->view.get();
		assert(view);
		nlohmann::json& cur = f[index];

		auto cursor_data = Cursor::GetCursorData(view->GetCursors());
		Cursor::StoreCursors(cur, cursor_data);

		cur["file_index"] = FileManager::GetFileIndex(view->file);
		if (view->wordwrap) {
			cur["yoffset"] = { view->yoffset.linenumber, view->yoffset.inner_line };
		} else {
			cur["xoffset"] = view->xoffset;
			cur["yoffset"] = view->yoffset.linenumber;
		}
		index++;
	}
	tb["selected_tab"] = active_tab;
}

bool TabControl::KeyboardCharacter(char character, PGModifier modifier) {
	if (this->PressCharacter(TabControl::keybindings, character, modifier)) {
		return true;
	}
	return false;
}

void TabControl::NewTab() {
	if (is_empty) {
		active_tab = -1;
		ClearTabs();
	}
	std::shared_ptr<TextFile> file = FileManager::OpenFile();
	tabs.insert(tabs.begin() + active_tab + 1, OpenTab(make_shared_control<TextView>(textfield, file)));
	active_tab++;
	SwitchToFile(tabs[active_tab].view);
}

void TabControl::OpenFile(std::string path) {
	if (temporary_textfile && temporary_textfile->file->path == path) {
		// if the file is currently opened as temporary file
		// open the temporary file as permanent file
		OpenTemporaryFileAsActualFile();
		return;
	}
	if (!SwitchToTab(path)) {
		PGFileError error;
		auto textfile = FileManager::OpenFile(path, error);
		if (textfile) {
			auto view = make_shared_control<TextView>(textfield, textfile);
			AddTab(view);
		} else if (textfield) {
			this->textfield->DisplayNotification(error);
		}
	}
}

void TabControl::OpenTemporaryFile(std::shared_ptr<TextView> view) {
	bool is_empty = this->is_empty;
	SwitchToFile(view);
	this->temporary_textfile = view;
	this->is_empty = is_empty;
	if (temporary_textfile) {
		Tab tab = Tab(temporary_textfile, -1);
		tab.width = MeasureTabWidth(tab);
		temporary_tab_width = tab.width + EXTRA_TAB_WIDTH + BASE_TAB_OFFSET;
	} else {
		temporary_tab_width = BASE_TAB_OFFSET;
	}
	this->Invalidate();
}

void TabControl::OpenTemporaryFileAsActualFile() {
	FileManager::OpenFile(temporary_textfile->file);
	auto file = temporary_textfile;
	AddTab(file);
	this->temporary_textfile = nullptr;
	this->temporary_tab_width = BASE_TAB_OFFSET;
	this->Invalidate();
}

void TabControl::CloseTemporaryFile() {
	if (!this->temporary_textfile) return;
	auto view = this->temporary_textfile;
	this->temporary_textfile = nullptr;
	this->temporary_tab_width = BASE_TAB_OFFSET;
	if (textfield->GetTextView() == view) {
		SwitchToFile(tabs[active_tab].view);
	}
	this->Invalidate();
}


PGScalar TabControl::GetTabPosition(int tabnr) {
	assert(tabnr <= tabs.size());
	PGScalar position = 0;
	for (lng i = 0; i < tabnr; i++) {
		position += MeasureTabWidth(tabs[i]) + TAB_SPACING;
	}
	return position;
}

void TabControl::SetActiveTab(int active_tab) {
	assert(active_tab >= 0 && active_tab < tabs.size());
	this->active_tab = active_tab;
	PGScalar start = GetTabPosition(active_tab);
	PGScalar width = MeasureTabWidth(tabs[active_tab]) + EXTRA_TAB_WIDTH;
	if (start < target_scroll) {
		SetScrollPosition(start);
	} else if (start + width > target_scroll + this->width - temporary_tab_width) {
		SetScrollPosition(start + width - (this->width - temporary_tab_width));
	}
	SwitchToFile(tabs[active_tab].view);
}


void TabControl::ReopenFile(PGClosedTab tab) {
	PGFileError error;
	std::shared_ptr<TextFile> textfile = FileManager::OpenFile(tab.filepath, error);
	if (textfile) {
		auto view = make_shared_control<TextView>(textfield, textfile);
		view->ApplySettings(tab.settings);
		AddTab(view, tab.id, tab.neighborid);
	} else if (textfield) {
		this->textfield->DisplayNotification(error);
	}
}


void TabControl::OpenFile(std::shared_ptr<TextView> view) {
	std::shared_ptr<TextFile> ptr = FileManager::OpenFile(view->file);
	if (ptr) {
		AddTab(view);
	}
}

void TabControl::OpenFile(std::shared_ptr<TextView> view, lng index) {
	std::shared_ptr<TextFile> ptr = FileManager::OpenFile(view->file);
	if (ptr) {
		AddTab(view, index);
	}
}

void TabControl::AddTab(std::shared_ptr<TextView> file) {
	assert(file);
	assert(active_tab < tabs.size());
	if (is_empty) {
		ClearTabs();
		tabs.push_back(OpenTab(file));
		active_tab = 0;
		is_empty = false;
	} else {
		tabs.insert(tabs.begin() + active_tab + 1, OpenTab(file));
		active_tab++;
	}
	SetActiveTab(active_tab);
}

void TabControl::AddTab(std::shared_ptr<TextView> file, lng index) {
	assert(file);
	assert(index <= tabs.size());
	tabs.insert(tabs.begin() + index, OpenTab(file));
	active_tab = index;
	SetActiveTab(active_tab);
}

void TabControl::AddTab(std::shared_ptr<TextView> file, lng id, lng neighborid) {
	assert(file);
	if (is_empty) {
		tabs.clear();
		tabs.push_back(Tab(file, id));
		active_tab = 0;
	} else if (neighborid < 0) {
		tabs.insert(tabs.begin(), Tab(file, id));
		active_tab = 0;
	} else {
		lng entry = 0;
		for (auto it = tabs.begin(); it != tabs.end(); it++) {
			assert(it->id != id);
			if (it->id == neighborid) {
				break;
			}
			entry++;
		}
		assert(entry != tabs.size());
		tabs.insert(tabs.begin() + entry + 1, Tab(file, id));
		active_tab = entry + 1;
	}
	SetActiveTab(active_tab);
}

int TabControl::GetSelectedTab(PGScalar x) {
	x += scroll_position;
	for (int i = 0; i < tabs.size(); i++) {
		if (x >= tabs[i].x && x <= tabs[i].x + tabs[i].width + TAB_SPACING) {
			return i;
		}
	}
	return -1;
}

void TabControl::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	x -= this->x; y -= this->y;
	if (button == PGLeftMouseButton) {
		if (temporary_textfile && x > this->width - temporary_tab_width) {
			if (click_count > 0) {
				OpenTemporaryFileAsActualFile();
			}
			return;
		}
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			PGScalar offset = x - tabs[selected_tab].x;
			if (offset > tabs[selected_tab].width - 12.5f + file_icon_width) {
				CloseTab(selected_tab);
			} else {
				active_tab = selected_tab;
				SetActiveTab(selected_tab);
				drag_offset = x - tabs[selected_tab].x + scroll_position;

				dragging_tab = tabs[active_tab];
				dragging_tab.x = x - drag_offset;
				dragging_tab.target_x = dragging_tab.x;
				active_tab_hidden = true;

				drag_tab = true;
				this->Invalidate();
			}
			return;
		}
	} else if (button == PGMiddleMouseButton) {
		if (temporary_textfile && x > this->width - temporary_tab_width) {
			CloseTemporaryFile();
		} else {
			int selected_tab = GetSelectedTab(x);
			if (selected_tab >= 0) {
				CloseTab(selected_tab);
				this->Invalidate();
			}
		}
	}
}

void TabControl::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (button & PGLeftMouseButton) {
		if (drag_tab) {
			drag_tab = false;
			PGScalar tab_pos = mouse.x - drag_offset;
			PGScalar position_x = -scroll_position;
			lng index = 0;
			lng current_index = 0;
			lng new_index = active_tab_hidden ? tabs.size() - 1 : tabs.size();
			for (auto it = tabs.begin(); it != tabs.end(); it++, index++) {
				// we skip the active tab if the file is from the current tab control
				// because the active tab is the file we are dragging
				if (active_tab_hidden && index == active_tab) continue;
				PGScalar width = MeasureTabWidth(*it);
				if (position_x + width / 2 > tab_pos) {
					new_index = current_index;
					break;
				}
				position_x += width + TAB_SPACING;
				current_index++;
			}
			if (active_tab_hidden) {
				// if the tab is from this tab control,
				// we only move the tab around
				if (new_index != active_tab) {
					Tab current_tab = tabs[active_tab];
					tabs.erase(tabs.begin() + active_tab);
					tabs.insert(tabs.begin() + new_index, current_tab);
					SetActiveTab(new_index);
				}
				tabs[active_tab].x = tab_pos + scroll_position;
				tabs[active_tab].target_x = tab_pos + scroll_position;
			} else {
				// if the tab is from a different tab control 
				// we have to open the file
				this->OpenFile(dragging_tab.view, new_index);
			}
			active_tab_hidden = false;
			dragging_tab.view = nullptr;
			if (drag_data && drag_data->tabs != this) {
				// we have drag data, and this tab control is not the original tabcontrol
				// we have to close the tab of the original tab control
				if (drag_data->tabs->tabs.size() == 1) drag_data->tabs->is_empty = true;
				drag_data->tabs->ActuallyCloseTab(drag_data->view);
				drag_data->tabs->active_tab_hidden = false;
				delete this->drag_data;
				this->drag_data = nullptr;
			}
			this->Invalidate();
		}
	} else if (button & PGRightMouseButton) {
		PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			this->currently_selected_tab = selected_tab;
			if (selected_tab == active_tab) {
				PGPopupMenuInsertCommand(menu, "Close", "close_tab", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images);
			} else {
				PGPopupMenuInsertEntry(menu, "Close", [](Control* control, PGPopupInformation* info) {
					TabControl* tb = dynamic_cast<TabControl*>(control);
					tb->CloseTab(tb->currently_selected_tab);
					tb->Invalidate();
				});
			}
			PGPopupMenuInsertEntry(menu, "Close Other Tabs", [](Control* control, PGPopupInformation* info) {
				TabControl* tb = dynamic_cast<TabControl*>(control);
				tb->CloseAllTabs(PGDirectionLeft);
				tb->CloseAllTabs(PGDirectionRight);
			});
			PGPopupMenuInsertSeparator(menu);
			PGPopupMenuInsertEntry(menu, "Close Tabs to the Left", [](Control* control, PGPopupInformation* info) {
				TabControl* tb = dynamic_cast<TabControl*>(control);
				tb->CloseAllTabs(PGDirectionLeft);
			});
			PGPopupMenuInsertEntry(menu, "Close Tabs to the Right", [](Control* control, PGPopupInformation* info) {
				TabControl* tb = dynamic_cast<TabControl*>(control);
				tb->CloseAllTabs(PGDirectionRight);
			});
			PGPopupMenuInsertSeparator(menu);
		}
		PGPopupMenuInsertCommand(menu, "New File", "new_tab", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images);
		PGPopupMenuInsertCommand(menu, "Open File", "open_file", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images);
		if (closed_tabs.size() > 0) {
			PGPopupMenuInsertSeparator(menu);
			PGPopupMenuInsertCommand(menu, "Reopen Last Tab", "reopen_last_file", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images);
		}
		PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
	}
}

void TabControl::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	PGScalar scroll_offset = (PGScalar)(std::abs(hdistance) > std::abs(distance) ? hdistance : distance);
	SetScrollPosition(target_scroll + scroll_offset * 2);
	this->Invalidate();
}

void TabControl::PrevTab() {
	active_tab--;
	if (active_tab < 0) {
		active_tab = tabs.size() - 1;
	}
	SetActiveTab(active_tab);
}

void TabControl::NextTab() {
	active_tab++;
	if (active_tab >= tabs.size()) {
		active_tab = 0;
	}
	SetActiveTab(active_tab);
}

bool TabControl::CloseAllTabs() {
	for (size_t i = 0; i < tabs.size(); i++) {
		if (!CloseTabConfirmation(i)) {
			return false;
		}
	}
	return true;
}

void TabControl::ClearTabs() {
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		FileManager::CloseFile(it->view->file);
	}
	tabs.clear();
}

bool TabControl::CloseAllTabs(PGDirection direction) {
	size_t start, end;
	if (direction == PGDirectionLeft) {
		start = 0;
		end = currently_selected_tab;
	} else {
		start = currently_selected_tab + 1;
		end = tabs.size();
	}
	for (size_t i = start; i < end; i++) {
		if (!CloseTabConfirmation(i, false)) {
			return false;
		}
		ActuallyCloseTab(i);
		i--;
		end--;
	}
	return true;
}

bool TabControl::CloseTabConfirmation(int tab, bool respect_hot_exit) {
	if (is_empty) {
		return true;
	}
	bool hot_exit = false;
	if (respect_hot_exit) {
		PGSettingsManager::GetSetting("hot_exit", hot_exit);
	}
	// we only show a confirmation on exit if hot_exit is disabled
	// or if hot_exit is enabled, but the file cannot be saved in the workspace because it is too large
	if (tabs[tab].view->IsLastFileView() && tabs[tab].view->file->HasUnsavedChanges() &&
		(!hot_exit || (hot_exit && tabs[tab].view->file->WorkspaceFileStorage() == TextFile::PGFileTooLarge))) {

		// switch to the tab
		SetActiveTab(tab);
		this->Invalidate();

		// then show the confirmation box
		PGResponse response = PGConfirmationBox(window, SAVE_CHANGES_TITLE, SAVE_CHANGES_DIALOG);
		if (response == PGResponseCancel) {
			return false;
		} else if (response == PGResponseYes) {
			tabs[tab].view->file->SaveChanges();
		}
	}
	return true;
}

void TabControl::ActuallyCloseTab(int tab) {
	bool close_window = false;
	if (tab <= active_tab && active_tab > 0) {
		if (tab == active_tab) {
			SwitchToFile(tabs[active_tab - 1].view);
		}
		active_tab--;
	} else if (tab == active_tab) {
		if (tabs.size() == 1) {
			if (is_empty) {
				close_window = true;
			} else {
				auto ptr = FileManager::OpenFile(std::shared_ptr<TextFile>(new TextFile()));
				tabs.push_back(OpenTab(make_shared_control<TextView>(textfield, ptr)));
				SwitchToFile(tabs[1].view);
				is_empty = true;
			}
		} else {
			SwitchToFile(tabs[1].view);
		}
	}
	if (tabs[tab].view->file->path.size() > 0) {
		closed_tabs.push_back(PGClosedTab(tabs[tab], tab == 0 ? -1 : tabs[tab - 1].id, tabs[tab].view->GetSettings()));
	}
	FileManager::CloseFile(tabs[tab].view->file);
	tabs.erase(tabs.begin() + tab);
	this->Invalidate();
	if (close_window) {
		PGCloseWindow(this->window);
	}
}

void TabControl::CloseTab(int tab) {
	if (!is_empty && tabs[tab].view->IsLastFileView() &&
		tabs[tab].view->file->HasUnsavedChanges()) {
		// if there are unsaved changes, we show a confirmation box
		int* tabnumbers = new int[1];
		tabnumbers[0] = tab;
		// we use a callback here rather than a blocking confirmation box
		// because a blocking confirmation box can launch a sub-event queue on Windows
		// which could lead to concurrency issues
		PGConfirmationBox(window, SAVE_CHANGES_TITLE, SAVE_CHANGES_DIALOG,
			[](PGWindowHandle window, Control* control, void* data, PGResponse response) {
			TabControl* tabs = (TabControl*)control;
			int* tabnumbers = (int*)data;
			int tabnr = tabnumbers[0];

			if (response == PGResponseCancel)
				return;
			if (response == PGResponseYes) {
				tabs->tabs[tabnr].view->file->SaveChanges();
			}
			tabs->ActuallyCloseTab(tabnr);
			delete tabnumbers;
		}, this, (void*)tabnumbers);
	} else {
		ActuallyCloseTab(tab);
	}
}


void TabControl::ActuallyCloseTab(std::shared_ptr<TextView> view) {
	for (size_t i = 0; i < tabs.size(); i++) {
		if (tabs[i].view == view) {
			ActuallyCloseTab(i);
			return;
		}
	}
	assert(0);
}

void TabControl::CloseTab(std::shared_ptr<TextView> view) {
	for (size_t i = 0; i < tabs.size(); i++) {
		if (tabs[i].view == view) {
			CloseTab(i);
			return;
		}
	}
	assert(0);
}

void TabControl::ReopenLastFile() {
	if (closed_tabs.size() == 0) return;

	PGClosedTab tab = closed_tabs.back();
	closed_tabs.pop_back();

	this->ReopenFile(tab);
}

void TabControl::SwitchToTab(std::shared_ptr<TextView> view) {
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].view == view) {
			SetActiveTab(i);
			this->Invalidate();
			return;
		}
	}
	assert(0);
}

bool TabControl::SwitchToTab(std::string path) {
	if (temporary_textfile && temporary_textfile->file->path == path) {
		return true;
	}
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].view->file->path == path) {
			SetActiveTab(i);
			this->Invalidate();
			return true;
		}
	}
	return false;
}


void TabControl::SwitchToFile(std::shared_ptr<TextView> view) {
	ControlManager* cm = GetControlManager(this);
	if (cm->active_projectexplorer) {
		cm->active_projectexplorer->RevealFile(view->file->GetFullPath(), true);
	}
	if (textfield->GetTextView() == view) return;

	textfield->SetTextView(view);
	if (temporary_textfile && view != temporary_textfile) {
		CloseTemporaryFile();
	}
	this->Invalidate();
}

void TabControl::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = TabControl::keybindings_noargs;
	noargs["open_file"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		std::vector<std::string> files = ShowOpenFileDialog(true, false, true);
		for (auto it = files.begin(); it != files.end(); it++) {
			t->OpenFile(*it);
		}
		t->Invalidate();
	};
	noargs["close_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		if (t->temporary_textfile != nullptr) {
			t->CloseTemporaryFile();
		} else {
			t->CloseTab(t->active_tab);
		}
		t->Invalidate();
	};
	noargs["new_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->NewTab();
		t->Invalidate();
	};
	noargs["next_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->NextTab();
		t->Invalidate();
	};
	noargs["prev_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->PrevTab();
		t->Invalidate();
	};
	noargs["reopen_last_file"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->ReopenLastFile();
		t->Invalidate();
	};
}

void PGTabMouseRegion::MouseMove(PGPoint mouse) {
	PGIRect rectangle = PGIRect(tabs->X(), tabs->Y(), tabs->width, tabs->height);
	bool contains = PGRectangleContains(rectangle, mouse);

	if (!contains && currently_contains) {
		for (auto it = tabs->tabs.begin(); it != tabs->tabs.end(); it++) {
			it->hover = false;
			it->button_hover = false;
		}
		tabs->Invalidate();
	}
	currently_contains = contains;
	if (!contains) return;

	PGScalar tab_height = tabs->height - 2;

	mouse.x -= tabs->X() - tabs->scroll_position; mouse.y -= tabs->Y();
	bool invalidated = false;
	for (auto it = tabs->tabs.begin(); it != tabs->tabs.end(); it++) {
		bool current_hover = it->hover;
		if (it->width == 0) {
			it->hover = false;
			it->button_hover = false;
		} else {
			PGRect rect = PGRect(it->x, 4, it->width + TAB_SPACING, tab_height);
			bool contains = PGRectangleContains(rect, mouse);
			it->hover = contains;
			if (contains) {
				PGRect rect = PGRect(it->x + it->width - 12.5f + tabs->file_icon_width, 10, 16, 22);
				bool current_hover = it->button_hover;
				it->button_hover = PGRectangleContains(rect, mouse);
				if (it->button_hover != current_hover) {
					invalidated = true;
				}
				it->hover = true;
			} else {
				it->button_hover = false;
			}
		}
		if (current_hover != it->hover) {
			invalidated = true;
		}
	}
	if (invalidated) {
		tabs->Invalidate();
	}
}

void TabControl::OnResize(PGSize old_size, PGSize new_size) {
	file_icon_height = (new_size.height - 2) * 0.6f;
	file_icon_width = file_icon_height * 0.8f;
}



#include "tabcontrol.h"
#include "filemanager.h"


TabControl::TabControl(PGWindowHandle window, TextField* textfield) : Control(window, false), active_tab(0), textfield(textfield) {
	std::vector<TextFile*>& files = FileManager::GetFiles();
	for (auto it = files.begin(); it != files.end(); it++) {
		this->tabs.push_back(Tab(*it));
	}
}

void TabControl::PeriodicRender() {
	bool invalidate = false;
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		if (index == active_tab && drag_tab) {
			index++;
			continue;
		}
		PGScalar current_x = (*it).x;
		PGScalar offset = ((*it).target_x - (*it).x) / 3;
		if (((*it).x < (*it).target_x && (*it).x + offset > (*it).target_x) || 
			(std::abs((*it).x + offset - (*it).target_x) < 1)) {
			(*it).x = (*it).target_x;
		} else {
			(*it).x = (*it).x < 0 ? (*it).target_x : (*it).x + offset;
		}
		if (std::abs(current_x - (*it).x) > 0.1)
			invalidate = true;
		index++;
	}
	if (invalidate) {
		this->Invalidate();
	}
}

void TabControl::Draw(PGRendererHandle renderer, PGIRect* rectangle) {
	PGScalar position_x = this->x - rectangle->x;
	PGScalar position_y = this->y - rectangle->y;
	PGScalar tab_padding = 5;
	PGColor color = PGColor(0, 102, 204);
	SetTextFont(renderer, nullptr, 13);
	SetTextColor(renderer, PGColor(255, 255, 255));
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		TextFile* file = (*it).file;
		std::string filename = file->GetName();
		(*it).target_x = position_x;
		(*it).width = MeasureTextWidth(renderer, filename.c_str(), filename.size());
		if (index != active_tab) {
			PGRect rect((*it).x, position_y, (*it).width + tab_padding * 2, this->height);
			RenderText(renderer, filename.c_str(), filename.length(), (*it).x + tab_padding, position_y);
		}
		position_x += (*it).width + tab_padding * 2;
		index++;
	}
	// render the active tab
	if (active_tab >= 0) {
		// the active tab is rendered last so it appears on top of other tabs when being dragged
		TextFile* file = tabs[active_tab].file;
		std::string filename = file->GetName();
		PGRect rect(tabs[active_tab].x, position_y, tabs[active_tab].width + tab_padding * 2, this->height);
		RenderRectangle(renderer, rect, color, PGStyleFill);
		RenderText(renderer, filename.c_str(), filename.length(), tabs[active_tab].x + tab_padding, position_y);

	}
	RenderLine(renderer, PGLine(0, this->height - 1 - rectangle->y, this->width, this->height - 1 - rectangle->y), PGColor(80, 150, 200));
}

bool TabControl::KeyboardButton(PGButton button, PGModifier modifier) {
	switch (button) {
	case PGButtonTab:
		if (modifier == PGModifierCtrlShift) {
			PrevTab();
			this->Invalidate();
			return true;
		} else if (modifier == PGModifierCtrl) {
			NextTab();
			this->Invalidate();
			return true;
		}
	default:
		return false;
	}
}

void TabControl::MouseMove(int x, int y, PGMouseButton buttons) {
	if (buttons & PGLeftMouseButton) {
		tabs[active_tab].x = x - drag_offset;
		if (active_tab != 0) {
			Tab left_tab = tabs[active_tab - 1];
			if (tabs[active_tab].x < (left_tab.target_x + left_tab.width / 4)) {
				Tab current_tab = tabs[active_tab];
				// switch tabs
				tabs.erase(tabs.begin() + active_tab);
				tabs.insert(tabs.begin() + active_tab - 1, current_tab);
				active_tab--;
				tabs[active_tab].target_x = tabs[active_tab + 1].target_x;
				tabs[active_tab + 1].target_x = tabs[active_tab].target_x + tabs[active_tab].width;
			}
		}
		if (active_tab != tabs.size() - 1) {
			Tab right_tab = tabs[active_tab + 1];
			if ((tabs[active_tab].x + tabs[active_tab].width) > (right_tab.target_x + right_tab.width / 4)) {
				Tab current_tab = tabs[active_tab];
				// switch tabs
				tabs.erase(tabs.begin() + active_tab);
				tabs.insert(tabs.begin() + active_tab + 1, current_tab);
				active_tab++;
				tabs[active_tab - 1].target_x = tabs[active_tab].target_x;
				tabs[active_tab].target_x = tabs[active_tab - 1].target_x + tabs[active_tab - 1].width;
			}
		}
		this->Invalidate();
	} else {
		drag_tab = false;
	}
}

int TabControl::GetSelectedTab(int x) {
	for (int i = 0; i < tabs.size(); i++) {
		if (x >= tabs[i].x && x <= tabs[i].x + tabs[i].width) {
			return i;
		}
	}
	return -1;
}

void TabControl::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (button == PGLeftMouseButton) {
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			active_tab = selected_tab;
			SwitchToFile(tabs[selected_tab].file);
			drag_offset = x - tabs[selected_tab].x;
			drag_tab = true;
			return;
		}
	} else if (button == PGMiddleMouseButton) {
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			if (selected_tab == active_tab && active_tab > 0) {
				active_tab--;
				SwitchToFile(tabs[active_tab].file);
			} else if (selected_tab == active_tab) {
				if (tabs.size() == 1) {
					// open an in-memory file to replace the current file
					TextFile* file = FileManager::OpenFile();
					tabs.push_back(Tab(file));
				}
				SwitchToFile(tabs[1].file);
			}
			FileManager::CloseFile(tabs[selected_tab].file);
			tabs.erase(tabs.begin() + selected_tab);
			this->Invalidate();
		}
	}
}

void TabControl::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (button == PGLeftMouseButton) {
		drag_tab = false;
		this->Invalidate();
	}
}

void TabControl::PrevTab() {
	std::vector<TextFile*> files = FileManager::GetFiles();
	active_tab--;
	if (active_tab < 0) active_tab = files.size() - 1;

	SwitchToFile(files[active_tab]);
}

void TabControl::NextTab() {
	std::vector<TextFile*> files = FileManager::GetFiles();
	active_tab++;
	if (active_tab >= files.size()) active_tab = 0;

	SwitchToFile(files[active_tab]);
}

void TabControl::CloseTab() {

}

void TabControl::SwitchToFile(TextFile* file) {
	textfield->SetTextFile(file);
}

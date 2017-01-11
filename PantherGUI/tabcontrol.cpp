
#include "tabcontrol.h"
#include "filemanager.h"
#include "style.h"


TabControl::TabControl(PGWindowHandle window, TextField* textfield) : 
	Control(window), active_tab(0), textfield(textfield) {
	std::vector<TextFile*>& files = FileManager::GetFiles();
	for (auto it = files.begin(); it != files.end(); it++) {
		this->tabs.push_back(Tab(*it));
	}
	this->font = PGCreateFont("myriad", false, true);
	SetTextFontSize(this->font, 13);
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
			(panther::abs((*it).x + offset - (*it).target_x) < 1)) {
			(*it).x = (*it).target_x;
		} else {
			(*it).x = (*it).x < 0 ? (*it).target_x : (*it).x + offset;
		}
		if (panther::abs(current_x - (*it).x) > 0.1)
			invalidate = true;
		index++;
	}
	if (invalidate) {
		this->Invalidate();
	}
}

void TabControl::RenderTab(PGRendererHandle renderer, Tab& tab, PGScalar& position_x, PGScalar x, PGScalar y, bool selected_tab) {
	TextFile* file = tab.file;
	std::string filename = file->GetName();

	const PGScalar tab_padding = 5;
	const PGScalar file_icon_height = this->height * 0.8f;
	const PGScalar file_icon_width = file_icon_height * 0.8f;

	tab.target_x = position_x;
	tab.width = file_icon_width + 5 + MeasureTextWidth(font, filename.c_str(), filename.size());
	PGScalar current_x = tab.x;
	PGRect rect(x + current_x, y, tab.width + tab_padding * 2, this->height);
	current_x += 2.5f;
	RenderRectangle(renderer, rect, 
		selected_tab ? PGStyleManager::GetColor(PGColorTabControlSelected) : 
		               PGStyleManager::GetColor(PGColorTabControlBackground), 
		PGDrawStyleFill);

	lng pos = filename.find_last_of('.');
	std::string ext = pos == std::string::npos ? std::string("") : filename.substr(pos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
	RenderFileIcon(renderer, font, ext.c_str(), current_x, y + (height - file_icon_height) / 2, file_icon_width, file_icon_height,
		file->GetLanguage() ? file->GetLanguage()->GetColor() : PGColor(255,255,255), PGColor(30, 30, 30), PGColor(91, 91, 91));
	current_x += file_icon_width + 2.5f;
	if (file->HasUnsavedChanges()) {
		SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlUnsavedText));
	} else {
		SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlText));
	}
	RenderText(renderer, font, filename.c_str(), filename.length(), x + current_x + tab_padding, y);
	position_x += tab.width + tab_padding * 2;
}

void TabControl::Draw(PGRendererHandle renderer, PGIRect* rectangle) {
	PGScalar x = X() - rectangle->x;
	PGScalar y = Y() - rectangle->y;
	PGScalar position_x = 0;
	PGColor color = PGStyleManager::GetColor(PGColorTabControlBackground);

	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		RenderTab(renderer, *it, position_x, x, y, index == active_tab);
		index++;
	}
	/*
	// render the active tab
	if (active_tab >= 0) {
		// the active tab is rendered last so it appears on top of other tabs when being dragged
		TextFile* file = tabs[active_tab].file;
		std::string filename = file->GetName();
		PGRect rect(x + tabs[active_tab].x, y, tabs[active_tab].width + tab_padding * 2, this->height);
		RenderRectangle(renderer, rect, PGStyleManager::GetColor(PGColorTabControlSelected), PGDrawStyleFill);
		if (file->HasUnsavedChanges()) {
			SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlUnsavedText));
		} else {
			SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlText));
		}
		RenderText(renderer, font, filename.c_str(), filename.length(), x +tabs[active_tab].x + tab_padding, y);
	}*/
	// FIXME: color
	RenderLine(renderer, PGLine(x, y + this->height - 1, x + this->width, y + this->height - 1), PGColor(80, 150, 200));
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
		break;
	case PGButtonPageUp:
		if (modifier == PGModifierCtrl) {
			PrevTab();
			this->Invalidate();
			return true;
		}
		break;
	case PGButtonPageDown:
		if (modifier == PGModifierCtrl) {
			NextTab();
			this->Invalidate();
			return true;
		}
		break;
	default:
		return false;
	}
	return false;
}

void TabControl::MouseMove(int x, int y, PGMouseButton buttons) {
	x -= this->x; y -= this->y;
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

bool TabControl::KeyboardCharacter(char character, PGModifier modifier) {
	if (modifier & PGModifierCtrl) {
		switch (character) {
		case 'W':
			CloseTab(active_tab);
			this->Invalidate();
			return true;
		case 'N':
			NewTab();
			this->Invalidate();
			return true;
		}
	}
	return false;
}

void TabControl::NewTab() {
	TextFile* file = FileManager::OpenFile();
	tabs.insert(tabs.begin() + active_tab + 1, Tab(file));
	active_tab++;
	SwitchToFile(tabs[active_tab].file);
}

void TabControl::AddTab(TextFile* file) {
	assert(file);
	tabs.insert(tabs.begin() + active_tab + 1, Tab(file));
	active_tab++;
	SwitchToFile(tabs[active_tab].file);
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
	x -= this->x; y -= this->y;
	if (button == PGLeftMouseButton) {
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			active_tab = selected_tab;
			SwitchToFile(tabs[selected_tab].file);
			drag_offset = x - tabs[selected_tab].x;
			drag_tab = true;
			this->Invalidate();
			return;
		}
	} else if (button == PGMiddleMouseButton) {
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			CloseTab(selected_tab);
			this->Invalidate();
		}
	}
}

void TabControl::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	x -= this->x; y -= this->y;
	if (button == PGLeftMouseButton) {
		drag_tab = false;
		this->Invalidate();
	} else if (button & PGRightMouseButton) {
		PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			this->currently_selected_tab = selected_tab;
			PGPopupMenuInsertEntry(menu, "Close",  [](Control* control) {
				TabControl* tb = dynamic_cast<TabControl*>(control);
				tb->CloseTab(tb->currently_selected_tab);
				tb->Invalidate();
			});
			PGPopupMenuInsertEntry(menu, "Close Other Tabs",  [](Control* control) {
			}, PGPopupMenuGrayed);
			PGPopupMenuInsertEntry(menu, "Close Tabs to the Right",  [](Control* control) {
			}, PGPopupMenuGrayed);
			PGPopupMenuInsertSeparator(menu);
		}
		PGPopupMenuInsertEntry(menu, "New File",  [](Control* control) {
			dynamic_cast<TabControl*>(control)->NewTab();
			control->Invalidate();
		});
		PGPopupMenuInsertEntry(menu, "Open File",  [](Control* control) {
		}, PGPopupMenuGrayed);
		PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
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

void TabControl::CloseTab(int tab) {
	if (tab == active_tab && active_tab > 0) {
		active_tab--;
		SwitchToFile(tabs[active_tab].file);
	} else if (tab == active_tab) {
		if (tabs.size() == 1) {
			// open an in-memory file to replace the current file
			TextFile* file = FileManager::OpenFile();
			tabs.push_back(Tab(file));
		}
		SwitchToFile(tabs[1].file);
	}
	FileManager::CloseFile(tabs[tab].file);
	tabs.erase(tabs.begin() + tab);
}

void TabControl::SwitchToFile(TextFile* file) {
	textfield->SetTextFile(file);
}

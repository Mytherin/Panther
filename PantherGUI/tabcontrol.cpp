
#include "tabcontrol.h"
#include "filemanager.h"


TabControl::TabControl(PGWindowHandle window, TextField* textfield) : Control(window, false), active_tab(0), textfield(textfield) {

}

void TabControl::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGScalar position_x = this->x;
	PGScalar position_y = this->y;
	PGScalar tab_padding = 5;
	std::vector<TextFile*>& files  = FileManager::GetFiles();
	PGColor color = PGColor(0, 102, 204);
	SetTextFont(renderer, nullptr, 13);
	SetTextColor(renderer, PGColor(255, 255, 255));
	int index = 0;
	for (auto it = files.begin(); it != files.end(); it++) {
		std::string filename = (*it)->GetName();
		PGScalar width = MeasureTextWidth(renderer, filename.c_str(), filename.size());
		PGRect rect(position_x, position_y, width + tab_padding * 2, this->height);
		if (index == active_tab) {
			RenderRectangle(renderer, rect, color, PGStyleFill);
		}
		RenderText(renderer, filename.c_str(), filename.length(), position_x + tab_padding, position_y);
		position_x += width + tab_padding * 2;
		index++;
	}
	RenderLine(renderer, PGLine(0, this->height - 1, this->width, this->height - 1), PGColor(80, 150, 200));
}

bool TabControl::KeyboardButton(PGButton button, PGModifier modifier) {
	switch (button) {
	case PGButtonTab:
		if (modifier == PGModifierCtrlShift) {
			PrevTab();
		} else if (modifier == PGModifierCtrl) {
			NextTab();
		}
		return true;
	default:
		return false;
	}
}

void TabControl::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void TabControl::MouseMove(int x, int y, PGMouseButton buttons) {

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

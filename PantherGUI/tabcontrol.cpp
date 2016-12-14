
#include "tabcontrol.h"
#include "filemanager.h"


TabControl::TabControl(PGWindowHandle window) : Control(window, true) {

}

void TabControl::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGScalar position_x = this->x;
	PGScalar position_y = this->y;
	PGScalar tab_padding = 5;
	std::vector<TextFile*>& files  = FileManager::GetFiles();
	PGColor color = PGColor(66, 66, 66);
	SetTextFont(renderer, nullptr, 13);
	SetTextColor(renderer, PGColor(191, 191, 191));
	for (auto it = files.begin(); it != files.end(); it++) {
		std::string filename = (*it)->GetName();
		PGScalar width = MeasureTextWidth(renderer, filename.c_str(), filename.size());
		RenderRectangle(renderer, PGRect(position_x, position_y, width + tab_padding * 2, this->height), color, PGStyleFill);
		RenderText(renderer, filename.c_str(), filename.length(), position_x + tab_padding, position_y);
	}
	RenderLine(renderer, PGLine(0, this->height - 1, this->width, this->height - 1), PGColor(80, 150, 200));
}
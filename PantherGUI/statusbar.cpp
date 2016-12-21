
#include "statusbar.h"

StatusBar::StatusBar(PGWindowHandle window, TextField* textfield) : active_textfield(textfield), Control(window, false) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGColor(255, 255, 255));
}

void StatusBar::UpdateParentSize(PGSize old_size, PGSize new_size) {
	Control::UpdateParentSize(old_size, new_size);
	this->height = STATUSBAR_HEIGHT;
	this->y = parent->height - this->height;
}

void StatusBar::SelectionChanged() {

}

void StatusBar::Draw(PGRendererHandle renderer, PGIRect* rect) {
	using namespace std;
	RenderRectangle(renderer, PGRect(this->x - rect->x, this->y - rect->y, this->width, this->height), PGColor(0, 122, 204), PGStyleFill);
	if (active_textfield) {
		TextFile& file = active_textfield->GetTextFile();
		auto cursors = file.GetCursors();
		std::string str = "";
		if (cursors.size() == 1) {
			str = string_sprintf("Line %lld, Column %lld", cursors[0]->SelectedLine() + 1, cursors[0]->SelectedCharacter() + 1);

			if (!cursors[0]->SelectionIsEmpty()) {
				if (cursors[0]->BeginLine() == cursors[0]->EndLine()) {
					str += string_sprintf(" (%lld characters selected)", cursors[0]->EndCharacter() - cursors[0]->BeginCharacter());
				} else {
					str += string_sprintf(" (%lld lines selected)", cursors[0]->EndLine() - cursors[0]->BeginLine() + 1);
				}
			}
		} else {
			str = to_string(cursors.size()) + string(" selected regions");
		}
		RenderText(renderer, font, str.c_str(), str.size(), this->x + 10, this->y - rect->y);
	}
}
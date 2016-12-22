
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
		if (file.IsLoaded()) {
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
			const int padding = 20;
			int right_position = padding;

			str = "UTF-8";
			right_position += RenderText(renderer, font, str.c_str(), str.size(), this->x + this->width - right_position, this->y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(this->x + this->width - right_position, this->y - rect->y, this->x + this->width - right_position, this->y - rect->y + this->height), PGColor(255, 255, 255));
			right_position += padding;

			PGLanguage* language = file.GetLanguage();
			str = language ? language->GetName() : "Plain Text";
			right_position += RenderText(renderer, font, str.c_str(), str.size(), this->x + this->width - right_position, this->y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(this->x + this->width - right_position, this->y - rect->y, this->x + this->width - right_position, this->y - rect->y + this->height), PGColor(255, 255, 255));
			right_position += padding;

			PGLineEnding ending = file.GetLineEnding();
			switch (ending) {
			case PGLineEndingWindows:
				str = "CRLF";
				break;
			case PGLineEndingUnix:
				str = "LF";
				break;
			case PGLineEndingMacOS:
				str = "CR";
				break;
			default:
				str = "Mixed";
			}
			right_position += RenderText(renderer, font, str.c_str(), str.size(), this->x + this->width - right_position, this->y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(this->x + this->width - right_position, this->y - rect->y, this->x + this->width - right_position, this->y - rect->y + this->height), PGColor(255, 255, 255));
			right_position += padding;

			PGLineIndentation indentation = file.GetLineIndentation();
			switch (indentation) {
			case PGIndentionSpaces:
				str = "Spaces: ";
				break;
			case PGIndentionTabs:
				str = "Tab-Width: ";
				break;
			default:
				str = "Mixed: ";
			}
			str += to_string(4);
			right_position += RenderText(renderer, font, str.c_str(), str.size(), this->x + this->width - right_position, this->y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(this->x + this->width - right_position, this->y - rect->y, this->x + this->width - right_position, this->y - rect->y + this->height), PGColor(255, 255, 255));
			right_position += padding;
		}
	}
}

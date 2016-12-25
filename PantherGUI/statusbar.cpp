
#include "statusbar.h"
#include "encoding.h"
#include "style.h"

StatusBar::StatusBar(PGWindowHandle window, TextField* textfield) : active_textfield(textfield), Control(window, false) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
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
	PGScalar x = X(), y = Y();
	RenderRectangle(renderer, PGRect(x - rect->x, y - rect->y, this->width, this->height), PGStyleManager::GetColor(PGColorStatusBarBackground), PGDrawStyleFill);
	if (active_textfield) {
		TextFile& file = active_textfield->GetTextFile();
		if (file.IsLoaded()) {
			PGColor line_color = PGStyleManager::GetColor(PGColorStatusBarText);
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
			RenderText(renderer, font, str.c_str(), str.size(), x + 10, y - rect->y);
			const int padding = 20;
			int right_position = padding;

			PGFileEncoding encoding = file.GetFileEncoding();
			str = PGEncodingToString(encoding);
			right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			buttons[0] = PGIRect(this->width - right_position, 0, 2 * padding + MeasureTextWidth(font, str.c_str(), str.size()), this->height);
			right_position += padding;

			PGLanguage* language = file.GetLanguage();
			str = language ? language->GetName() : "Plain Text";
			right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			buttons[1] = PGIRect(this->width - right_position, 0, 2 * padding + MeasureTextWidth(font, str.c_str(), str.size()), this->height);
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
			right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			buttons[2] = PGIRect(this->width - right_position, 0, 2 * padding + MeasureTextWidth(font, str.c_str(), str.size()), this->height);
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
			right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
			right_position += padding;
			RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			buttons[3] = PGIRect(this->width - right_position, 0, 2 * padding + MeasureTextWidth(font, str.c_str(), str.size()), this->height);
			right_position += padding;
		}
	}
}

void StatusBar::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void StatusBar::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (button & PGLeftMouseButton) {
		TextFile& file = active_textfield->GetTextFile();
		if (PGRectangleContains(buttons[0], mouse)) {

			PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
			PGPopupMenuHandle reopen_menu = PGCreatePopupMenu(this->window, this);
			PGPopupMenuHandle savewith_menu = PGCreatePopupMenu(this->window, this);
			{
				PGPopupMenuInsertEntry(reopen_menu, "UTF-8", [](Control* control) {
					// FIXME: reopen with encoding
				}, PGPopupMenuGrayed);
				PGPopupMenuInsertEntry(savewith_menu, "UTF-8", [](Control* control) {
					// FIXME: save with encoding and change active encoding
				}, PGPopupMenuGrayed);
			}
			PGPopupMenuInsertSubmenu(menu, reopen_menu, "Reopen with Encoding...");
			PGPopupMenuInsertSubmenu(menu, savewith_menu, "Save with Encoding...");

			PGDisplayPopupMenu(menu, ConvertWindowToScreen(this->window, PGPoint(X() + buttons[0].x + buttons[0].width, Y() + buttons[0].y)), PGTextAlignRight | PGTextAlignBottom);
		} else if (PGRectangleContains(buttons[1], mouse)) {
			PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
			auto languages = PGLanguageManager::GetLanguages();
			auto active_language = file.GetLanguage();
			for (auto it = languages.begin(); it != languages.end(); it++) {
				PGPopupMenuInsertEntry(menu, (*it)->GetName().c_str(), [](Control* control) {
					// FIXME: switch highlighter language
				}, active_language == *it ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
			}
			PGDisplayPopupMenu(menu, ConvertWindowToScreen(this->window, PGPoint(X() + buttons[1].x + buttons[1].width, Y() + buttons[1].y)), PGTextAlignRight | PGTextAlignBottom);
		} else if (PGRectangleContains(buttons[2], mouse)) {
			PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
			PGPopupMenuInsertEntry(menu, "Windows (\\r\\n)", [](Control* control) {
				TextFile& file = dynamic_cast<StatusBar*>(control)->active_textfield->GetTextFile();
				file.ChangeLineEnding(PGLineEndingWindows);
			}, file.GetLineEnding() == PGLineEndingWindows ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
			PGPopupMenuInsertEntry(menu, "Unix (\\n)", [](Control* control) {
				TextFile& file = dynamic_cast<StatusBar*>(control)->active_textfield->GetTextFile();
				file.ChangeLineEnding(PGLineEndingUnix);
			}, file.GetLineEnding() == PGLineEndingUnix ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
			PGPopupMenuInsertEntry(menu, "Mac OS 9 (\\r)", [](Control* control) {
				TextFile& file = dynamic_cast<StatusBar*>(control)->active_textfield->GetTextFile();
				file.ChangeLineEnding(PGLineEndingMacOS);
			}, file.GetLineEnding() == PGLineEndingMacOS ? PGPopupMenuChecked : PGPopupMenuFlagsNone);

			PGDisplayPopupMenu(menu, ConvertWindowToScreen(this->window, PGPoint(X() + buttons[2].x + buttons[2].width, Y() + buttons[2].y)), PGTextAlignRight | PGTextAlignBottom);
		} else if (PGRectangleContains(buttons[3], mouse)) {
			PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);

			PGPopupMenuInsertEntry(menu, "Indent Using Spaces", [](Control* control) {
				// FIXME: change indentation of file and convert
			}, file.GetLineIndentation() == PGIndentionSpaces ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
			PGPopupMenuInsertSeparator(menu);
			for (int i = 1; i <= 8; i++) {
				std::string header = "Tab Width: " + std::to_string(i);
				PGPopupMenuInsertEntry(menu, header, [](Control* control) {
					// FIXME: change tab width
				});
			}
			PGPopupMenuInsertSeparator(menu);
			PGPopupMenuInsertEntry(menu, "Convert Indentation To Spaces", [](Control* control) {
				// FIXME: change indentation of file
			});
			PGPopupMenuInsertEntry(menu, "Convert Indentation To Tabs", [](Control* control) {
				// FIXME: change indentation of file
			});
			PGDisplayPopupMenu(menu, ConvertWindowToScreen(this->window, PGPoint(X() + buttons[3].x + buttons[3].width, Y() + buttons[3].y)), PGTextAlignRight | PGTextAlignBottom);
		} 
	}
}


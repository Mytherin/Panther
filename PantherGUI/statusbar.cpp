
#include "statusbar.h"
#include "encoding.h"
#include "style.h"

StatusBar::StatusBar(PGWindowHandle window, TextField* textfield) :
	active_textfield(textfield), Control(window, false),
	buttons{ Button(this), Button(this), Button(this), Button(this) } {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
	textfield->OnSelectionChanged([](Control* c, void* data) {
		((StatusBar*)(data))->SelectionChanged();
	}, (void*) this);

	buttons[0].OnPressed([](Button* b) {
		Control* c = b->parent;
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);
		PGPopupMenuHandle reopen_menu = PGCreatePopupMenu(c->window, c);
		PGPopupMenuHandle savewith_menu = PGCreatePopupMenu(c->window, c);
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

		PGDisplayPopupMenu(menu, ConvertWindowToScreen(c->window,
			PGPoint(c->X() + b->region.x + b->region.width - 1,
				c->Y() + b->region.y)), PGTextAlignRight | PGTextAlignBottom);
	});

	buttons[1].OnPressed([](Button* b) {
		Control* c = b->parent;
		TextFile& file = ((StatusBar*)c)->active_textfield->GetTextFile();
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);
		auto languages = PGLanguageManager::GetLanguages();
		auto active_language = file.GetLanguage();
		for (auto it = languages.begin(); it != languages.end(); it++) {
			PGPopupMenuInsertEntry(menu, (*it)->GetName().c_str(), [](Control* control) {
				// FIXME: switch highlighter language
			}, active_language == *it ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		}
		PGDisplayPopupMenu(menu, ConvertWindowToScreen(c->window,
			PGPoint(c->X() + b->region.x + b->region.width - 1,
				c->Y() + b->region.y)), PGTextAlignRight | PGTextAlignBottom);
	});

	buttons[2].OnPressed([](Button* b) {
		Control* c = b->parent;
		TextFile& file = ((StatusBar*)c)->active_textfield->GetTextFile();
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);
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

		PGDisplayPopupMenu(menu, ConvertWindowToScreen(c->window,
			PGPoint(c->X() + b->region.x + b->region.width - 1,
				c->Y() + b->region.y)), PGTextAlignRight | PGTextAlignBottom);
	});
	buttons[3].OnPressed([](Button* b) {
		Control* c = b->parent;
		TextFile& file = ((StatusBar*)c)->active_textfield->GetTextFile();
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);

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
		PGDisplayPopupMenu(menu, ConvertWindowToScreen(c->window,
			PGPoint(c->X() + b->region.x + b->region.width - 1,
				c->Y() + b->region.y)), PGTextAlignRight | PGTextAlignBottom);
	});
}

StatusBar::~StatusBar() {
	// FIXME: unregister textfield function
}

void StatusBar::UpdateParentSize(PGSize old_size, PGSize new_size) {
	Control::UpdateParentSize(old_size, new_size);
	this->height = STATUSBAR_HEIGHT;
	this->y = parent->height - this->height;
}

void StatusBar::SelectionChanged() {
	this->Invalidate();
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
			int right_position = 0;

			PGFileEncoding encoding = file.GetFileEncoding();
			str = PGEncodingToString(encoding);
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				buttons[0].region = PGIRect(this->width - 2 * padding - text_width - right_position, 0, 2 * padding + text_width, this->height);
				buttons[0].DrawBackground(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}

			PGLanguage* language = file.GetLanguage();
			str = language ? language->GetName() : "Plain Text";
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				buttons[1].region = PGIRect(this->width - 2 * padding - text_width - right_position, 0, 2 * padding + text_width, this->height);
				buttons[1].DrawBackground(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}

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
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				buttons[2].region = PGIRect(this->width - 2 * padding - text_width - right_position, 0, 2 * padding + text_width, this->height);
				buttons[2].DrawBackground(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}

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
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				buttons[3].region = PGIRect(this->width - 2 * padding - text_width - right_position, 0, 2 * padding + text_width, this->height);
				buttons[3].DrawBackground(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}
		}
	}
}

void StatusBar::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for (int i = 0; i < 4; i++) {
		this->buttons[i].MouseDown(mouse.x, mouse.y, button, modifier);
	}
}

void StatusBar::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	for (int i = 0; i < 4; i++) {
		this->buttons[i].MouseUp(mouse.x, mouse.y, button, modifier);
	}
}

void StatusBar::MouseMove(int x, int y, PGMouseButton b) {
	if (!(b & PGLeftMouseButton)) {
		for (int i = 0; i < 4; i++) {
			buttons[i].clicking = false;
		}
		this->Invalidate();
	}
}

bool StatusBar::IsDragging() {
	for (int i = 0; i < 4; i++) {
		if (buttons[i].clicking) return true;
	}
	return false;
}


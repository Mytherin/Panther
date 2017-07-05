
#include "statusbar.h"
#include "encoding.h"
#include "style.h"
#include "controlmanager.h"
#include "searchbox.h"

StatusBar::StatusBar(PGWindowHandle window) :
	PGContainer(window), notifications(nullptr) {
	font = PGCreateFont(PGFontTypeUI);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
	GetControlManager(this)->OnSelectionChanged([](Control* c, void* data) {
		((StatusBar*)(data))->SelectionChanged();
	}, (void*) this);
	GetControlManager(this)->OnActiveTextFieldChanged([](Control* c, void* data) {
		((StatusBar*)(data))->SelectionChanged();
	}, (void*) this);
}

StatusBar::~StatusBar() {
	// FIXME: unregister textfield function
}

void StatusBar::Initialize() {
	Button* buttons[4];
	for (int i = 0; i < 4; i++) {
		auto button = make_shared_control<Button>(window, this);
		buttons[i] = button.get();
		buttons[i]->background_color = PGColor(0, 0, 0, 0);
		buttons[i]->background_stroke_color = PGColor(0, 0, 0, 0);
		buttons[i]->SetAnchor(PGAnchorTop | PGAnchorRight);
		if (i > 0) {
			buttons[i]->right_anchor = buttons[i - 1];
		}
		buttons[i]->percentage_height = 1;
		buttons[i]->fixed_size = false;
		buttons[i]->padding.left = 20;
		buttons[i]->padding.right = 20;
		this->AddControl(button);
	}

	unicode_button = buttons[0];
	language_button = buttons[1];
	lineending_button = buttons[2];
	tabwidth_button = buttons[3];
	
	unicode_button->OnPressed([](Button* button, void* data) {
		Control* c = button->parent.lock().get();
		PGPopupMenuHandle menu = PGCreatePopupMenu(button->window, c);
		PGPopupMenuHandle reopen_menu = PGCreatePopupMenu(button->window, c);
		PGPopupMenuHandle savewith_menu = PGCreatePopupMenu(button->window, c);
		{
			PGPopupMenuInsertEntry(reopen_menu, "UTF-8", [](Control* control, PGPopupInformation* info) {
				// FIXME: reopen with encoding
			}, PGPopupMenuGrayed);
			PGPopupMenuInsertEntry(savewith_menu, "UTF-8", [](Control* control, PGPopupInformation* info) {
				// FIXME: save with encoding and change active encoding
			}, PGPopupMenuGrayed);
		}
		PGPopupMenuInsertSubmenu(menu, reopen_menu, "Reopen with Encoding...");
		PGPopupMenuInsertSubmenu(menu, savewith_menu, "Save with Encoding...");

		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	}, this);

	language_button->OnPressed([](Button* button, void* data) {
		Control* c = button->parent.lock().get();
		TextField* tf = dynamic_cast<StatusBar*>(c)->GetActiveTextField();
		auto file = tf->GetTextView()->file;
		std::vector<SearchEntry> entries;
		// plain text entry
		SearchEntry entry;
		entry.basescore = 10;
		entry.multiplier = 1;
		entry.display_name = "Plain Text";
		entry.display_subtitle = "";
		entries.push_back(entry);
		// entries for all the supported languages
		auto languages = PGLanguageManager::GetLanguages();
		auto active_language = file->GetLanguage();
		for (auto it = languages.begin(); it != languages.end(); it++) {
			SearchEntry entry;
			entry.basescore = active_language == *it ? 1 : 0;
			entry.multiplier = 1;
			entry.display_name = (*it)->GetName();
			entry.display_subtitle = "";
			entries.push_back(entry);
		}
		tf->DisplaySearchBox(entries, [](SearchBox* searchbox, bool success, SearchEntry& entry, void* data) {
			if (success) {
				// switch language
				TextField* tf = (TextField*)data;
				auto file = tf->GetTextView()->file;
				PGLanguage* language = PGLanguageManager::GetLanguageFromName(entry.display_name);
				file->SetLanguage(language);
			}
		}, tf);
	}, this);

	lineending_button->OnPressed([](Button* button, void* data) {
		Control* c = button->parent.lock().get();
		auto tf = ((StatusBar*)c)->GetActiveTextField();
		auto file = tf->GetTextView()->file;
		PGPopupMenuHandle menu = PGCreatePopupMenu(button->window, c);
		PGPopupMenuInsertEntry(menu, "Windows (\\r\\n)", [](Control* control, PGPopupInformation* info) {
			auto tf = ((StatusBar*)control)->GetActiveTextField();
			auto file = tf->GetTextView()->file;
			file->ChangeLineEnding(PGLineEndingWindows);
		}, file->GetLineEnding() == PGLineEndingWindows ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		PGPopupMenuInsertEntry(menu, "Unix (\\n)", [](Control* control, PGPopupInformation* info) {
			auto tf = ((StatusBar*)control)->GetActiveTextField();
			auto file = tf->GetTextView()->file;
			file->ChangeLineEnding(PGLineEndingUnix);
		}, file->GetLineEnding() == PGLineEndingUnix ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		PGPopupMenuInsertEntry(menu, "Mac OS 9 (\\r)", [](Control* control, PGPopupInformation* info) {
			auto tf = ((StatusBar*)control)->GetActiveTextField();
			auto file = tf->GetTextView()->file;
			file->ChangeLineEnding(PGLineEndingMacOS);
		}, file->GetLineEnding() == PGLineEndingMacOS ? PGPopupMenuChecked : PGPopupMenuFlagsNone);

		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	}, this);
	tabwidth_button->OnPressed([](Button* button, void* data) {
		Control* c = button->parent.lock().get();
		TextField* tf = dynamic_cast<StatusBar*>(c)->GetActiveTextField();
		auto file = tf->GetTextView()->file;
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);

		PGPopupMenuInsertEntry(menu, "Convert Indentation To Spaces", [](Control* control, PGPopupInformation* info) {
			TextField* tf = dynamic_cast<StatusBar*>(control)->GetActiveTextField();
			auto file = tf->GetTextView()->file;
			file->ConvertToIndentation(PGIndentionSpaces);
		});
		PGPopupMenuInsertEntry(menu, "Convert Indentation To Tabs", [](Control* control, PGPopupInformation* info) {
			TextField* tf = dynamic_cast<StatusBar*>(control)->GetActiveTextField();
			auto file = tf->GetTextView()->file;
			file->ConvertToIndentation(PGIndentionTabs);
		});
		PGPopupMenuInsertEntry(menu, "Remove Trailing Whitespace", [](Control* control, PGPopupInformation* info) {
			TextField* tf = dynamic_cast<StatusBar*>(control)->GetActiveTextField();
			auto file = tf->GetTextView()->file;
			file->RemoveTrailingWhitespace();
		}, file->GetLineIndentation() == PGIndentionSpaces ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		PGPopupMenuInsertSeparator(menu);
		for (int i = 1; i <= 8; i++) {
			PGPopupInformation info(menu);
			info.text = "Tab Width: " + std::to_string(i);
			info.hotkey = "";
			info.data = std::string(1, (char)i);
			PGPopupMenuInsertEntry(menu, info, [](Control* control, PGPopupInformation* info) {
				TextField* tf = dynamic_cast<StatusBar*>(control)->GetActiveTextField();
				auto file = tf->GetTextView()->file;
				file->SetTabWidth(info->data[0]);
			});
		}
		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	}, this);
	PGContainer::Initialize();
}

void StatusBar::SetText(std::string text) {
	this->status = text;
	this->Invalidate();
}

void StatusBar::SelectionChanged() {
	this->Invalidate();
}

void StatusBar::Draw(PGRendererHandle renderer) {
	using namespace std;
	PGScalar x = X(), y = Y();
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorStatusBarBackground), PGDrawStyleFill);
	auto active_textfield = GetActiveTextField();
	if (active_textfield) {
		auto view = active_textfield->GetTextView();
		if (view->file->IsLoaded()) {
			PGColor line_color = PGStyleManager::GetColor(PGColorStatusBarText);
			auto cursors = view->GetCursors();
			std::string str = "";
			if (cursors.size() == 1) {
				auto selected_pos = cursors[0].SelectedCharacterPosition();
				auto begin_pos = cursors[0].BeginCharacterPosition();
				auto end_pos = cursors[0].EndCharacterPosition();
				str = string_sprintf("Line %lld, Column %lld", selected_pos.line + 1, selected_pos.character + 1);

				if (!cursors[0].SelectionIsEmpty()) {
					if (begin_pos.line == end_pos.line) {
						str += string_sprintf(" (%lld characters selected)", end_pos.character - begin_pos.character);
					} else {
						str += string_sprintf(" (%lld lines selected)", end_pos.line - begin_pos.line + 1);
					}
				}
			} else {
				str = to_string(cursors.size()) + string(" selected regions");
			}
			if (status.size() > 0) {
				str += " - " + status;
			}
			RenderText(renderer, font, str.c_str(), str.size(), x + 10, y);

			PGFileEncoding encoding = view->file->GetFileEncoding();
			str = PGEncodingToString(encoding);
			unicode_button->SetText(str, font);
			PGLanguage* language = view->file->GetLanguage();
			str = language ? language->GetName() : "Plain Text";
			language_button->SetText(str, font);
			PGLineEnding ending = view->file->GetLineEnding();
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
			lineending_button->SetText(str, font);
			PGLineIndentation indentation = view->file->GetLineIndentation();
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
			str += to_string(view->file->GetTabWidth());
			tabwidth_button->SetText(str, font);

			this->TriggerResize();
			this->FlushRemoves();

			PGContainer::Draw(renderer);
		}
	}
	Control::Draw(renderer);
}

TextField* StatusBar::GetActiveTextField() {
	return GetControlManager(this)->active_textfield;
}

std::shared_ptr<PGStatusNotification> StatusBar::AddNotification(PGStatusType type, std::string text, std::string tooltip, bool progress_bar) {
	auto n = std::make_shared<PGStatusNotification>(window, font, type, text, tooltip, progress_bar);
	PGStatusNotification* notification = n.get();
	notification->SetAnchor(PGAnchorTop | PGAnchorRight);
	notification->margin.top = 1;
	notification->margin.bottom = 1;
	notification->padding.left = 4;
	notification->padding.right = 4;
	notification->percentage_height = 1;
	notification->right_anchor = notifications ? (Control*)notifications : (Control*)tabwidth_button;
	notifications = notification;
	this->AddControl(n);
	this->TriggerResize();
	return n;
}

void StatusBar::RemoveNotification(std::shared_ptr<PGStatusNotification> notification) {
	PGStatusNotification* n = notifications;
	PGStatusNotification* prev = nullptr;
	while (n && n->GetControlType() == PGControlTypeStatusNotification) {
		if (n == notification.get()) {
			if (prev) {
				prev->right_anchor = n->right_anchor;
			}
			if (notification.get() == notifications) {
				notifications = n->right_anchor->GetControlType() == PGControlTypeStatusNotification ?
					(PGStatusNotification*) n->right_anchor : nullptr;
			}
			this->RemoveControl(notification.get());
			return;
		}
		prev = n;
		n = (PGStatusNotification*)n->right_anchor;
	}
	assert(0);
}

void StatusBar::AddTimedNotification(PGStatusType type, std::string text, std::string tooltip, int id, double time) {
	for(size_t i = 0; i < timed_notifications.size(); i++) {
		auto ptr = timed_notifications[i].lock();
		if (ptr) {
			if (ptr->id == id) {
				ptr->SetText(text);
				ptr->remaining_time = time;
				return;
			}
		}
	}
	auto n = this->AddNotification(type, text, tooltip, false);

	n->id = id;
	n->remaining_time = time;
	this->timed_notifications.push_back(std::weak_ptr<PGStatusNotification>(n));
}

void StatusBar::Update(/* double t */) {
	for(size_t i = 0; i < timed_notifications.size(); i++) {
		auto ptr = timed_notifications[i].lock();
		if (ptr) {
			ptr->remaining_time -= 1.0 / 60.0;
			if (ptr->remaining_time <= 0) {
				RemoveNotification(ptr);
			} else {
				continue;
			}
		}
		timed_notifications.erase(timed_notifications.begin() + i);
		i--;
	}
}


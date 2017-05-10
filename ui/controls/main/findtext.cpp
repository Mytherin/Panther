
#include "controlmanager.h"
#include "findtext.h"
#include "style.h"
#include "workspace.h"

#include "findresults.h"
#include "globalsettings.h"

#include "statusbar.h"
#include "statusnotification.h"

#define HPADDING 10
#define HPADDING_SMALL 5
#define VPADDING 4

PG_CONTROL_INITIALIZE_KEYBINDINGS(PGFindText);

static void UpdateHighlight(Control* c, PGFindText* f) {
	if (f->HighlightMatches()) {
		f->FindAll(false);
	};
}

PGFindText::PGFindText(PGWindowHandle window, PGFindTextType type) :
	PGContainer(window), history_entry(0), field_lines(1), notification(nullptr) {
	font = PGCreateFont(PGFontTypeUI);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));

	field = new SimpleTextField(window, true);
	field->percentage_width = 1;
	field->fixed_height = field->height;
	field->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop);
	field->margin.top = VPADDING;
	field->OnPrevEntry([](Control* c, void* data) {
		PGFindText* f = (PGFindText*)data;
		std::vector<std::string>& find_history = f->GetFindTextManager().find_history;
		if (find_history.size() > f->history_entry + 1) {
			f->history_entry++;
			((SimpleTextField*)c)->SetText(find_history[f->history_entry]);
		}
	}, this);
	field->OnNextEntry([](Control* c, void* data) {
		PGFindText* f = (PGFindText*)data;
		std::vector<std::string>& find_history = f->GetFindTextManager().find_history;
		if (f->history_entry > 0) {
			f->history_entry--;
			((SimpleTextField*)c)->SetText(find_history[f->history_entry]);
		} else {
			if (find_history[f->history_entry].size() > 0) {
				f->history_entry = 0;
				find_history.insert(find_history.begin(), std::string(""));
				((SimpleTextField*)c)->SetText(find_history[f->history_entry]);
			}
		}
	}, this);
	field->OnTextChanged([](Control* c, void* data) {
		// if HighlightMatches is turned on, we search as soon as text is entered
		PGFindText* f = (PGFindText*)data;
		f->UpdateFieldHeight();
		if (f->HighlightMatches()) {
			f->FindAll(true);
		}
	}, (void*) this);
	this->AddControl(field);

	this->percentage_width = 1;

	PGScalar button_width = MeasureTextWidth(font, "Find Prev") + 2 * HPADDING;
	PGScalar button_height = field->height;
	hoffset = button_width * 3 + button_height;

	Button* buttons[2];
	for (int i = 0; i < 2; i++) {
		buttons[i] = new Button(window, this);
		buttons[i]->fixed_width = button_width;
		buttons[i]->fixed_height = button_height;
		buttons[i]->right_anchor = i > 0 ? buttons[i - 1] : nullptr;
		buttons[i]->SetAnchor(PGAnchorRight | PGAnchorTop);
		buttons[i]->margin.top = VPADDING;
		buttons[i]->margin.left = HPADDING;
		this->AddControl(buttons[i]);
	}
	find_expand = buttons[0];
	find_button = buttons[1];

	find_expand->margin.left = HPADDING;
	find_expand->margin.right = HPADDING_SMALL;
	find_expand->fixed_width = button_height;

	FindTextManager& manager = GetFindTextManager();
	this->regex = manager.regex;
	this->matchcase = manager.matchcase;
	this->wholeword = manager.wholeword;
	this->wrap = manager.wrap;
	this->highlight = manager.highlight;
	this->ignore_binary_files = manager.ignore_binary_files;
	this->respect_gitignore = manager.respect_gitignore;

	toggle_regex = ToggleButton::CreateFromCommand(this, "toggle_regex", "Toggle regular expressions",
		PGFindText::keybindings_noargs, font, ".*", this->regex);
	toggle_matchcase = ToggleButton::CreateFromCommand(this, "toggle_matchcase", "Toggle case sensitive search",
		PGFindText::keybindings_noargs, font, "Aa", this->matchcase);
	toggle_wholeword = ToggleButton::CreateFromCommand(this, "toggle_wholeword", "Toggle whole word search",
		PGFindText::keybindings_noargs, PGFindText::keybindings_images, this->wholeword);
	toggle_wrap = ToggleButton::CreateFromCommand(this, "toggle_wrap", "Toggle search wrap",
		PGFindText::keybindings_noargs, PGFindText::keybindings_images, this->wrap);
	toggle_highlight = ToggleButton::CreateFromCommand(this, "toggle_highlight", "Automatically search while typing",
		PGFindText::keybindings_noargs, PGFindText::keybindings_images, this->highlight);

	ToggleButton* toggles[] = { toggle_regex, toggle_matchcase, toggle_wholeword, toggle_wrap, toggle_highlight };
	for (int i = 0; i < 5; i++) {
		toggles[i]->SetAnchor(PGAnchorLeft | PGAnchorTop);
		toggles[i]->left_anchor = i > 0 ? toggles[i - 1] : nullptr;
		toggles[i]->margin.left = 1;
		toggles[i]->margin.top = VPADDING;
		this->AddControl(toggles[i]);
	}

	toggle_regex->margin.left = HPADDING_SMALL;
	toggle_wrap->margin.left = HPADDING_SMALL;

	find_label = new Label(window, this);
	find_label->SetAnchor(PGAnchorTop | PGAnchorLeft);
	find_label->SetText("Find:", font);
	find_label->fixed_height = field->height;
	find_label->fixed_width = MeasureTextWidth(font, "Replace:");
	find_label->margin.left = HPADDING_SMALL;
	find_label->margin.right = HPADDING_SMALL;
	find_label->margin.top = VPADDING;
	find_label->left_anchor = toggle_highlight;
	this->AddControl(find_label);

	field->left_anchor = find_label;
	field->right_anchor = find_button;

	find_button->SetText(std::string("Find"), font);
	find_expand->SetText(std::string("v"), font);

	ControlManager* cmanager = GetControlManager(this);
	cmanager->OnTextChanged((PGControlDataCallback)UpdateHighlight, this);
	cmanager->OnActiveTextFieldChanged((PGControlDataCallback)UpdateHighlight, this);

	this->replace_field = nullptr;
	this->replace_button = nullptr;
	this->replace_all_button = nullptr;
	this->replace_in_selection_button = nullptr;
	this->files_to_include_field = nullptr;
	this->source_files_only = nullptr;
	this->toggle_respect_gitignore = nullptr;
	this->replace_expand = nullptr;
	this->find_prev = nullptr;
	this->find_all = nullptr;
	this->replace_label = nullptr;
	this->filter_label = nullptr;

	this->SetType(type);
}

PGFindText::~PGFindText() {
	ControlManager* manager = GetControlManager(this);
	manager->active_findtext = nullptr;
	manager->UnregisterOnTextChanged((PGControlDataCallback)UpdateHighlight, this);
	manager->UnregisterOnActiveTextFieldChanged((PGControlDataCallback)UpdateHighlight, this);
	if (notification) {
		manager->statusbar->RemoveNotification(notification);
		notification = nullptr;
	}
}

void PGFindText::SetType(PGFindTextType type) {
	this->type = type;

	// clear current controls
	std::vector<Control*> controls{ replace_field, replace_button, replace_all_button, replace_in_selection_button,
		files_to_include_field, source_files_only, toggle_respect_gitignore, replace_expand, find_prev, find_all,
		replace_label, filter_label };

	for (auto it = controls.begin(); it != controls.end(); it++) {
		if ((*it)) {
			this->RemoveControl(*it);
		}
	}
	this->replace_field = nullptr;
	this->replace_button = nullptr;
	this->replace_all_button = nullptr;
	this->replace_in_selection_button = nullptr;
	this->files_to_include_field = nullptr;
	this->source_files_only = nullptr;
	this->toggle_respect_gitignore = nullptr;
	this->replace_expand = nullptr;
	this->find_prev = nullptr;
	this->find_all = nullptr;
	this->replace_label = nullptr;
	this->filter_label = nullptr;

	if (type != PGFindReplaceManyFiles) {
		Button* buttons[2];
		for (int i = 0; i < 2; i++) {
			buttons[i] = new Button(window, this);
			buttons[i]->fixed_width = find_button->fixed_width;
			buttons[i]->fixed_height = find_button->fixed_height;
			buttons[i]->margin.top = find_button->margin.top;
			buttons[i]->margin.left = HPADDING;
			buttons[i]->SetAnchor(PGAnchorTop | PGAnchorRight);
			buttons[i]->right_anchor = i > 0 ? buttons[i - 1] : nullptr;
			this->AddControl(buttons[i]);
		}
		find_all = buttons[0];
		find_prev = buttons[1];

		find_button->right_anchor = find_prev;
		find_all->right_anchor = find_expand;

		find_prev->SetText(std::string("Find Prev"), font);
		find_all->SetText(std::string("Find All"), font);

		find_prev->OnPressed([](Button* b, void* data) {
			((PGFindText*)data)->Find(PGDirectionLeft);
		}, this);

		find_all->OnPressed([](Button* b, void* data) {
			PGFindText* ft = ((PGFindText*)data);
			ft->SelectAllMatches();
			ft->Close();
		}, this);

		find_button->OnPressed([](Button* b, void* data) {
			((PGFindText*)data)->Find(PGDirectionRight);
		}, this);
	} else {
		find_button->OnPressed([](Button* b, void* data) {
			((PGFindText*)data)->FindInFiles();
		}, this);
	}

	switch (type) {
		case PGFindSingleFile:
		{
			find_expand->SetText(std::string("v"), font);
			find_expand->OnPressed([](Button* b, void* data) {
				PGFindText* tf = ((PGFindText*)data);
				tf->SetType(PGFindReplaceSingleFile);
			}, this);
			break;
		}
		case PGFindReplaceManyFiles:
		{
			filter_label = new Label(window, this);
			filter_label->SetText("Filter:", font);
			filter_label->SetAnchor(PGAnchorTop | PGAnchorLeft);
			filter_label->left_anchor = toggle_highlight;
			filter_label->fixed_width = find_label->fixed_width;
			filter_label->fixed_height = find_label->fixed_height;
			filter_label->margin = find_label->margin;
			this->AddControl(filter_label);

			files_to_include_field = new SimpleTextField(window, true);
			files_to_include_field->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop);
			files_to_include_field->left_anchor = filter_label;
			files_to_include_field->right_anchor = find_button;
			files_to_include_field->percentage_width = 1;
			files_to_include_field->fixed_height = field->fixed_height;
			files_to_include_field->margin.top = VPADDING;
			this->AddControl(files_to_include_field);

			files_to_include_field->OnTextChanged([](Control* c, void* data) {
				PGFindText* f = (PGFindText*)data;
				f->UpdateFieldHeight();
			}, (void*) this);


			source_files_only = ToggleButton::CreateFromCommand(this, "toggle_ignorebinary", "Ignore binary files",
				PGFindText::keybindings_noargs, font, "B", this->ignore_binary_files);
			toggle_respect_gitignore = ToggleButton::CreateFromCommand(this, "toggle_gitignore", "Respect gitignore files",
				PGFindText::keybindings_noargs, font, ".g", this->respect_gitignore);

			ToggleButton* toggles[] = { source_files_only, toggle_respect_gitignore };
			for (int i = 0; i < 2; i++) {
				toggles[i]->SetAnchor(PGAnchorLeft | PGAnchorTop);
				toggles[i]->left_anchor = i > 0 ? toggles[i - 1] : nullptr;
				toggles[i]->margin.top = VPADDING;
				toggles[i]->margin.left = 1;
				this->AddControl(toggles[i]);
			}
			source_files_only->margin.left = HPADDING_SMALL;
		}
		case PGFindReplaceSingleFile:
		{
			replace_label = new Label(window, this);
			replace_label->SetText("Replace:", font);
			replace_label->SetAnchor(PGAnchorTop | PGAnchorLeft);
			replace_label->top_anchor = field;
			replace_label->left_anchor = toggle_highlight;
			replace_label->fixed_width = find_label->fixed_width;
			replace_label->fixed_height = find_label->fixed_height;
			replace_label->margin = find_label->margin;
			this->AddControl(replace_label);

			replace_field = new SimpleTextField(window, true);
			replace_field->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop);
			replace_field->top_anchor = field;
			replace_field->left_anchor = replace_label;
			replace_field->right_anchor = find_button;
			replace_field->percentage_width = 1;
			replace_field->fixed_height = field->fixed_height;
			replace_field->margin.top = VPADDING;
			this->AddControl(replace_field);

			replace_field->OnTextChanged([](Control* c, void* data) {
				PGFindText* f = (PGFindText*)data;
				f->UpdateFieldHeight();
			}, (void*) this);

			if (type == PGFindReplaceManyFiles) {
				toggle_respect_gitignore->top_anchor = replace_field;
				source_files_only->top_anchor = replace_field;
				files_to_include_field->top_anchor = replace_field;
				filter_label->top_anchor = replace_field;
			}

			Button* buttons[3];
			int button_count = type == PGFindReplaceSingleFile ? 3 : 1;
			for (int i = 0; i < button_count; i++) {
				buttons[i] = new Button(window, this);
				buttons[i]->fixed_width = find_button->fixed_width;
				buttons[i]->fixed_height = find_button->fixed_height;
				buttons[i]->margin.top = find_button->margin.top;
				buttons[i]->margin.left = HPADDING;
				buttons[i]->top_anchor = field;
				buttons[i]->SetAnchor(PGAnchorTop | PGAnchorRight);
				buttons[i]->right_anchor = i > 0 ? buttons[i - 1] : nullptr;
				this->AddControl(buttons[i]);
			}

			replace_button = buttons[0];
			replace_button->right_anchor = find_expand;
			replace_button->SetText(std::string("Replace"), font);

			if (type == PGFindReplaceSingleFile) {
				replace_all_button = buttons[1];
				replace_in_selection_button = buttons[2];

				replace_in_selection_button->right_anchor = find_expand;
				replace_all_button->right_anchor = replace_in_selection_button;
				replace_button->right_anchor = replace_all_button;

				replace_all_button->SetText(std::string("Replace All"), font);
				replace_in_selection_button->SetText(std::string("In Selection"), font);

				replace_all_button->OnPressed([](Button* b, void* data) {
					PGFindText* ft = ((PGFindText*)data);
					ft->ReplaceAll();
					ft->Close();
				}, this);
				replace_in_selection_button->OnPressed([](Button* b, void* data) {
					PGFindText* ft = ((PGFindText*)data);
					ft->ReplaceAll(true);
				}, this);


				replace_expand = new Button(window, this);
				replace_expand->SetAnchor(PGAnchorBottom | PGAnchorRight);
				replace_expand->margin.right = HPADDING_SMALL;
				replace_expand->margin.bottom = VPADDING;
				replace_expand->fixed_width = replace_expand->fixed_height = find_expand->fixed_width;
				this->AddControl(replace_expand);
				replace_expand->SetText(std::string("v"), font);
				replace_expand->OnPressed([](Button* b, void* data) {
					PGFindText* tf = ((PGFindText*)data);
					tf->SetType(PGFindReplaceManyFiles);
				}, this);

				replace_button->OnPressed([](Button* b, void* data) {
					((PGFindText*)data)->Replace();
				}, this);
			} else {
				replace_button->OnPressed([](Button* b, void* data) {
					PGFindText* ft = ((PGFindText*)data);
					assert(0);
				}, this);

			}

			find_expand->SetText(std::string("^"), font);
			if (type == PGFindReplaceSingleFile) {
				find_expand->OnPressed([](Button* b, void* data) {
					PGFindText* tf = ((PGFindText*)data);
					tf->SetType(PGFindSingleFile);
				}, this);
			} else {
				find_expand->OnPressed([](Button* b, void* data) {
					PGFindText* tf = ((PGFindText*)data);
					tf->SetType(PGFindReplaceSingleFile);
				}, this);
			}
			break;
		}
		default:
			break;
	}
	this->UpdateFieldHeight(true);
	GetControlManager(this)->TriggerResize();
	this->TriggerResize();
}

void PGFindText::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();

	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);

	this->everything_dirty = true;
	PGContainer::Draw(renderer);
}

void PGFindText::UpdateFieldHeight(bool force_update) {
	lng current_lines = std::max(1LL, field->GetTextFile().GetLineCount());
	lng textfield_count = 1;
	if (replace_field) {
		current_lines = std::max(current_lines, replace_field->GetTextFile().GetLineCount());
		textfield_count++;
	}
	if (files_to_include_field) {
		current_lines = std::max(current_lines, files_to_include_field->GetTextFile().GetLineCount());
		textfield_count++;
	}
	if (current_lines != field_lines || force_update) {
		PGScalar max_height = (GetControlManager(this)->height / 2.0f) / textfield_count;
		PGScalar height = std::min(current_lines * std::ceil(GetTextHeight(field->GetTextfieldFont())) + 6, max_height);
		field->fixed_height = height;
		if (replace_field) replace_field->fixed_height = height;
		if (files_to_include_field) files_to_include_field->fixed_height = height;

		field_lines = current_lines;
		GetControlManager(this)->TriggerResize();
	}
}

bool PGFindText::Find(PGDirection direction, bool include_selection) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	if (!tf.IsLoaded()) return false;
	char* error_message = nullptr;

	std::string pattern = field->GetText();
	PGRegexHandle regex_handle = PGCompileRegex(pattern, this->regex, GetRegexFlags());
	if (!regex_handle) {
		// FIXME: throw an error, regex was not compiled properly
		error_message = panther::strdup("Error");
		return false;
	}

	bool found_result = tf.FindMatch(regex_handle, direction,
		toggle_wrap->IsToggled(),
		include_selection);

	std::vector<std::string>& find_history = GetFindTextManager().find_history;
	if (find_history.size() == 0) {
		find_history.push_back(pattern);
	} else if (find_history[0] != pattern) {
		auto current_entry = std::find(find_history.begin(), find_history.end(), pattern);
		if (current_entry != find_history.end()) {
			find_history.erase(current_entry);
		}

		std::string first_entry = find_history[0];
		if (first_entry.size() == 0) {
			find_history[0] = pattern;
		} else {
			find_history.insert(find_history.begin(), pattern);
			if (find_history.size() > MAXIMUM_FIND_HISTORY) {
				find_history.erase(find_history.begin() + find_history.size() - 1);
			}
		}
		history_entry = 0;
	}

	if (!error_message) {
		// successful search
		this->field->SetValidInput(true);
		this->Invalidate();
	} else {
		// error compiling regex
		this->field->SetValidInput(false);
		this->Invalidate();
	}
	return found_result;
}

void PGFindText::FindInFiles() {
	// FIXME: white/black list
	// first compile the regex
	std::string regex_pattern = field->GetText();
	PGRegexHandle regex = PGCompileRegex(regex_pattern, this->regex, GetRegexFlags());
	if (!regex) {
		// failed to compile regex
		return;
	}
	// then compile the globset
	assert(files_to_include_field);
	std::string file_filter = files_to_include_field->GetText();
	auto split = panther::split(file_filter, ',');
	for (lng i = 0; i < split.size(); i++) {
		split[i] = panther::trim(split[i]);
		if (split[i].size() == 0) {
			split.erase(split.begin() + i);
			i--;
		}
	}
	PGGlobSet whitelist = nullptr;
	if (split.size() > 0) {
		PGGlobBuilder glob_builder = PGCreateGlobBuilder();
		for (auto it = split.begin(); it != split.end(); it++) {
			if (PGGlobBuilderAddGlob(glob_builder, it->c_str()) != 0) {
				// failed to compile glob
				continue;
			}
		}
		whitelist = PGCompileGlobBuilder(glob_builder);
		PGDestroyGlobBuilder(glob_builder);
		if (!whitelist) {
			// failed to compile glob builder
			assert(0);
			return;
		}
	}

	// create a textfile for us to store the search results
	ControlManager* manager = GetControlManager(this);
	TextField* textfield = manager->active_textfield;
	auto textfile = std::shared_ptr<TextFile>(new TextFile(nullptr));

	textfile->SetReadOnly(true);

	textfile->SetName("Find Results");
	textfile->SetLanguage(PGLanguageManager::GetLanguage("findresults"));
	textfield->GetTabControl()->OpenFile(textfile);
	/*
	// first find the list of files we want to search
	std::vector<PGFile> files;
	auto& directories = manager->active_projectexplorer->GetDirectories();
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->ListFiles(files, whitelist);
	}

	if (whitelist) {
		PGDestroyGlobSet(whitelist);
	}*/
	// now schedule the actual search
	textfile->FindAllMatchesAsync(whitelist, manager->active_projectexplorer, regex, 2, ignore_binary_files);
}

bool PGFindText::SelectAllMatches(bool in_selection) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	if (!tf.IsLoaded()) return false;

	if (HighlightMatches()) {
		// toggle-highlight is turned on
		// so we should already be searching
		// wait for the search to finish
		while (!tf.FinishedSearch());
	} else {
		// otherwise, we have to actually perform the search
		// FIXME: if FindAll becomes non-blocking, we have to call the
		// blocking version here
		this->FindAll(false);
	}
	assert(tf.FinishedSearch());
	return tf.SelectMatches(in_selection);
}
void PGFindText::FindAll(bool select_first_match) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();

	if (!tf.IsLoaded()) return;

	PGStatusType type;
	std::string text;

	tf.SetSelectedMatch(0);
	auto begin_pos = tf.GetActiveCursor().BeginPosition();
	auto end_pos = tf.GetActiveCursor().EndPosition();
	std::string pattern = field->GetText();

	if (pattern.size() == 0) {
		tf.ClearMatches();
		if (notification) {
			GetControlManager(this)->statusbar->RemoveNotification(notification);
			notification = nullptr;
		}
		return;
	}

	PGRegexHandle regex_handle = PGCompileRegex(pattern, this->regex, GetRegexFlags());
	if (!regex_handle || PGRegexHasErrors(regex_handle)) {
		type = PGStatusError;
		text = "Failed to compile regex: " + PGGetRegexError(regex_handle);
		this->field->SetValidInput(false);
		goto set_notification;
	}
	tf.FindAllMatches(regex_handle, select_first_match,
		begin_pos.line, begin_pos.position,
		end_pos.line, end_pos.position,
		toggle_wrap->IsToggled());
	
	{
		size_t match_count = tf.GetFindMatches().size();
		type = match_count == 0 ? PGStatusWarning : PGStatusInProgress;
		text = "Found " + std::to_string(match_count) + " matches";
		this->field->SetValidInput(true);
	}

set_notification:
	if (!notification) {
		notification = GetControlManager(this)->statusbar->AddNotification(type, text, "", false);
	} else {
		notification->SetType(type);
		notification->SetText(text);
	}
}

void PGFindText::Replace() {
	if (!replace_field) return;
	std::string replacement = replace_field->GetText();
	if (this->Find(PGDirectionRight, true)) {
		ControlManager* manager = GetControlManager(this);
		TextFile& tf = manager->active_textfield->GetTextFile();
		PGRegexHandle regex = PGCompileRegex(field->GetText(), this->regex, GetRegexFlags());
		tf.RegexReplace(regex, replacement);
		if (HighlightMatches()) {
			this->FindAll(PGDirectionRight);
		}
	}
}

void PGFindText::ReplaceAll(bool in_selection) {
	if (!replace_field) return;
	std::string replacement = replace_field->GetText();
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	if (!tf.IsLoaded()) return;

	if (!this->SelectAllMatches(in_selection)) {
		// no matches were found
		return;
	}
	PGRegexHandle regex = PGCompileRegex(field->GetText(), this->regex, GetRegexFlags());
	tf.RegexReplace(regex, replacement);
	if (HighlightMatches()) {
		this->FindAll(false);
	}
}

bool PGFindText::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(PGFindText::keybindings, button, modifier)) {
		return true;
	}
	return PGContainer::KeyboardButton(button, modifier);
}

void PGFindText::Close() {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	tf.ClearMatches();
	dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);
}

void PGFindText::ShiftTextfieldFocus(PGDirection direction) {
	// (shift+)tab switches between the two text fields
	SimpleTextField* new_focus = nullptr;
	SimpleTextField* fields[3] = { field, replace_field, files_to_include_field };
	lng selected_field = focused_control == files_to_include_field ? 2 : (focused_control == replace_field ? 1 : 0);

	selected_field += direction == PGDirectionRight ? 1 : -1;
	if (selected_field < 0) {
		selected_field = 2;
	} else if (selected_field > 2) {
		selected_field = 0;
	}
	assert(fields[0]);
	while (fields[selected_field] == nullptr) {
		selected_field += direction == PGDirectionRight ? 1 : -1;
		if (selected_field > 2) {
			selected_field = 0;
		}
	}
	new_focus = fields[selected_field];
	if (new_focus) {
		focused_control = new_focus;
		new_focus->RefreshCursors();
		new_focus->Invalidate();
	}
}

void PGFindText::InitializeKeybindings() {
	std::map<std::string, PGBitmapHandle>& images = PGFindText::keybindings_images;
	std::map<std::string, PGKeyFunction>& noargs = PGFindText::keybindings_noargs;
	noargs["find_next"] = [](Control* c) {
		((PGFindText*)c)->Find(PGDirectionRight);
	};
	noargs["find_prev"] = [](Control* c) {
		((PGFindText*)c)->Find(PGDirectionLeft);
	};
	noargs["close"] = [](Control* c) {
		((PGFindText*)c)->Close();
	};
	noargs["shift_focus_forward"] = [](Control* c) {
		((PGFindText*)c)->ShiftTextfieldFocus(PGDirectionRight);
	};
	noargs["shift_focus_backward"] = [](Control* c) {
		((PGFindText*)c)->ShiftTextfieldFocus(PGDirectionLeft);
	};
	images["toggle_regex"] = PGStyleManager::GetImage("data/icons/regex.png");
	noargs["toggle_regex"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleRegex);
	};
	images["toggle_matchcase"] = PGStyleManager::GetImage("data/icons/matchcase.png");
	noargs["toggle_matchcase"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleMatchcase);
	};
	images["toggle_wholeword"] = PGStyleManager::GetImage("data/icons/wholeword.png");
	noargs["toggle_wholeword"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleWholeword);
	};
	images["toggle_wrap"] = PGStyleManager::GetImage("data/icons/wrap.png");
	noargs["toggle_wrap"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleWrap);
	};
	images["toggle_highlight"] = PGStyleManager::GetImage("data/icons/instantsearch.png");
	noargs["toggle_highlight"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleHighlight);
	};
	noargs["toggle_ignorebinary"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleIgnoreBinary);
	};
	noargs["toggle_gitignore"] = [](Control* c) {
		((PGFindText*)c)->Toggle(PGFindTextToggleGitIgnore);
	};
}

void PGFindText::ResolveSize(PGSize new_size) {
	switch (this->type) {
		case PGFindSingleFile:
			this->fixed_height = field->fixed_height + VPADDING * 2;
			break;
		case PGFindReplaceManyFiles:
			this->fixed_height = 3 * (field->fixed_height + VPADDING * 2);
			break;
		case PGFindReplaceSingleFile:
			this->fixed_height = 2 * (field->fixed_height + VPADDING * 2);
			break;
	}
	PGContainer::ResolveSize(new_size);
}

FindTextManager& PGFindText::GetFindTextManager() {
	return PGGetWorkspace(this->window)->GetFindTextManager();
}

void PGFindText::Toggle(PGFindTextToggles type) {
	FindTextManager& manager = GetFindTextManager();
	switch (type) {
		case PGFindTextToggleRegex:
			regex = !regex;
			manager.regex = regex;
			if (toggle_regex) {
				toggle_regex->SetToggled(regex);
			}
			break;
		case PGFindTextToggleMatchcase:
			matchcase = !matchcase;
			manager.matchcase = matchcase;
			if (toggle_matchcase) {
				toggle_matchcase->SetToggled(matchcase);
			}
			break;
		case PGFindTextToggleWholeword:
			wholeword = !wholeword;
			manager.wholeword = wholeword;
			if (toggle_wholeword) {
				toggle_wholeword->SetToggled(wholeword);
			}
			break;
		case PGFindTextToggleWrap:
			wrap = !wrap;
			manager.wrap = wrap;
			if (toggle_wrap) {
				toggle_wrap->SetToggled(wrap);
			}
			break;
		case PGFindTextToggleHighlight:
			highlight = !highlight;
			manager.highlight = highlight;
			if (toggle_highlight) {
				toggle_highlight->SetToggled(highlight);
			}
			if (!highlight) {
				ControlManager* manager = GetControlManager(this);
				TextFile& tf = manager->active_textfield->GetTextFile();
				tf.ClearMatches();
				tf.SetSelectedMatch(-1);
			}
			break;
		case PGFindTextToggleIgnoreBinary:
			ignore_binary_files = !ignore_binary_files;
			manager.ignore_binary_files = ignore_binary_files;
			if (ignore_binary_files) {
				source_files_only->SetToggled(ignore_binary_files);
			}
			break;
		case PGFindTextToggleGitIgnore:
			respect_gitignore = !respect_gitignore;
			manager.respect_gitignore = respect_gitignore;
			if (toggle_respect_gitignore) {
				toggle_respect_gitignore->SetToggled(respect_gitignore);
			}
			break;
	}
	switch (type) {
		case PGFindTextToggleRegex:
		case PGFindTextToggleMatchcase:
		case PGFindTextToggleWholeword:
		case PGFindTextToggleWrap:
		case PGFindTextToggleHighlight:
			if (HighlightMatches()) {
				FindAll(true);
			}
		default:
			break;
	}
}

PGRegexFlags PGFindText::GetRegexFlags() {
	PGRegexFlags flags = PGRegexFlagsNone;
	if (!this->matchcase) flags |= PGRegexCaseInsensitive;
	if (this->wholeword) flags |= PGRegexWholeWordSearch;
	return flags;
}

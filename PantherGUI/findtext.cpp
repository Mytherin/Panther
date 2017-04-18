
#include "controlmanager.h"
#include "findtext.h"
#include "style.h"
#include "workspace.h"

#include "findresults.h"

#define HPADDING 10
#define HPADDING_SMALL 5
#define VPADDING 4

#define MAXIMUM_FIND_HISTORY 5

PG_CONTROL_INITIALIZE_KEYBINDINGS(PGFindText);

static void UpdateHighlight(Control* c, PGFindText* f) {
	if (f->HighlightMatches()) {
		f->FindAll(false);
	};
}

PGFindText::PGFindText(PGWindowHandle window, PGFindTextType type) :
	PGContainer(window), history_entry(0) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));

	field = new SimpleTextField(window);
	field->width = this->width - 400;
	field->x = 150;
	field->y = VPADDING;
	field->OnPrevEntry([](Control* c, void* data) {
		PGFindText* f = (PGFindText*)data;
		nlohmann::json& find_history = f->GetFindHistory();
		if (find_history.size() > f->history_entry + 1) {
			f->history_entry++;
			((SimpleTextField*)c)->SetText(find_history[f->history_entry]);
		}
	}, this);
	field->OnNextEntry([](Control* c, void* data) {
		PGFindText* f = (PGFindText*)data;
		nlohmann::json& find_history = f->GetFindHistory();
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
		if (f->HighlightMatches()) {
			f->FindAll(true);
		}
	}, (void*) this);
	this->AddControl(field);

	this->percentage_width = 1;
	this->fixed_height = field->height + VPADDING * 2;
	this->height = fixed_height;

	PGScalar button_width = MeasureTextWidth(font, "Find Prev") + 2 * HPADDING;
	PGScalar button_height = field->height;
	hoffset = button_width * 3 + button_height;

	Button* buttons[2];
	for (int i = 0; i < 2; i++) {
		buttons[i] = new Button(window, this);
		buttons[i]->SetSize(PGSize(button_width, button_height));
		buttons[i]->y = VPADDING;
		this->AddControl(buttons[i]);
	}
	find_button = buttons[0];
	find_expand = buttons[1];
	find_expand->SetSize(PGSize(button_height, button_height));

	auto workspace = PGGetWorkspace(window);
	assert(workspace);
	if (workspace->settings.count("find_text") == 0 || !workspace->settings["find_text"].is_object()) {
		workspace->settings["find_text"] = nlohmann::json::object();
	}
	nlohmann::json& find_text = workspace->settings["find_text"];

	assert(find_text.is_object());
	if (find_text.count("find_history") == 0 || !find_text["find_history"].is_array()) {
		find_text["find_history"] = nlohmann::json::array();
	}
	nlohmann::json& find_history = find_text["find_history"];
	assert(find_history.is_array());
	if (find_history.size() > 0) {
		field->SetText(find_history[0]);
		field->Invalidate();
	}

	static char* toggle_regex_text = "toggle_regex";
	static char* toggle_matchcase_text = "toggle_matchcase";
	static char* toggle_wholeword_text = "toggle_wholeword";
	static char* toggle_wrap_text = "toggle_wrap";
	static char* toggle_highlight_text = "toggle_highlight";

	bool initial_values[5];
	initial_values[0] = find_text.get_if_exists(toggle_regex_text, false);
	initial_values[1] = find_text.get_if_exists(toggle_matchcase_text, false);
	initial_values[2] = find_text.get_if_exists(toggle_wholeword_text, false);
	initial_values[3] = find_text.get_if_exists(toggle_wrap_text, true);
	initial_values[4] = find_text.get_if_exists(toggle_highlight_text, true);

	ToggleButton* toggles[5];
	for (int i = 0; i < 5; i++) {
		toggles[i] = new ToggleButton(window, this, initial_values[i]);
		toggles[i]->SetSize(PGSize(button_height, button_height));
		toggles[i]->y = VPADDING;
		this->AddControl(toggles[i]);
	}
	toggle_regex = toggles[0];
	toggle_matchcase = toggles[1];
	toggle_wholeword = toggles[2];
	toggle_wrap = toggles[3];
	toggle_highlight = toggles[4];

	find_button->SetText(std::string("Find"), font);
	find_expand->SetText(std::string("v"), font);
	toggle_regex->SetText(std::string(".*"), font);
	toggle_matchcase->SetText(std::string("Aa"), font);
	toggle_wholeword->SetText(std::string("\"\""), font);
	toggle_wrap->SetText(std::string("W"), font);
	toggle_highlight->SetText(std::string("H"), font);

	toggle_regex->SetTooltip("Toggle regular expressions (Ctrl+R)");
	toggle_matchcase->SetTooltip("Toggle case sensitive search (Ctrl+C)");
	toggle_wholeword->SetTooltip("Toggle whole word search (Ctrl+W)");
	toggle_wrap->SetTooltip("Toggle search wrap (Ctrl+Z)");
	toggle_highlight->SetTooltip("Highlight search matches (Ctrl+H)");

	toggle_highlight->OnToggle([](Button* b, bool toggled, void* setting_name) {
		PGGetWorkspace(b->window)->settings["find_text"][((char*)setting_name)] = toggled;
		PGFindText* f = dynamic_cast<PGFindText*>(b->parent);
		if (toggled) {
			f->FindAll(true);
		} else {
			ControlManager* manager = GetControlManager(f);
			TextFile& tf = manager->active_textfield->GetTextFile();
			tf.ClearMatches();
			tf.SetSelectedMatch(-1);
		}
	}, toggle_highlight_text);
	auto update_highlight = [](Button* b, bool toggled, void* setting_name) {
		PGGetWorkspace(b->window)->settings["find_text"][((char*)setting_name)] = toggled;
		PGFindText* f = dynamic_cast<PGFindText*>(b->parent);
		if (f->HighlightMatches()) {
			f->FindAll(true);
		}};

	toggle_regex->OnToggle(update_highlight, toggle_regex_text);
	toggle_matchcase->OnToggle(update_highlight, toggle_matchcase_text);
	toggle_wholeword->OnToggle(update_highlight, toggle_wholeword_text);
	toggle_wrap->OnToggle(update_highlight, toggle_wrap_text);

	ControlManager* manager = GetControlManager(this);
	manager->OnTextChanged((PGControlDataCallback)UpdateHighlight, this);
	manager->OnActiveTextFieldChanged((PGControlDataCallback)UpdateHighlight, this);

	this->replace_field = nullptr;
	this->replace_button = nullptr;
	this->replace_all_button = nullptr;
	this->replace_in_selection_button = nullptr;
	this->files_to_include_field = nullptr;
	this->source_files_only = nullptr;
	this->respect_gitignore = nullptr;
	this->replace_expand = nullptr;
	this->find_prev = nullptr;
	this->find_all = nullptr;

	this->SetType(type);
}

PGFindText::~PGFindText() {
	ControlManager* manager = GetControlManager(this);
	manager->active_findtext = nullptr;
	manager->UnregisterOnTextChanged((PGControlDataCallback)UpdateHighlight, this);
	manager->UnregisterOnActiveTextFieldChanged((PGControlDataCallback)UpdateHighlight, this);
}

void PGFindText::SetType(PGFindTextType type) {
	this->type = type;

	// clear current controls
	std::vector<Control*> controls{ replace_field, replace_button, replace_all_button, replace_in_selection_button,
		files_to_include_field, source_files_only, respect_gitignore, replace_expand, find_prev, find_all };


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
	this->respect_gitignore = nullptr;
	this->replace_expand = nullptr;
	this->find_prev = nullptr;
	this->find_all = nullptr;

	if (type != PGFindReplaceManyFiles) {
		Button* buttons[2];
		for (int i = 0; i < 2; i++) {
			buttons[i] = new Button(window, this);
			buttons[i]->SetSize(PGSize(find_button->width, find_button->height));
			buttons[i]->y = find_button->y;
			this->AddControl(buttons[i]);
		}
		find_prev = buttons[0];
		find_all = buttons[1];

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
			this->fixed_height = field->height + VPADDING * 2;

			find_expand->y = VPADDING;
			find_expand->SetText(std::string("v"), font);
			find_expand->OnPressed([](Button* b, void* data) {
				PGFindText* tf = ((PGFindText*)data);
				tf->SetType(PGFindReplaceSingleFile);
			}, this);
			break;
		}
		case PGFindReplaceManyFiles:
		{
			PGScalar base_y = 2 * (field->height + VPADDING) + VPADDING;
			this->fixed_height = 3 * (field->height + VPADDING) + VPADDING;

			files_to_include_field = new SimpleTextField(window);
			files_to_include_field->width = this->width - 400;
			files_to_include_field->x = 150;
			files_to_include_field->y = base_y;
			this->AddControl(files_to_include_field);

			PGScalar button_width = field->height;
			PGScalar button_height = field->height;

			auto workspace = PGGetWorkspace(window);
			assert(workspace);
			if (workspace->settings.count("find_text") == 0 || !workspace->settings["find_text"].is_object()) {
				workspace->settings["find_text"] = nlohmann::json::object();
			}
			nlohmann::json& find_text = workspace->settings["find_text"];
			assert(find_text.is_object());

			static char* toggle_source_files_only = "toggle_source_only";
			static char* toggle_respect_gitignore = "toggle_respect_gitignore";

			bool initial_values[5];
			initial_values[0] = find_text.get_if_exists(toggle_source_files_only, true);
			initial_values[1] = find_text.get_if_exists(toggle_respect_gitignore, true);

			ToggleButton* toggles[2];
			for (int i = 0; i < 2; i++) {
				toggles[i] = new ToggleButton(window, this, initial_values[i]);
				toggles[i]->SetSize(PGSize(button_width, button_height));
				toggles[i]->y = base_y;
				this->AddControl(toggles[i]);
			}
			source_files_only = toggles[0];
			respect_gitignore = toggles[1];
			source_files_only->y = field->height + VPADDING * 2;
			respect_gitignore->y = base_y;

			source_files_only->SetText(std::string("Source Only"), font);
			respect_gitignore->SetText(std::string(".gitignore"), font);

			auto update_find_text = [](Button* b, bool toggled, void* setting_name) {
				PGGetWorkspace(b->window)->settings["find_text"][((char*)setting_name)] = toggled;
			};

			source_files_only->OnToggle(update_find_text, toggle_source_files_only);
			respect_gitignore->OnToggle(update_find_text, toggle_respect_gitignore);
		}
		case PGFindReplaceSingleFile:
		{
			PGScalar base_y = field->height + VPADDING * 2;
			if (type == PGFindReplaceSingleFile) {
				this->fixed_height = 2 * (field->height + VPADDING * 2);
			}

			replace_field = new SimpleTextField(window);
			replace_field->width = this->width - 400;
			replace_field->x = 150;
			replace_field->y = base_y;
			this->AddControl(replace_field);

			Button* buttons[3];
			int button_count = type == PGFindReplaceSingleFile ? 3 : 1;
			for (int i = 0; i < button_count; i++) {
				buttons[i] = new Button(window, this);
				buttons[i]->SetSize(PGSize(find_button->width, find_button->height));
				buttons[i]->y = base_y;
				this->AddControl(buttons[i]);
			}

			replace_button = buttons[0];
			replace_button->SetText(std::string("Replace"), font);

			if (type == PGFindReplaceSingleFile) {
				replace_all_button = buttons[1];
				replace_in_selection_button = buttons[2];

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
				replace_expand->SetSize(PGSize(field->height, field->height));
				replace_expand->y = base_y;
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

			find_expand->y = VPADDING;
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
	GetControlManager(this)->TriggerResize();
	this->TriggerResize();
}

void PGFindText::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();

	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);

	PGScalar field_offset = MeasureTextWidth(font, "Replace:") + HPADDING_SMALL * 2;
	x += HPADDING_SMALL;
	RenderString(renderer, font, "Find:", x + field->x - field_offset, y + field->y + (field->height - GetTextHeight(font)) / 2 - 1);
	if (replace_field) {
		RenderString(renderer, font, "Replace:", x + replace_field->x - field_offset, y + replace_field->y + (replace_field->height - GetTextHeight(font)) / 2 - 1);
	}
	if (files_to_include_field) {
		RenderString(renderer, font, "Filter:", x + files_to_include_field->x - field_offset, y + files_to_include_field->y + (files_to_include_field->height - GetTextHeight(font)) / 2 - 1);
	}

	PGContainer::Draw(renderer);
}

void PGFindText::OnResize(PGSize old_size, PGSize new_size) {
	toggle_regex->SetPosition(PGPoint(HPADDING_SMALL, toggle_regex->y));
	toggle_matchcase->SetPosition(PGPoint(toggle_regex->x + toggle_regex->width, toggle_matchcase->y));
	toggle_wholeword->SetPosition(PGPoint(toggle_matchcase->x + toggle_matchcase->width, toggle_wholeword->y));
	toggle_wrap->SetPosition(PGPoint(toggle_wholeword->x + toggle_wholeword->width + HPADDING_SMALL, toggle_wrap->y));
	toggle_highlight->SetPosition(PGPoint(toggle_wrap->x + toggle_wrap->width, toggle_highlight->y));
	if (source_files_only) {
		source_files_only->SetPosition(PGPoint(toggle_regex->x, source_files_only->y));
		respect_gitignore->SetPosition(PGPoint(toggle_regex->x, respect_gitignore->y));
		source_files_only->SetSize(PGSize(toggle_highlight->x + toggle_highlight->width - toggle_regex->x, source_files_only->height));
		respect_gitignore->SetSize(PGSize(source_files_only->width, respect_gitignore->height));
	}

	PGScalar field_offset = MeasureTextWidth(font, "In Files:") + HPADDING_SMALL * 2;;
	field->x = toggle_highlight->x + toggle_highlight->width + HPADDING_SMALL + field_offset;
	field->width = new_size.width - (type == PGFindReplaceManyFiles ? (find_button->width + HPADDING_SMALL) : hoffset) - field->x - field_offset;
	if (replace_field) {
		replace_field->x = field->x;
		replace_field->width = field->width;
	}
	if (files_to_include_field) {
		files_to_include_field->x = field->x;
		files_to_include_field->width = field->width;
	}
	find_button->SetPosition(PGPoint(field->x + field->width + HPADDING, find_button->y));
	if (replace_button) {
		replace_button->SetPosition(PGPoint(find_button->x, replace_button->y));
	}
	PGScalar find_expand_position = find_button->x + find_button->width + HPADDING;
	if (find_prev) {
		find_prev->SetPosition(PGPoint(find_button->x + find_button->width + HPADDING, find_prev->y));
		if (replace_all_button) {
			replace_all_button->SetPosition(PGPoint(find_prev->x, replace_all_button->y));
		}
		find_all->SetPosition(PGPoint(find_prev->x + find_prev->width + HPADDING, find_all->y));
		if (replace_in_selection_button) {
			replace_in_selection_button->SetPosition(PGPoint(find_all->x, replace_in_selection_button->y));
		}
		find_expand_position = find_all->x + find_all->width + HPADDING;
	}
	find_expand->SetPosition(PGPoint(find_expand_position, find_expand->y));
	if (replace_expand) {
		replace_expand->SetPosition(PGPoint(find_expand->x, replace_expand->y));
	}
}

nlohmann::json& PGFindText::GetFindHistory() {
	auto workspace = PGGetWorkspace(window);
	nlohmann::json& find_text = workspace->settings["find_text"];
	nlohmann::json& find_history = find_text["find_history"];
	assert(find_history.is_array());
	assert(history_entry >= 0 && (history_entry < find_history.size() || find_history.size() == 0));
	return find_history;
}

bool PGFindText::Find(PGDirection direction, bool include_selection) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	if (!tf.IsLoaded()) return false;
	char* error_message = nullptr;

	std::string pattern = field->GetText();
	PGRegexHandle regex_handle = PGCompileRegex(pattern, toggle_regex->IsToggled(), toggle_matchcase->IsToggled() ? PGRegexFlagsNone : PGRegexCaseInsensitive);
	if (!regex_handle) {
		// FIXME: throw an error, regex was not compiled properly
		error_message = panther::strdup("Error");
		return false;
	}

	bool found_result = tf.FindMatch(regex_handle, direction,
		toggle_wrap->IsToggled(),
		include_selection);

	nlohmann::json& find_history = GetFindHistory();
	/*
	if (find_history.size() == 0 || find_history[0] != search_text) {
		std::string first_entry = find_history[0];
		if (first_entry.size() == 0) {
			find_history[0] = search_text;
		} else {
			find_history.insert(find_history.begin(), search_text);
			if (find_history.size() > MAXIMUM_FIND_HISTORY) {
				find_history.erase(find_history.begin() + find_history.size() - 1);
			}
		}
		history_entry = 0;
	}*/

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
	PGRegexHandle regex = PGCompileRegex(regex_pattern, toggle_regex->IsToggled(), toggle_matchcase->IsToggled() ? PGRegexFlagsNone : PGRegexCaseInsensitive);
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

	// first find the list of files we want to search
	std::vector<PGFile> files;
	auto& directories = manager->active_projectexplorer->GetDirectories();
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->ListFiles(files, whitelist);
	}

	if (whitelist) {
		PGDestroyGlobSet(whitelist);
	}
	// now schedule the actual search
	textfile->FindAllMatchesAsync(files, regex, 2);
}

void PGFindText::SelectAllMatches(bool in_selection) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	if (!tf.IsLoaded()) return;

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
	tf.SelectMatches(in_selection);
}
void PGFindText::FindAll(bool select_first_match) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	if (!tf.IsLoaded()) return;
	char* error_message = nullptr;
	tf.SetSelectedMatch(0);
	auto begin_pos = tf.GetActiveCursor().BeginPosition();
	auto end_pos = tf.GetActiveCursor().EndPosition();
	std::string text = field->GetText();
	PGRegexHandle regex_handle = PGCompileRegex(text, toggle_regex->IsToggled(), toggle_matchcase->IsToggled() ? PGRegexFlagsNone : PGRegexCaseInsensitive);
	if (!regex_handle) {
		// FIXME: throw an error, regex was not compiled properly
		error_message = panther::strdup("Error");
		return;
	}
	tf.FindAllMatches(regex_handle, select_first_match,
		begin_pos.line, begin_pos.position,
		end_pos.line, end_pos.position,
		toggle_wrap->IsToggled());
	if (!error_message) {
		this->field->SetValidInput(true);
		this->Invalidate();
	} else {
		this->field->SetValidInput(false);
		this->Invalidate();
	}
}

void PGFindText::Replace() {
	if (!replace_field) return;
	std::string replacement = replace_field->GetText();
	if (this->Find(PGDirectionRight, true)) {
		ControlManager* manager = GetControlManager(this);
		TextFile& tf = manager->active_textfield->GetTextFile();
		PGRegexHandle regex = PGCompileRegex(field->GetText(), toggle_regex->IsToggled(), toggle_matchcase->IsToggled() ? PGRegexCaseInsensitive : PGRegexFlagsNone);
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

	this->SelectAllMatches(in_selection);
	if (tf.GetCursors().size() > 0) {
		// check if there are any matches
		PGRegexHandle regex = PGCompileRegex(field->GetText(), toggle_regex->IsToggled(), toggle_matchcase->IsToggled() ? PGRegexCaseInsensitive : PGRegexFlagsNone);
		tf.RegexReplace(regex, replacement);
		if (HighlightMatches()) {
			this->FindAll(false);
		}
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
		selected_field--;
	}
	new_focus = fields[selected_field];
	if (new_focus) {
		focused_control = new_focus;
		new_focus->RefreshCursors();
		new_focus->Invalidate();
	}
}

void PGFindText::InitializeKeybindings() {
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
}

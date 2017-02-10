
#include "controlmanager.h"
#include "findtext.h"
#include "style.h"
#include "workspace.h"

#define HPADDING 10
#define HPADDING_SMALL 5
#define VPADDING 4

#define MAXIMUM_FIND_HISTORY 5

PG_CONTROL_INITIALIZE_KEYBINDINGS(FindText);

FindText::FindText(PGWindowHandle window, bool replace) :
	PGContainer(window), find_history(nullptr), history_entry(0) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));

	field = new SimpleTextField(window);
	field->width = this->width - 400;
	field->x = 150;
	field->y = VPADDING;
	field->OnPrevEntry([](Control* c, void* data) {
		FindText* f = (FindText*)data;
		if (f->find_history) {
			if (f->find_history->size() > f->history_entry + 1) {
				f->history_entry++;
				((SimpleTextField*)c)->SetText((*f->find_history)[f->history_entry]);
			}
		}
	}, this);
	field->OnNextEntry([](Control* c, void* data) {
		FindText* f = (FindText*)data;
		if (f->find_history) {
			if (f->history_entry > 0) {
				f->history_entry--;
				((SimpleTextField*)c)->SetText((*f->find_history)[f->history_entry]);
			} else {
				if ((*f->find_history)[f->history_entry].size() > 0) {
					f->history_entry = 0;
					f->find_history->insert(f->find_history->begin(), std::string(""));
					((SimpleTextField*)c)->SetText((*f->find_history)[f->history_entry]);
				}
			}
		}
	}, this);
	field->OnTextChanged([](Control* c, void* data) {
		// if HighlightMatches is turned on, we search as soon as text is entered
		FindText* f = (FindText*)data;
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
	hoffset = (button_width + HPADDING) * 3 + HPADDING * 2 + button_height;

	Button* buttons[4];
	for (int i = 0; i < 4; i++) {
		buttons[i] = new Button(window, this);
		buttons[i]->SetSize(PGSize(button_width, button_height));
		buttons[i]->y = VPADDING;
		this->AddControl(buttons[i]);
	}

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
	this->find_history = &find_history;
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
	find_button = buttons[0];
	find_prev = buttons[1];
	find_all = buttons[2];
	find_expand = buttons[3];
	find_expand->SetSize(PGSize(button_height, button_height));
	toggle_regex = toggles[0];
	toggle_matchcase = toggles[1];
	toggle_wholeword = toggles[2];
	toggle_wrap = toggles[3];
	toggle_highlight = toggles[4];

	find_button->SetText(std::string("Find"), font);
	find_prev->SetText(std::string("Find Prev"), font);
	find_all->SetText(std::string("Find All"), font);
	find_expand->SetText(std::string("v"), font);
	toggle_regex->SetText(std::string(".*"), font);
	toggle_matchcase->SetText(std::string("Aa"), font);
	toggle_wholeword->SetText(std::string("\"\""), font);
	toggle_wrap->SetText(std::string("W"), font);
	toggle_highlight->SetText(std::string("H"), font);

	toggle_highlight->OnToggle([](Button* b, bool toggled, void* setting_name) {
		PGGetWorkspace(b->window)->settings["find_text"][((char*)setting_name)] = toggled;
		FindText* f = dynamic_cast<FindText*>(b->parent);
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
		FindText* f = dynamic_cast<FindText*>(b->parent);
		if (f->HighlightMatches()) {
			f->FindAll(true);
		}};

	toggle_regex->OnToggle(update_highlight, toggle_regex_text);
	toggle_matchcase->OnToggle(update_highlight, toggle_matchcase_text);
	toggle_wholeword->OnToggle(update_highlight, toggle_wholeword_text);
	toggle_wrap->OnToggle(update_highlight, toggle_wrap_text);
	find_prev->OnPressed([](Button* b, void* data) {
		((FindText*)data)->Find(PGDirectionLeft);
	}, this);
	find_button->OnPressed([](Button* b, void* data) {
		((FindText*)data)->Find(PGDirectionRight);
	}, this);
	find_all->OnPressed([](Button* b, void* data) {
		FindText* ft = ((FindText*)data);
		ft->SelectAllMatches();
		ft->Close();
	}, this);
	find_expand->OnPressed([](Button* b, void* data) {
		FindText* tf = ((FindText*)data);
		tf->ToggleReplace();
	}, this);

	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	SetTextfile(&tf);

	// FIXME: unsubscribe after findtext is done
	manager->active_textfield->OnTextChanged([](Control* c, void* data) {
		FindText* f = (FindText*) data;
		if (f->HighlightMatches()) {
			f->FindAll(false);
		};
	},
	this);

	if (replace) {
		ToggleReplace();
	}
}

FindText::~FindText() {
	

}

void FindText::SetTextfile(TextFile* textfile) {
	this->current_textfile = textfile;
	begin_pos = textfile->GetActiveCursor().BeginPosition();
	end_pos = textfile->GetActiveCursor().EndPosition();
}

void FindText::ToggleReplace() {
	replace = !replace;
	if (replace) {
		PGScalar base_y = field->height + VPADDING * 2;
		this->fixed_height = 2 * (field->height + VPADDING * 2);

		replace_field = new SimpleTextField(window);
		replace_field->width = this->width - 400;
		replace_field->x = 150;
		replace_field->y = base_y;
		this->AddControl(replace_field);
		Button* buttons[2];
		for (int i = 0; i < 2; i++) {
			buttons[i] = new Button(window, this);
			buttons[i]->SetSize(PGSize(find_button->width, find_button->height));
			buttons[i]->y = base_y;
			this->AddControl(buttons[i]);
		}
		replace_button = buttons[0];
		replace_all_button = buttons[1];
		replace_button->SetText(std::string("Replace"), font);
		replace_all_button->SetText(std::string("Replace All"), font);

		replace_button->OnPressed([](Button* b, void* data) {
			((FindText*)data)->Replace();
		}, this);
		replace_all_button->OnPressed([](Button* b, void* data) {
			FindText* ft = ((FindText*)data);
			ft->ReplaceAll();
			ft->Close();
		}, this);

		find_expand->y = base_y;
		find_expand->SetText(std::string("^"), font);
	} else {
		this->fixed_height = field->height + VPADDING * 2;
		auto replace_field = this->replace_field;
		this->replace_field = nullptr;
		this->RemoveControl(replace_field);

		auto replace_button = this->replace_button;
		this->replace_button = nullptr;
		this->RemoveControl(replace_button);

		auto replace_all_button = this->replace_all_button;
		this->replace_all_button = nullptr;
		this->RemoveControl(replace_all_button);


		find_expand->y = VPADDING;
		find_expand->SetText(std::string("v"), font);
	}
	GetControlManager(this)->TriggerResize();
	GetControlManager(this)->Invalidate();
}

void FindText::Draw(PGRendererHandle renderer, PGIRect* rect) {
	PGScalar x = X() - rect->x;
	PGScalar y = Y() - rect->y;

	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);
	PGContainer::Draw(renderer, rect);
}

void FindText::OnResize(PGSize old_size, PGSize new_size) {
	toggle_regex->SetPosition(PGPoint(HPADDING_SMALL, toggle_regex->y));
	toggle_matchcase->SetPosition(PGPoint(toggle_regex->x + toggle_regex->width, toggle_matchcase->y));
	toggle_wholeword->SetPosition(PGPoint(toggle_matchcase->x + toggle_matchcase->width, toggle_wholeword->y));
	toggle_wrap->SetPosition(PGPoint(toggle_wholeword->x + toggle_wholeword->width + HPADDING_SMALL, toggle_wrap->y));
	toggle_highlight->SetPosition(PGPoint(toggle_wrap->x + toggle_wrap->width, toggle_highlight->y));

	field->x = toggle_highlight->x + toggle_highlight->width + HPADDING_SMALL;
	field->width = new_size.width - hoffset - field->x;
	if (replace_field) {
		replace_field->x = field->x;
		replace_field->width = field->width;
	}
	find_button->SetPosition(PGPoint(field->x + field->width + HPADDING, find_button->y));
	if (replace_button) {
		replace_button->SetPosition(PGPoint(field->x + field->width + HPADDING, replace_button->y));
	}
	find_prev->SetPosition(PGPoint(find_button->x + find_button->width + HPADDING, find_prev->y));
	if (replace_all_button) {
		replace_all_button->SetPosition(PGPoint(find_button->x + find_button->width + HPADDING, replace_all_button->y));
	}
	find_all->SetPosition(PGPoint(find_prev->x + find_prev->width + HPADDING, find_all->y));
	find_expand->SetPosition(PGPoint(find_all->x + find_all->width + HPADDING, find_expand->y));
}

bool FindText::Find(PGDirection direction, bool include_selection) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	char* error_message = nullptr;
	SetTextfile(&tf);

	std::string search_text = field->GetText();
	bool found_result = tf.FindMatch(search_text, direction,
		&error_message,
		toggle_matchcase->IsToggled(), toggle_wrap->IsToggled(), toggle_regex->IsToggled(),
		include_selection);

	/*
		if (find_history->size() == 0 || (*find_history)[0] != search_text) {
			std::string first_entry = (*find_history)[0];
			if (first_entry.size() == 0) {
				(*find_history)[0] = search_text;
			} else {
				(*find_history).insert(find_history->begin(), search_text);
				if ((*find_history).size() > MAXIMUM_FIND_HISTORY) {
					find_history->erase(find_history->begin() + find_history->size() - 1);
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

void FindText::SelectAllMatches() {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();

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
	tf.SelectMatches();
}
void FindText::FindAll(bool select_first_match) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	char* error_message = nullptr;
	std::string text = field->GetText();
	if (&tf != current_textfile)
		SetTextfile(&tf);
	tf.SetSelectedMatch(0);
	tf.FindAllMatches(text, select_first_match,
		begin_pos.line, begin_pos.position,
		end_pos.line, end_pos.position,
		&error_message,
		toggle_matchcase->IsToggled(), toggle_wrap->IsToggled(), toggle_regex->IsToggled());
	if (!error_message) {
		this->field->SetValidInput(true);
		this->Invalidate();
	} else {
		this->field->SetValidInput(false);
		this->Invalidate();
	}
}

void FindText::Replace() {
	if (!replace_field) return;
	std::string replacement = replace_field->GetText();
	// FIXME: only find first time if not already on a found selection
	if (this->Find(PGDirectionRight, true)) {
		ControlManager* manager = GetControlManager(this);
		TextFile& tf = manager->active_textfield->GetTextFile();
		tf.InsertText(replacement);
		SetTextfile(&tf);
		if (HighlightMatches()) {
			this->FindAll(PGDirectionRight);
		}
	}
}

void FindText::ReplaceAll() {
	if (!replace_field) return;
	std::string replacement = replace_field->GetText();
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();

	this->SelectAllMatches();
	if (tf.GetCursors().size() > 0) {
		// check if there are any matches
		tf.InsertText(replacement);
		this->SetTextfile(&tf);
		if (HighlightMatches()) {
			this->FindAll(false);
		}
	}
}

bool FindText::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(FindText::keybindings, button, modifier)) {
		return true;
	}
	return PGContainer::KeyboardButton(button, modifier);
}

void FindText::Close() {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	tf.ClearMatches();
	dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);
}

void FindText::ShiftTextfieldFocus(PGDirection direction) {
	if (replace) {
		// (shift+)tab switches between the two text fields
		if (focused_control == field) {
			focused_control = replace_field;
			replace_field->RefreshCursors();
			replace_field->Invalidate();
		} else {
			focused_control = field;
			field->RefreshCursors();
			field->Invalidate();
		}
	}
}

void FindText::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = FindText::keybindings_noargs;
	noargs["find_next"] = [](Control* c) {
		((FindText*)c)->Find(PGDirectionRight);
	};
	noargs["find_prev"] = [](Control* c) {
		((FindText*)c)->Find(PGDirectionLeft);
	};
	noargs["close"] = [](Control* c) {
		((FindText*)c)->Close();
	};
	noargs["shift_focus_forward"] = [](Control* c) {
		((FindText*)c)->ShiftTextfieldFocus(PGDirectionRight);
	};
	noargs["shift_focus_backward"] = [](Control* c) {
		((FindText*)c)->ShiftTextfieldFocus(PGDirectionLeft);
	};
}

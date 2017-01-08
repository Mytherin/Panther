
#include "controlmanager.h"
#include "findtext.h"
#include "style.h"

#define HPADDING 10
#define HPADDING_SMALL 5
#define VPADDING 4

static void CancelOperation(Control* c, void* data, PGModifier modifier) {
	// user pressed escape, cancelling the find operation
	((FindText*)data)->Close();
}

static void ExecuteFind(Control* c, void* data, PGModifier modifier) {
	// perform the find operation
	// if we hold shift, we perform Find Prev instead of Find Next
	((FindText*)data)->Find(modifier & PGModifierShift ? PGDirectionLeft : PGDirectionRight);
}

FindText::FindText(PGWindowHandle window, bool replace) :
	PGContainer(window) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));

	field = new SimpleTextField(window);
	field->width = this->width - 400;
	field->x = 150;
	field->y = VPADDING;
	field->OnUserCancel(CancelOperation, (void*) this);
	field->OnSuccessfulExit(ExecuteFind, (void*) this);
	field->OnTextChanged([](Control* c, void* data) {
		// if HighlightMatches is turned on, we search as soon as text is entered
		FindText* f = (FindText*)data;
		if (f->HighlightMatches()) {
			f->FindAll(PGDirectionRight);
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
	// FIXME: remember toggled buttons from settings and save toggles in settings after being made
	ToggleButton* toggles[5];
	for (int i = 0; i < 5; i++) {
		toggles[i] = new ToggleButton(window, this, false);
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

	toggle_wrap->Toggle();
	toggle_highlight->Toggle();

	find_button->SetText(std::string("Find"), font);
	find_prev->SetText(std::string("Find Prev"), font);
	find_all->SetText(std::string("Find All"), font);
	find_expand->SetText(std::string("v"), font);
	toggle_regex->SetText(std::string(".*"), font);
	toggle_matchcase->SetText(std::string("Aa"), font);
	toggle_wholeword->SetText(std::string("\"\""), font);
	toggle_wrap->SetText(std::string("W"), font);
	toggle_highlight->SetText(std::string("H"), font);

	toggle_highlight->OnToggle([](Button* b, bool toggled) {
		FindText* f = dynamic_cast<FindText*>(b->parent);
		if (toggled) {
			f->FindAll(PGDirectionRight);
		} else {
			ControlManager* manager = GetControlManager(f);
			TextFile& tf = manager->active_textfield->GetTextFile();
			tf.ClearMatches();
			tf.SetSelectedMatch(-1);
		}
	});
	auto update_highlight = [](Button* b, bool toggled) {
		FindText* f = dynamic_cast<FindText*>(b->parent);
		if (f->toggle_highlight) {
			f->FindAll(PGDirectionRight);
		}};

	toggle_regex->OnToggle(update_highlight);
	toggle_matchcase->OnToggle(update_highlight);
	toggle_wholeword->OnToggle(update_highlight);
	toggle_wrap->OnToggle(update_highlight);
	find_prev->OnPressed([](Button* b) {
		dynamic_cast<FindText*>(b->parent)->Find(PGDirectionLeft);
	});
	find_button->OnPressed([](Button* b) {
		dynamic_cast<FindText*>(b->parent)->Find(PGDirectionRight);
	});
	find_all->OnPressed([](Button* b) {
		FindText* ft = dynamic_cast<FindText*>(b->parent);
		ft->SelectAllMatches();
		ft->Close();
	});
	find_expand->OnPressed([](Button* b) {
		FindText* tf = dynamic_cast<FindText*>(b->parent);
		tf->ToggleReplace();
	});

	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	SetTextfile(&tf);

	if (replace) {
		ToggleReplace();
	}
}

FindText::~FindText() {

}

void FindText::SetTextfile(TextFile* textfile) {
	this->current_textfile = textfile;
	begin_pos = textfile->GetActiveCursor()->BeginPosition();
	end_pos = textfile->GetActiveCursor()->EndPosition();
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
		replace_field->OnUserCancel(CancelOperation, (void*) this);
		replace_field->OnSuccessfulExit(ExecuteFind, (void*) this);
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

		replace_button->OnPressed([](Button* b) {
			dynamic_cast<FindText*>(b->parent)->Replace();
		});
		replace_all_button->OnPressed([](Button* b) {
			FindText* ft = dynamic_cast<FindText*>(b->parent);
			ft->ReplaceAll();
			ft->Close();
		});

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

	bool found_result = tf.FindMatch(field->GetText(), direction,
		&error_message,
		toggle_matchcase->IsToggled(), toggle_wrap->IsToggled(), toggle_regex->IsToggled(), 
		include_selection);
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

	if (tf.FinishedSearch()) {
		tf.SelectMatches();
	} else {
		assert(0);
		// wait?
	}
}
void FindText::FindAll(PGDirection direction) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	char* error_message = nullptr;
	std::string text = field->GetText();
	if (&tf != current_textfile)
		SetTextfile(&tf);
	tf.SetSelectedMatch(0);
	tf.FindAllMatches(text, direction, 
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
		if (this->toggle_highlight) {
			this->FindAll(PGDirectionRight);
		}
	}
}

void FindText::ReplaceAll() {
	if (!replace_field) return;
	std::string replacement = replace_field->GetText();
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();

	if (tf.FinishedSearch()) {
		tf.SelectMatches();
		if (tf.GetCursors().size() > 0) {
			// check if there are any matches
			tf.InsertText(replacement);
			if (this->toggle_highlight) {
				this->FindAll(PGDirectionRight);
			}
		}
	} else {
		assert(0);
		// wait?
	}
}


bool FindText::KeyboardButton(PGButton button, PGModifier modifier) {
	if (button == PGButtonTab) {
		if (!(modifier & PGModifierCtrl)) {
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
				return true;
			}
		}
	}
	return PGContainer::KeyboardButton(button, modifier);
}

void FindText::Close() {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	tf.ClearMatches();
	dynamic_cast<PGContainer*>(this->parent)->RemoveControl(this);
}

#include "controlmanager.h"
#include "findtext.h"
#include "style.h"

#define HPADDING 10
#define HPADDING_SMALL 5
#define VPADDING 4

FindText::FindText(PGWindowHandle window) :
	PGContainer(window) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));

	field = new SimpleTextField(window);
	field->width = this->width - 400;
	field->x = 150;
	field->y = VPADDING;
	field->OnTextChanged([](Control* c, void* data) {
		// text changed, if highlight matches is on we immediately execute the find operation
		// FIXME:
	}, (void*) this);
	field->OnUserCancel([](Control* c, void* data, PGModifier modifier) {
		// user pressed escape, cancelling the find operation
		Control* control = (Control*)data;
		dynamic_cast<PGContainer*>(control->parent)->RemoveControl(control);
	}, (void*) this);
	field->OnSuccessfulExit([](Control* c, void* data, PGModifier modifier) {
		// execute find
		((FindText*)data)->Find(modifier & PGModifierShift ? PGDirectionLeft : PGDirectionRight);
	}, (void*) this);
	this->AddControl(field);
	this->height = field->height + VPADDING * 2;

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
	find_expand->SetText(std::string("V"), font);
	toggle_regex->SetText(std::string(".*"), font);
	toggle_matchcase->SetText(std::string("Aa"), font);
	toggle_wholeword->SetText(std::string("\"\""), font);
	toggle_wrap->SetText(std::string("W"), font);
	toggle_highlight->SetText(std::string("H"), font);

	find_prev->OnPressed([](Button* b) {
		dynamic_cast<FindText*>(b->parent)->Find(PGDirectionLeft);
	});
	find_button->OnPressed([](Button* b) {
		dynamic_cast<FindText*>(b->parent)->Find(PGDirectionRight);
	});/*
	field->OnTextChanged([]() {

	}, this);*/
}

FindText::~FindText() {

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
	find_button->SetPosition(PGPoint(field->x + field->width + HPADDING, find_button->y));
	find_prev->SetPosition(PGPoint(find_button->x + find_button->width + HPADDING, find_prev->y));
	find_all->SetPosition(PGPoint(find_prev->x + find_prev->width + HPADDING, find_all->y));
	find_expand->SetPosition(PGPoint(find_all->x + find_all->width + HPADDING, find_expand->y));
}

void FindText::Find(PGDirection direction) {
	ControlManager* manager = GetControlManager(this);
	TextFile& tf = manager->active_textfield->GetTextFile();
	char* error_message = nullptr;
	PGFindMatch match = tf.FindMatch(field->GetText(), direction, 
		tf.GetActiveCursor()->BeginLine(), tf.GetActiveCursor()->BeginPosition(), 
		tf.GetActiveCursor()->EndLine(), tf.GetActiveCursor()->EndPosition(),
		&error_message,
		toggle_matchcase->IsToggled(), toggle_wrap->IsToggled(), toggle_regex->IsToggled());
	if (!error_message) {
		if (match.start_character >= 0) {
			tf.SetCursorLocation(match.start_line, match.start_character, match.end_line, match.end_character);
		}
		this->field->SetValidInput(true);
		this->Invalidate();
	} else {
		this->field->SetValidInput(false);
		this->Invalidate();
	}
}

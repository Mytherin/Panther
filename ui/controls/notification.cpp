
#include "label.h"
#include "notification.h"
#include "style.h"

PGNotification::PGNotification(PGWindowHandle window, PGNotificationType type, PGScalar width, std::string text) :
	PGContainer(window), type(type) {
	this->width = width;

	font = PGCreateFont(PGFontTypeUI);
	SetTextFontSize(font, 13);

	Label *warning_label = new Label(window, this);
	warning_label->SetText(text, font);
	warning_label->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop);
	warning_label->margin.left = 5;
	warning_label->margin.top = warning_label->margin.bottom = PGMarginAuto;
	warning_label->padding = PGPadding(5, 5, 2, 2);
	warning_label->fixed_height = GetTextHeight(font);
	warning_label->fixed_size = false;
	switch (type) {
		case PGNotificationTypeError:
			warning_label->SetText("Error", font);
			warning_label->background_color = PGStyleManager::GetColor(PGColorNotificationError);
			break;
		case PGNotificationTypeWarning:
			warning_label->SetText("Warning", font);
			warning_label->background_color = PGStyleManager::GetColor(PGColorNotificationWarning);
			break;
	}
	this->AddControl(std::shared_ptr<Control>(warning_label));

	this->label = new Label(window, this);
	this->label->SetText(text, font);
	this->label->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop);
	this->label->percentage_width = 1;
	this->label->left_anchor = warning_label;
	this->label->margin.left = 5;
	this->label->padding = PGPadding(0, 0, 5, 5);
	this->label->fixed_size = false;
	this->label->wrap_text = true;
	this->AddControl(std::shared_ptr<Control>(label));



	RecomputeHeight();
}

PGNotification::~PGNotification() {
	for (auto it = buttons.begin(); it != buttons.end(); it++) {
		delete *it;
	}
	buttons.clear();
}

void PGNotification::RecomputeHeight() {
	Control::TriggerResize();
	this->height = label->height;
}

void PGNotification::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();

	// render the background of the notification
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height),
		PGStyleManager::GetColor(PGColorNotificationBackground), PGDrawStyleFill);

	// render the buttons (controls)
	PGContainer::Draw(renderer);
}

void PGNotification::AddButton(PGControlDataCallback callback, Control* c, void* data, std::string button_text) {
	ButtonData* b = new ButtonData();
	b->callback = callback;
	b->data = data;
	b->control = c;
	Button* button = new Button(window, this);
	button->SetText(button_text, font);
	b->button = button;
	this->buttons.push_back(b);

	button->SetAnchor(PGAnchorTop | PGAnchorRight);
	button->fixed_size = false;
	button->fixed_height = GetTextHeight(font) + 4;
	button->padding.left = button->padding.right = 10;
	button->margin.left = 2;
	button->margin.right = 5;
	button->margin.top = PGMarginAuto;
	button->margin.bottom = PGMarginAuto;
	button->right_anchor = label->right_anchor;
	button->background_color = PGStyleManager::GetColor(PGColorNotificationButton);
	button->background_stroke_color = PGColor(0, 0, 0);
	button->OnPressed([](Button* b, void* data) {
		ButtonData* bd = ((ButtonData*)data);
		bd->callback(bd->control, bd->data);
	}, b);
	this->AddControl(std::shared_ptr<Control>(button));
	label->right_anchor = button;
	RecomputeHeight();
}